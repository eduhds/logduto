#!/bin/sh

mkdir -p out

rm -rf out/* 2> /dev/null || true

program_name="$(basename $(pwd))"

g++ -std=c++17 -o out/$program_name *.cpp
