/*
 * Copyright (C) 2026 Kyaw Tun Linn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <drogon/HttpResponse.h>

using namespace drogon;

struct ConversionTask {
    std::vector<std::string> args;
    std::string outputFilename; // For verification or just logging
    std::string inputFilename; // To cleanup if needed
    std::function<void(bool success)> callback;
};

/**
 * @class ConversionManager
 * @brief Singleton service that manages async file conversion tasks.
 * 
 * This class implements a thread pool pattern to handle multiple conversion
 * tasks concurrently without blocking the main event loop. It uses a secure
 * fork/exec mechanism to execute external commands (ffmpeg, zip).
 */
class ConversionManager {
public:
    /**
     * @brief Get the singleton instance.
     * @return Reference to the ConversionManager instance.
     */
    static ConversionManager& instance() {
        static ConversionManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator to ensure singleton uniqueness
    ConversionManager(const ConversionManager&) = delete;
    void operator=(const ConversionManager&) = delete;

    /**
     * @brief Adds a task to the execution queue.
     * @param args Vector of command arguments (e.g., {"ffmpeg", "-i", ...}).
     * @param inputFilename Path to input file (for cleanup tracking).
     * @param outputFilename Path to output file.
     * @param callback Function to call upon task completion.
     */
    void addTask(const std::vector<std::string>& args, const std::string& inputFilename, const std::string& outputFilename, std::function<void(bool success)> callback);
    
    uint64_t getTotalConversions() const { return totalConversions_; }
    void incrementTotalConversions() { totalConversions_++; }

private:
    ConversionManager();
    ~ConversionManager();

    std::atomic<uint64_t> totalConversions_{0};

    // Background worker thread loop
    void workerThread();
    // Periodic file cleanup loop
    void cleanupLoop();

    std::vector<std::thread> workers_;
    std::thread cleanupThread_;
    std::queue<ConversionTask> tasks_;
    
    // Mutex for protecting the task queue
    std::mutex queueMutex_;
    std::condition_variable condition_;
    
    // Separate mutex for cleanup thread to avoid contention with workers
    std::mutex cleanupMutex_;
    std::condition_variable cleanupCondition_;
    
    bool stop_ = false;
};
