/*
 * Copyright (C) 2026 Kyaw Tun Linn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "ConversionManager.h"
#include <trantor/utils/Logger.h>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

ConversionManager::ConversionManager() {
    // Start worker threads equal to CPU cores (or at least 2)
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2; // Fallback
    
    LOG_INFO << "Starting ConversionManager with " << numThreads << " worker threads.";

    for (unsigned int i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ConversionManager::workerThread, this);
    }
    
    // Start cleanup thread
    cleanupThread_ = std::thread(&ConversionManager::cleanupLoop, this);
}

ConversionManager::~ConversionManager() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    condition_.notify_all();
    cleanupCondition_.notify_all(); // Wake up cleanup thread
    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }
}

void ConversionManager::addTask(const std::vector<std::string>& args, const std::string& inputFilename, const std::string& outputFilename, std::function<void(bool success)> callback) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        tasks_.push({args, outputFilename, inputFilename, callback});
    }
    condition_.notify_one();
}

void ConversionManager::workerThread() {
    while (true) {
        ConversionTask task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            
            if (stop_ && tasks_.empty()) return;
            
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        std::string cmdStr;
        for(const auto& arg : task.args) cmdStr += arg + " ";
        LOG_INFO << "Worker processing conversion: " << cmdStr;
        
        // Secure execution using fork/exec
        pid_t pid = fork();
        int status = 0;
        bool success = false;

        if (pid == -1) {
            LOG_ERROR << "Failed to fork";
            success = false;
        } else if (pid == 0) {
            // Child process
            std::vector<char*> cargs;
            for (const auto& arg : task.args) {
                cargs.push_back(const_cast<char*>(arg.c_str()));
            }
            cargs.push_back(nullptr);

            // Redirect stdout/stderr to /dev/null using low-level file descriptors
            // This avoids "ignoring return value of freopen" warnings and is safer in forked processes.
            int devNull = open("/dev/null", O_WRONLY);
            if (devNull != -1) {
                dup2(devNull, STDOUT_FILENO); // Redirect stdout
                dup2(devNull, STDERR_FILENO); // Redirect stderr
                close(devNull);
            }
            
            execvp(cargs[0], cargs.data());
            
            // If execvp returns, it failed
            exit(1); 
        } else {
            // Parent process
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                int exitCode = WEXITSTATUS(status);
                LOG_INFO << "Worker execution result: " << exitCode;
                success = (exitCode == 0);
            } else {
                LOG_ERROR << "Child process terminated abnormally";
                success = false;
            }
        }
        
        if (success) {
            ConversionManager::instance().incrementTotalConversions();
        }
        
        if (task.callback) {
            task.callback(success);
        }
    }
}

void ConversionManager::cleanupLoop() {
    namespace fs = std::filesystem;
    // Cleanup every 5 minutes
    const auto cleanupInterval = std::chrono::minutes(5);
    // Delete files older than 1 hour
    const auto maxFileAge = std::chrono::hours(1);
    
    while (true) {
        {
            // Use dedicated cleanup mutex to avoid contention with workers
            std::unique_lock<std::mutex> lock(cleanupMutex_);
            if (cleanupCondition_.wait_for(lock, cleanupInterval, [this] { return stop_; })) {
                return; // Stop requested
            }
        }
        
        LOG_INFO << "Running old file cleanup...";
        
        std::vector<std::string> dirs = {"./uploads/", "./www/downloads/"};
        auto now = fs::file_time_type::clock::now();
        
        for (const auto& dirPath : dirs) {
            if (!fs::exists(dirPath)) continue;
            
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                if (!entry.is_regular_file()) continue;
                
                try {
                    auto ftime = entry.last_write_time();
                    if ((now - ftime) > maxFileAge) {
                        LOG_INFO << "Deleting old file: " << entry.path();
                        fs::remove(entry.path());
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR << "Error deleting file: " << e.what();
                }
            }
        }
    }
}
