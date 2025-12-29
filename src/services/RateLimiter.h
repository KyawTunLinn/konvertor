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
#include <unordered_map>
#include <deque>
#include <mutex>
#include <chrono>

/**
 * @class RateLimiter
 * @brief Service to prevent abuse by limiting requests per IP address.
 * 
 * Implements a specific limit (e.g., 10 requests per hour) using a sliding
 * window algorithm stored in memory. It is thread-safe using mutex protection.
 */
class RateLimiter {
public:
    static RateLimiter& instance() {
        static RateLimiter inst;
        return inst;
    }

    /**
     * @brief Checks if an IP is allowed to make a request.
     * @param ipAddress The client's IP address.
     * @return true if allowed, false if limit exceeded.
     */
    bool isAllowed(const std::string& ipAddress);
    
    /**
     * @brief Gets the number of remaining requests for an IP in the current window.
     */
    int getRemainingRequests(const std::string& ipAddress);

private:
    RateLimiter() = default;
    ~RateLimiter() = default;
    RateLimiter(const RateLimiter&) = delete;
    RateLimiter& operator=(const RateLimiter&) = delete;

    struct RequestHistory {
        std::deque<std::chrono::steady_clock::time_point> timestamps;
    };

    std::unordered_map<std::string, RequestHistory> ipHistory_;
    std::mutex mutex_;
    
    // Configuration
    const int MAX_REQUESTS_PER_WINDOW = 10; // 10 requests
    const std::chrono::minutes TIME_WINDOW{60}; // per hour
    
    // Remove timestamps that are older than the time window
    void cleanup(RequestHistory& history);
    // Remove IP entries that have no active history
    void cleanupStaleEntries();
};
