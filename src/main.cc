/*
 * Copyright (C) 2026 Kyaw Tun Linn
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

/**
 * @file main.cc
 * @brief Entry point for the KConvertor Application.
 * 
 * This file initializes the Drogon framework, loads the configuration,
 * and starts the main event loop.
 */

#include <drogon/drogon.h>

int main() {
    // Load configuration from local JSON file.
    // This sets listener ports, thread counts, and upload limits.
    drogon::app().loadConfigFile("config/config.json");
    
    // Start the Drogon HTTP framework event loop.
    // This call blocks until the server is stopped.
    drogon::app().run();
    return 0;
}
