# <img src="www/logo.png" width="64" height="64" alt="KConvertor Logo" style="vertical-align: middle;"> Drogon Static Web Server (v0.1.0)

A high-performance C++ Video-to-Audio Converter and static web server built using the [Drogon](https://github.com/drogonframework/drogon) framework. This project allows users to upload video files and convert them to various audio formats (MP3, AAC, OGG, etc.) with custom quality settings. It also features a "Download All" (ZIP) capability for batch conversions.

## Project Structure

- `src/`: Source code (`main.cc`, controllers).
- `include/`: Header files.
- `config/`: Configuration files (`config.json`).
- `www/`: Static assets (HTML, CSS, JS) to be served.
- `build/`: Directory for build artifacts.

## Prerequisites

To build this project, you need a C++ development environment and the Drogon framework dependencies.

### Ubuntu / Debian Implementation
Run the following command to install necessary system libraries:
```bash
sudo apt update
sudo apt install -y git cmake g++ \
    libjsoncpp-dev uuid-dev openssl libssl-dev zlib1g-dev \
    libmysqlclient-dev libbrotli-dev \
    ffmpeg zip
```

**Note:** You must also have the **Drogon** framework installed on your system. If you haven't installed it yet, follow the official instructions or build it from source:
```bash
git clone https://github.com/drogonframework/drogon
cd drogon
git submodule update --init
mkdir build
cd build
cmake ..
make && sudo make install
```

## How to Build

1. **Clone the repository** (if applicable) and navigate to the project root.

2. **Create a build directory:**
   ```bash
   mkdir -p build
   ```

3. **Generate Makefiles with CMake:**
   ```bash
   cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```
   *The `CMakeLists.txt` automatically attempts to find MySQL and other libraries.*

4. **Compile the project:**
   ```bash
   make
   ```

## How to Run

1. **Navigate to the build directory:**
   ```bash
   cd build
   ```
   *(Or running from project root using `./build/drogon_static_server`)*

2. **Run the executable:**
   ```bash
   ./build/konvertor
   ```
   *Important: The server expects `config/config.json` to be available. If you run it from the `build` directory, ensure the relative path `../config/config.json` is resolved or copy the config folder.*
   *Currently, `src/main.cc` looks for `config/config.json`. It is best to run the executable from the **project root folder**:*
   ```bash
   # From project root (/root/format-convertor)
   ./build/konvertor
   ```

3. **Access the server:**
   Open [http://localhost:8080](http://localhost:8080).

## Features

- **Static File Serving**: Serves HTML, CSS, JS from `www/`.
- **Directory Indexing**: Automatically serves `index.html` for directory requests.
- **Compression**: Supports Gzip and Brotli (enabled in config).
- **Custom Error Pages**: Serves `www/404.html` for invalid paths.
- **Security**: Basic path traversal prevention.
- **Flexible Configuration**: Fully configurable via `config.json`.

## Configuration

Edit `config/config.json` to change settings. Example structure:

```json
{
    "listeners": [
        {
            "address": "0.0.0.0",
            "port": 8080,
            "https": false
        }
    ],
    "app": {
        "number_of_threads": 4,
        "log_level": "INFO",
        "use_gzip": true,
        "use_brotli": true
    },
    "static_server": {
        "root_path": "./www",
        "index_page": "index.html",
        "error_404_page": "404.html",
        "cache_control": "public, max-age=3600"
    }
}
```

### Static Server Options

| Option | Default | Description |
|--------|---------|-------------|
| `root_path` | `./www` | The root directory containing your static website files. |
| `index_page` | `index.html` | The default file to serve when a directory is requested. |
| `error_404_page` | `404.html` | The custom HTML file to serve when a 404 error occurs. |
| `cache_control` | `public, max-age=3600` | The value for the `Cache-Control` HTTP header. |

## Monitor & Control

- **API Documentation**: Available at `/api_docs.html`.
- **System Service**: For production, create a systemd service file or use a process manager like `pm2` to keep the server running.

## Author

**Kyaw Tun Linn**
- GitHub: [https://github.com/KyawTunLinn](https://github.com/KyawTunLinn)

## License

This project is licensed under the **GNU General Public License v2.0 (GPL-2.0)**.

```
Copyright (C) 2026 Kyaw Tun Linn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
```

