/*
 * Copyright (C) 2026 Kyaw Tun Linn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#pragma once

#include <drogon/HttpController.h>
#include <trantor/utils/Logger.h>
#include <string>

using namespace drogon;

/**
 * @class ConverterController
 * @brief Controller responsible for handling format conversion requests.
 * 
 * This controller serves two main endpoints:
 * - /api/convert: Accepts video files and converts them to audio.
 * - /api/zip: Bundles converted files into a ZIP archive.
 */
class ConverterController : public drogon::HttpController<ConverterController>
{
  public:
    METHOD_LIST_BEGIN
    // Register the conversion endpoint: POST /api/convert
    ADD_METHOD_TO(ConverterController::convert, "/api/convert", Post);
    // Register the batch download endpoint: POST /api/zip
    ADD_METHOD_TO(ConverterController::createZip, "/api/zip", Post);
    METHOD_LIST_END

    /**
     * @brief Handles file upload and triggers async conversion.
     * @param req The HTTP request containing the multipart file upload.
     * @param callback Callback to return the HTTP response.
     */
    void convert(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback);
                 
    /**
     * @brief Creates a ZIP archive of specified files.
     * @param req The HTTP request containing the JSON list of files.
     * @param callback Callback to return the HTTP response.
     */
    void createZip(const HttpRequestPtr &req,
                   std::function<void(const HttpResponsePtr &)> &&callback);
};
