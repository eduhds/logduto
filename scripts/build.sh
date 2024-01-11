#!/bin/sh

program_name="$(basename $(pwd))"
version=0.0.1

build_libs() {
    gcc -c -std=c99 -o build/debug/lib/termbox2.o libs/termbox2.c
    g++ -c -std=c++17 -o build/debug/lib/argparse.o libs/argparse.cpp
    g++ -c -std=c++17 -o build/debug/lib/httplib.o libs/httplib.cpp
}

mkdir -p build

if [ "$1" = "-r" ]; then
    # Build for release
    rm -rf build/release 2> /dev/null || true && mkdir -p build/release
    mkdir build/release/{bin,lib}

    build_libs
    g++ -c -std=c++17 -o build/debug/lib/main.o main.cpp

    if [ "$(uname)" = "Darwin" ]; then
        g++ -O3 -std=c++17 \
            -o build/release/bin/$program_name \
            build/debug/lib/*.o \
            -lssl -lcrypto -framework CoreFoundation -framework Security
    else
        if [ "$2" = "-s" ]; then
            # Copy static libs
            cp -L /usr/lib/x86_64-linux-gnu/{libssl,libcrypto}.a build/release/lib

            if [ $? -ne 0 ]; then
                echo "Failed to copy static libs"; exit 1
            fi

            g++ -O3 -std=c++17 -static-libgcc -static-libstdc++ \
                -pthread \
                build/debug/lib/*.o \
                -L build/release/lib -l:libssl.a -l:libcrypto.a -ldl \
                -o build/release/bin/$program_name
        else
            # Copy dynamic libs
            cp -L /usr/lib/x86_64-linux-gnu/{libssl,libcrypto}.so.* build/release/lib

            if [ $? -ne 0 ]; then
                echo "Failed to copy dynamic libs"; exit 1
            fi

            g++ -O3 -std=c++17 -static-libgcc -static-libstdc++ \
                -pthread -Wl,-rpath,\$ORIGIN/../lib/ \
                -o build/release/bin/$program_name *.cpp \
                -L./build/release/lib -lssl -lcrypto
        fi
    fi

    if [ $? -ne 0 ]; then
        echo "Failed to compile"; exit 1
    else
        tar -C build/release/bin \
            -czvf build/release/bin/$program_name-$version-$(uname).tar.gz \
            $program_name > /dev/null
    fi
elif [ "$1" = "-d" ]; then
    # Build for debug
    rm -rf build/debug 2> /dev/null || true && mkdir -p build/debug
    mkdir -p build/debug/{bin,lib}

    build_libs
    g++ -c -std=c++17 -o build/debug/lib/main.o main.cpp

    if [ "$(uname)" = "Darwin" ]; then
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
    echo "Usage: $0 [-r] [-d] [-s]"
    exit 1
fi
