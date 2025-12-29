/*
 * Copyright (C) 2026 Kyaw Tun Linn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "ConverterController.h"
#include "../services/ConversionManager.h"
#include "../services/RateLimiter.h"
#include <drogon/utils/Utilities.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>

void ConverterController::convert(const HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Step 1: Rate Limiting
    // Check if the client IP has exceeded the allowed number of requests per hour.
    std::string clientIP = req->getPeerAddr().toIp();
    if (!RateLimiter::instance().isAllowed(clientIP)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k429TooManyRequests);
        
        Json::Value json;
        json["status"] = "error";
        json["error"] = "Rate limit exceeded. Maximum 10 conversions per hour.";
        json["remaining"] = RateLimiter::instance().getRemainingRequests(clientIP);
        
        resp->setBody(json.toStyledString());
        resp->setContentTypeCode(CT_APPLICATION_JSON);
        callback(resp);
        return;
    }
    
    // Step 2: Handle File Upload
    // Parse the multipart request to extract the uploaded file.
    MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0 || fileUpload.getFiles().empty())
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("No file uploaded");
        callback(resp);
        return;
    }

    auto &file = fileUpload.getFiles()[0];
    auto uuid = drogon::utils::getUuid(); // Generate unique ID for this conversion task
    
    // Step 3: Security Sanitization
    // Sanitize the original filename to prevent Command Injection and Path Traversal attacks.
    // We strictly allow only alphanumeric characters, dots, dashes, and underscores.
    std::string rawFilename = file.getFileName();
    std::string safeFilename;
    for (char c : rawFilename) {
        if (isalnum(c) || c == '.' || c == '-' || c == '_') {
            safeFilename += c;
        } else {
            safeFilename += '_';
        }
    }
    
    // Fallback if filename becomes empty (e.g., was all special symbols)
    if (safeFilename.empty() || safeFilename == "." || safeFilename == "..") {
        safeFilename = "video_file";
    }

    std::string uploadDir = "./uploads/";
    std::string inputFilename = uploadDir + uuid + "_" + safeFilename;
    
    // Step 4: File Size Validation
    // Enforce a maximum file size limit (500MB) to prevent Denial of Service (DoS).
    const size_t MAX_FILE_SIZE = 500 * 1024 * 1024; // 500MB
    if (file.fileLength() > MAX_FILE_SIZE) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k413RequestEntityTooLarge);
        resp->setBody("File too large. Maximum size: 500MB");
        callback(resp);
        return;
    }
    
    // Ensure uploads directory exists
    if (!std::filesystem::exists(uploadDir)) {
        std::filesystem::create_directory(uploadDir);
    }

    // Save the uploaded file to disk
    file.saveAs(inputFilename);

    // Step 5: Parameter Extraction
    // Get target format and quality settings from the request.
    auto &params = fileUpload.getParameters();
    std::string targetFormat;
    auto it = params.find("format");
    if (it != params.end()) {
        targetFormat = it->second;
    }
    if (targetFormat.empty()) targetFormat = "mp3";
    
    // Get quality preset (high/medium/low/podcast)
    std::string quality = "medium"; // default
    auto qualityIt = params.find("quality");
    if (qualityIt != params.end()) {
        quality = qualityIt->second;
    }

    // Validate format - now supporting AAC, FLAC, M4A, OPUS
    if (targetFormat != "mp3" && targetFormat != "wav" && targetFormat != "ogg" &&
        targetFormat != "aac" && targetFormat != "flac" && targetFormat != "m4a" && targetFormat != "opus") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid format. Supported: mp3, wav, ogg, aac, flac, m4a, opus");
        callback(resp);
        // Clean up input
        std::filesystem::remove(inputFilename);
        return;
    }
    
    // Validate quality preset
    if (quality != "high" && quality != "medium" && quality != "low" && quality != "podcast") {
        quality = "medium"; // fallback to default
    }

    LOG_INFO << "File saved to: " << inputFilename;

    std::string outputFilename = uploadDir + uuid + "." + targetFormat;

    // Construct ffmpeg command with quality presets
    // Construct ffmpeg command args
    std::vector<std::string> args;
    args.push_back("ffmpeg");
    args.push_back("-nostdin");
    args.push_back("-i");
    args.push_back(inputFilename);
    args.push_back("-vn");
    
    // Quality Presets Implementation
    if (targetFormat == "mp3") {
        args.push_back("-acodec"); args.push_back("libmp3lame");
        if (quality == "high") {
            args.push_back("-b:a"); args.push_back("320k");
        } else if (quality == "medium") {
            args.push_back("-q:a"); args.push_back("2");
        } else if (quality == "low") {
            args.push_back("-q:a"); args.push_back("5");
        } else { // podcast
            args.push_back("-b:a"); args.push_back("64k");
            args.push_back("-ac"); args.push_back("1");
        }
    } 
    else if (targetFormat == "aac" || targetFormat == "m4a") {
        args.push_back("-acodec"); args.push_back("aac");
        if (quality == "high") {
            args.push_back("-b:a"); args.push_back("256k");
        } else if (quality == "medium") {
            args.push_back("-b:a"); args.push_back("192k");
        } else if (quality == "low") {
            args.push_back("-b:a"); args.push_back("128k");
        } else { // podcast
            args.push_back("-b:a"); args.push_back("64k");
            args.push_back("-ac"); args.push_back("1");
        }
    }
    else if (targetFormat == "ogg") {
        args.push_back("-acodec"); args.push_back("libvorbis");
        if (quality == "high") {
            args.push_back("-q:a"); args.push_back("6");
        } else if (quality == "medium") {
            args.push_back("-q:a"); args.push_back("4");
        } else if (quality == "low") {
            args.push_back("-q:a"); args.push_back("3");
        } else { // podcast
            args.push_back("-q:a"); args.push_back("1");
            args.push_back("-ac"); args.push_back("1");
        }
    }
    else if (targetFormat == "opus") {
        args.push_back("-acodec"); args.push_back("libopus");
        if (quality == "high") {
            args.push_back("-b:a"); args.push_back("192k");
        } else if (quality == "medium") {
            args.push_back("-b:a"); args.push_back("128k");
        } else if (quality == "low") {
            args.push_back("-b:a"); args.push_back("96k");
        } else { // podcast
            args.push_back("-b:a"); args.push_back("48k");
            args.push_back("-ac"); args.push_back("1");
        }
    }
    else if (targetFormat == "flac") {
        args.push_back("-acodec"); args.push_back("flac");
        if (quality == "podcast") {
            args.push_back("-ar"); args.push_back("22050");
            args.push_back("-ac"); args.push_back("1");
        }
    }
    else if (targetFormat == "wav") {
        args.push_back("-acodec"); args.push_back("pcm_s16le");
        if (quality == "podcast") {
            args.push_back("-ar"); args.push_back("22050");
            args.push_back("-ac"); args.push_back("1");
        }
    }

    args.push_back(outputFilename);
    args.push_back("-y");

    std::string downloadDir = "./www/downloads/";
    if (!std::filesystem::exists(downloadDir)) {
        std::filesystem::create_directory(downloadDir);
    }
    
    std::filesystem::path originalPath(safeFilename);
    std::string stem = originalPath.stem().string();
    std::string shortUuid = uuid.substr(0, 5);
    std::string newBaseName = "konverter_" + shortUuid + "_" + stem;

    // Use ConversionManager for async processing
    auto callbackCopy = callback; // shared_ptr copy
    
    // No global mutex needed anymore due to unique file paths (UUID)
    
    ConversionManager::instance().addTask(args, inputFilename, outputFilename, 
        [destinationDir = downloadDir, outputFilename, inputFilename, targetFormat, newBaseName, callbackCopy](bool success) {
            
            if (!success) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody("Conversion failed");
                callbackCopy(resp);
                
                // Cleanup
                std::filesystem::remove(inputFilename);
                return;
            }

            // Success logic
            std::string publicOutputFilename = destinationDir + newBaseName + "." + targetFormat;
            
            try {
                // Move file
                std::filesystem::rename(outputFilename, publicOutputFilename);
                std::filesystem::remove(inputFilename); // Delete source video
            } catch (const std::filesystem::filesystem_error& e) {
                LOG_ERROR << "File operation failed: " << e.what();
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody("File operation failed");
                callbackCopy(resp);
                return;
            }
            
            Json::Value json;
            json["status"] = "success";
            json["download_url"] = "/downloads/" + newBaseName + "." + targetFormat;

            auto resp = HttpResponse::newHttpJsonResponse(json);
            callbackCopy(resp);
        });
}

// Step 7: Zip Archive Creation
// Bundles valid files into a standardized zip archive for batch download.
void ConverterController::createZip(const HttpRequestPtr &req,
                                    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }
    
    const Json::Value& files = (*jsonPtr)["files"];
    if (!files.isArray() || files.empty()) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Files list required");
        callback(resp);
        return;
    }
    
    std::string downloadDir = "./www/downloads/";
    
    // Construct zip command arguments securely
    std::vector<std::string> args;
    args.push_back("zip");
    args.push_back("-j"); // junk paths (don't include directory structure in zip)
    args.push_back("-q"); // quiet mode
    
    // Generate unique zip filename
    auto uuid = drogon::utils::getUuid();
    std::string zipName = "batch_" + uuid + ".zip";
    std::string zipPath = downloadDir + zipName;
    
    args.push_back(zipPath);
    
    // Validate requested files
    for (const auto& file : files) {
        std::string filename = file.asString();
        // Basic sanitization/validation: ensure it's just a filename in downloads dir
        // We only allow files that exist in ./www/downloads/
        
        // Remove any directory components for security
        std::filesystem::path p(filename);
        std::string basename = p.filename().string();
        
        std::string fullPath = downloadDir + basename;
        // Verify file existence to prevent zipping non-existent or malicious paths
        if (std::filesystem::exists(fullPath)) {
            args.push_back(fullPath);
        }
    }
    
    if (args.size() <= 4) { // request + zip + -j + -q + output + nothing?
       // Means no valid files found
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("No valid files to zip");
        callback(resp);
        return;
    }
    
    // Use ConversionManager to execute zip command async
    auto callbackCopy = callback;
    
    ConversionManager::instance().addTask(args, "", zipPath, 
        [zipName, callbackCopy](bool success) {
            if (success) {
                Json::Value json;
                json["status"] = "success";
                json["download_url"] = "/downloads/" + zipName;
                auto resp = HttpResponse::newHttpJsonResponse(json);
                callbackCopy(resp);
            } else {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k500InternalServerError);
                resp->setBody("Zip creation failed");
                callbackCopy(resp);
            }
        });
}
