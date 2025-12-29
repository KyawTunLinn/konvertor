/*
 * Copyright (C) 2026 Kyaw Tun Linn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "RateLimiter.h"
#include <trantor/utils/Logger.h>

bool RateLimiter::isAllowed(const std::string& ipAddress) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& history = ipHistory_[ipAddress];
    cleanup(history);
    
    // Periodic cleanup of stale IPs to prevent memory leak
    // Trigger if map gets too large (e.g. > 1000 entries)
    if (ipHistory_.size() > 1000) {
        cleanupStaleEntries();
    }
    
    // Check if under limit
    if (history.timestamps.size() >= MAX_REQUESTS_PER_WINDOW) {
        LOG_WARN << "Rate limit exceeded for IP: " << ipAddress 
                 << " (" << history.timestamps.size() << " requests in window)";
        return false;
    }
    
    // Add current request timestamp
    history.timestamps.push_back(std::chrono::steady_clock::now());
    
    LOG_DEBUG << "Request allowed for IP: " << ipAddress 
              << " (" << history.timestamps.size() << "/" << MAX_REQUESTS_PER_WINDOW << ")";
    
    return true;
}

void RateLimiter::cleanupStaleEntries() {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - TIME_WINDOW;
    
    for (auto it = ipHistory_.begin(); it != ipHistory_.end(); ) {
        cleanup(it->second); // Remove old timestamps
        
        // If no timestamps left, or all are old (which cleanup handles), remove the entry
        if (it->second.timestamps.empty()) {
            it = ipHistory_.erase(it);
        } else {
            ++it;
        }
    }
}

int RateLimiter::getRemainingRequests(const std::string& ipAddress) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = ipHistory_.find(ipAddress);
    if (it == ipHistory_.end()) {
        return MAX_REQUESTS_PER_WINDOW;
    }
    
    cleanup(it->second);
    return MAX_REQUESTS_PER_WINDOW - static_cast<int>(it->second.timestamps.size());
}

void RateLimiter::cleanup(RequestHistory& history) {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - TIME_WINDOW;
    
    // Remove timestamps older than the time window
    while (!history.timestamps.empty() && history.timestamps.front() < cutoff) {
        history.timestamps.pop_front();
    }
}
