#pragma once

#include <drogon/HttpController.h>
#include <string>

class StaticFileController : public drogon::HttpController<StaticFileController>
{
  public:
    METHOD_LIST_BEGIN
    // Catch-all route to serve files using regex
    ADD_METHOD_VIA_REGEX(StaticFileController::asyncHandleHttpRequest, "/(.*)", drogon::Get);
    METHOD_LIST_END

    void asyncHandleHttpRequest(const drogon::HttpRequestPtr& req,
                                std::function<void (const drogon::HttpResponsePtr &)> &&callback,
                                std::string path);

  private:
    std::string rootPath_ = "./www"; // Default static directory
    
    // Security check: validate path to prevent directory traversal
    bool isPathAllowed(const std::string& path) const;
    
    // Helper to get mime type (optional, Drogon handles this but good for custom logic)
    std::string getMimeType(const std::string& path) const;
};
