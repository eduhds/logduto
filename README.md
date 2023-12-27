# Logduto

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![macOS](https://img.shields.io/badge/mac%20os-000000?style=for-the-badge&logo=macos&logoColor=F0F0F0)

An unpretentious HTTP request logger.

## Install

Download binary executable from [Releases](https://github.com/eduhds/logduto/releases).

```sh
wget https://github.com/eduhds/logduto/releases/download/0.0.1/Logduto-0.0.1-$(uname).tar.gz

tar -xf Logduto*.tar.gz

chmod +x logduto
```

## Usage

```sh
# With default options
logduto -u <url>

# Specify Logduto server host and/or port
logduto -u <url> -h <host> -p <port>

# Specify directory to save reports
logduto -u <url> -l <path to directory>

# Save request/response as file with content type (like .json)
logduto -u <url> -d
```

## Developement

```sh
# Build debug
sh scripts/build.sh -d

# Build release
sh scripts/build.sh -r
```

## Credits

- [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [p-ranav/argparse](https://github.com/p-ranav/argparse)
