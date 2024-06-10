#!/bin/sh

program_name="$(basename $(pwd))"
version=0.0.6
os=$(uname)
arch=$(uname -m)

build_libs() {
    lib_dir="build/$1/lib"

    if ! [ -f "$lib_dir/termbox2.o" ]; then
        gcc -c -std=c99 -o $lib_dir/termbox2.o libs/termbox2.c
    fi
    if ! [ -f "$lib_dir/argparse.o" ]; then
        g++ -c -std=c++17 -o $lib_dir/argparse.o libs/argparse.cpp
    fi
    if ! [ -f "$lib_dir/httplib.o" ]; then
        g++ -c -std=c++17 -o $lib_dir/httplib.o libs/httplib.cpp
    fi

    if [ $? -ne 0 ]; then
        echo "Failed to compile libs"; exit 1
    fi
}

mkdir -p build

if [ "$1" = "-r" ]; then
    # Build for release
    rm -rf build/release/bin 2> /dev/null || true && mkdir -p build/release
    mkdir -p build/release/{bin,lib}

    build_libs "release"
    g++ -c -std=c++17 -o build/release/lib/main.o main.cpp

    if [ $? -ne 0 ]; then
        echo "Failed to compile main"; exit 1
    fi

    if [ "$os" = "Darwin" ]; then
        g++ -O3 -std=c++17 \
            -o build/release/bin/$program_name \
            build/release/lib/*.o \
            -lssl -lcrypto -framework CoreFoundation -framework Security
    else
        # Copy static libs
        cp -L /usr/lib/x86_64-linux-gnu/{libssl,libcrypto}.a build/release/lib

        if [ $? -ne 0 ]; then
            echo "Failed to copy static libs"; exit 1
        fi

        g++ -O3 -std=c++17 -static-libgcc -static-libstdc++ \
            -pthread \
            build/release/lib/*.o \
            -L build/release/lib -l:libssl.a -l:libcrypto.a -ldl \
            -o build/release/bin/$program_name
    fi

    if [ $? -ne 0 ]; then
        echo "Failed to compile"; exit 1
    else
        tar -C build/release/bin \
            -czvf build/release/bin/${program_name}-v$version-${os,}-$arch.tar.gz \
            $program_name > /dev/null
    fi
elif [ "$1" = "-d" ]; then
    # Build for debug
    rm -rf build/debug/bin 2> /dev/null || true && mkdir -p build/debug
    mkdir -p build/debug/{bin,lib}

    build_libs "debug"
    g++ -c -std=c++17 -o build/debug/lib/main.o main.cpp

    if [ $? -ne 0 ]; then
        echo "Failed to compile main"; exit 1
    fi

    if [ "$os" = "Darwin" ]; then
        g++ -g -std=c++17 \
            -o build/debug/bin/$program_name \
            build/debug/lib/*.o \
            -lssl -lcrypto -framework CoreFoundation -framework Security
    else
        g++ -g -std=c++17 \
            -pthread \
            -o build/debug/bin/$program_name \
            build/debug/lib/*.o \
            -lssl -lcrypto
    fi
else
    echo "Usage: $0 [-r] [-d]"
    exit 1
fi
