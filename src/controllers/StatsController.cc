/*
 * Copyright (C) 2026 Kyaw Tun Linn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "StatsController.h"
#include "../services/ConversionManager.h"

void StatsController::getStats(const HttpRequestPtr& req,
                               std::function<void (const HttpResponsePtr &)> &&callback)
{
    Json::Value json;
    // We will implement getTotalConversions in ConversionManager
    json["total_conversions"] = (Json::UInt64)ConversionManager::instance().getTotalConversions();
    
    // Optional: Add active tasks count if available
    // json["active_tasks"] = ...
    
    auto resp = HttpResponse::newHttpJsonResponse(json);
    callback(resp);
}
