#include "controllers/StaticFileController.h"
#include <drogon/utils/Utilities.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

// The main handler for incoming HTTP requests matching our regex
void StaticFileController::asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                                  std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                                                  std::string path)
{
    // ==========================================
    // 1. Load Configuration
    // ==========================================
    // Fetch the "static_server" configuraton block from config.json
    auto config = drogon::app().getCustomConfig()["static_server"];
    
    // Default values if config is missing (Fallback safety)
    std::string rootPath = config.get("root_path", "./www").asString();
    std::string indexPage = config.get("index_page", "index.html").asString();
    std::string error404Page = config.get("error_404_page", "404.html").asString();
    std::string cacheControl = config.get("cache_control", "no-store, no-cache, must-revalidate, max-age=0").asString();

    // ==========================================
    // 2. Request Sanitization & Routing
    // ==========================================
    
    // If path is empty or root "/", serve the configured index page (e.g., index.html)
    if (path.empty() || path == "/") {
        path = indexPage;
    }

    // Construct the full absolute path on the filesystem
    // We combine the configured root path with the requested request path
    fs::path fullPath = fs::path(rootPath) / path;
    
    // ==========================================
    // 3. Security Checks
    // ==========================================
    
    // Enhanced Directory Traversal Prevention:
    // Use canonical paths to prevent sophisticated attacks
    if (path.find("..") != std::string::npos) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        resp->setBody("Forbidden: Invalid path detected");
        callback(resp);
        return;
    }
    
    // Additional check: Verify canonical path is within root
    try {
        fs::path canonicalRoot = fs::canonical(rootPath);
        
        // Only canonicalize if the path exists, otherwise check parent
        if (fs::exists(fullPath)) {
            fs::path canonicalPath = fs::canonical(fullPath);
            
            // Check if canonical path starts with canonical root
            auto rootStr = canonicalRoot.string();
            auto pathStr = canonicalPath.string();
            
            if (pathStr.find(rootStr) != 0) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                resp->setBody("Forbidden: Path outside allowed directory");
                callback(resp);
                return;
            }
        }
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR << "Path validation error: " << e.what();
        // Continue with basic checks if canonical fails
    }

    // ==========================================
    // 4. Directory Handling (Index Implementation)
    // ==========================================
    
    // If exact path is a directory (e.g. /docs/), try to serve the index page inside it
    if (fs::exists(fullPath) && fs::is_directory(fullPath)) {
        fullPath /= indexPage;
    }

    // ==========================================
    // 5. File Existence Check & 404 Handling
    // ==========================================
    
    // Check if the final resolved path exists and is a regular file
    if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        
        // Try to serve the configured custom 404 page
        fs::path errorPagePath = fs::path(rootPath) / error404Page;
        
        if (fs::exists(errorPagePath)) {
             // Read the custom 404 HTML file content
             std::ifstream ifs(errorPagePath);
             std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
             resp->setBody(content);
        } else {
             // Fallback if custom 404 page is also missing
             resp->setBody("404 Not Found (Custom error page missing)");
        }
        callback(resp);
        return;
    }

    // ==========================================
    // 6. Serve File & Add Headers
    // ==========================================

    // Use Drogon's optimized newFileResponse (uses sendfile for zero-copy performance)
    auto resp = drogon::HttpResponse::newFileResponse(fullPath.string());
    
    // Add configured Cache-Control header to improve client-side performance
    resp->addHeader("Cache-Control", cacheControl);
    
    // Return the response to the client
    callback(resp);
}

// Authorization check (placeholder for future expansion)
bool StaticFileController::isPathAllowed(const std::string& path) const {
    return true;
}

// Mime type detection (handled automatically by Drogon internally)
std::string StaticFileController::getMimeType(const std::string& path) const {
    return "";
}
