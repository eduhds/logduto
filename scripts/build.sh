#!/bin/sh

program_name="$(basename $(pwd))"

mkdir -p build

if [ "$1" = "-r" ]; then
    # Build for release
    rm -rf build/release 2> /dev/null || true && mkdir -p build/release
    mkdir build/release/{bin,lib}

    # Copy libs
    cp -L /usr/lib/x86_64-linux-gnu/{libssl,libcrypto}.so.* build/release/lib

    if [ $? -ne 0 ]; then
        echo "Failed to copy libs"; exit 1
    fi

    # TODO: g++ -O3 -sg++ ...
    g++ -std=c++17 -static-libgcc -static-libstdc++ \
        -pthread -Wl,-rpath,\$ORIGIN/../lib/ \
        -o build/release/bin/$program_name *.cpp \
        -L./build/release/lib -lssl -lcrypto
elif [ "$1" = "-d" ]; then
    # Build for debug
    rm -rf build/debug 2> /dev/null || true && mkdir -p build/debug
    mkdir build/debug && mkdir build/debug/bin

    if [ "$(uname)" = "Darwin" ]; then
        g++ -g -std=c++17 \
            -o build/debug/bin/$program_name *.cpp \
            -lssl -lcrypto -framework CoreFoundation -framework Security
    else
        g++ -g -std=c++17 \
            -pthread \
            -o build/debug/bin/$program_name *.cpp \
            -lssl -lcrypto
    fi
else
    echo "Usage: $0 [-r] [-d]"
    exit 1
fi
