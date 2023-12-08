#!/bin/sh

program_name="$(basename $(pwd))"

if [ "$1" = "-r" ]; then
    # Build for release
    mkdir -p build
    rm -rf build/release 2> /dev/null || true && mkdir -p build/release

    # TODO: g++ -O3 -sg++ ...

    if [ $? -ne 0 ]; then
        exit 1
    fi
elif [ "$1" = "-d" ]; then
    # Build for debug
    mkdir -p build
    rm -rf build/debug 2> /dev/null || true && mkdir -p build/debug

    if [ "$(uname)" = "Darwin" ]; then
        g++ -g -std=c++17 \
            -o build/debug/$program_name *.cpp \
            -lssl -lcrypto -framework CoreFoundation -framework Security
    else
        g++ -g -std=c++17 \
            -pthread \
            -o build/debug/$program_name *.cpp \
            -lssl -lcrypto
    fi
else
    echo "Usage: $0 [-r] [-d]"
    exit 1
fi
