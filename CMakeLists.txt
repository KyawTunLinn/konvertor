cmake_minimum_required(VERSION 3.5)
project(drogon_static_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Security & Optimization Flags
if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(
        -O3 
        -flto
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -Wformat 
        -Werror=format-security
    )
    add_link_options(-flto)
endif()

# MySQL Detection Fix for Debian/Ubuntu
find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS /usr/include/mysql /usr/local/include/mysql)
    
find_library(MYSQL_LIB
    NAMES mysqlclient libmysqlclient
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib/mysql /usr/local/lib/mysql)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIB)
    set(MYSQL_FOUND TRUE)
    set(MYSQL_INCLUDE_DIRS ${MYSQL_INCLUDE_DIR})
    set(MYSQL_LIBRARIES ${MYSQL_LIB})
    message(STATUS "Found MySQL manually: ${MYSQL_LIB}")
else()
    message(WARNING "MySQL not found manually, relying on Drogon's finder...")
endif()

# Find Drogon
find_package(Drogon REQUIRED)

# Include directories
include_directories(${DROGON_INCLUDE_DIRS})
include_directories(include)

# Link directories
link_directories(${DROGON_LIB_DIRS})

# Source files
add_executable(konvertor 
    src/main.cc 
    src/controllers/StaticFileController.cc
    src/services/ConversionManager.cc
    src/services/RateLimiter.cc
    src/controllers/ConverterController.cc
    src/controllers/StatsController.cc
)

target_link_libraries(konvertor
    ${DROGON_LIBRARIES}
    jsoncpp
    uuid
)
