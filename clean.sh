#!/bin/bash

cd ./src
make clean
rm -f *.ur-safe
cd ..

if [ -d "./build" ]; then
    rm -rf ./build
fi

if [ -d "./bin" ]; then
    rm -rf ./bin
fi

if [ -d "./lib" ]; then
    rm -rf ./lib
fi

if [ -d "./kmod" ]; then
    rm -rf ./kmod
fi
