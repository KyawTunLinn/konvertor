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

using namespace drogon;

class StatsController : public drogon::HttpController<StatsController>
{
  public:
    METHOD_LIST_BEGIN
        // Map /api/stats to getStats method
        ADD_METHOD_TO(StatsController::getStats, "/api/stats", Get);
    METHOD_LIST_END

    void getStats(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback);
};
