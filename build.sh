#!/bin/bash

if [ ! -d "./build" ]; then
    mkdir ./build
fi

if [ ! -d "./kmod" ]; then
    mkdir ./kmod
fi

cd ./src
make
if [ $? -eq 0 ]; then
    cp ./memqueue.ko ../kmod/
fi

if [ $? -eq 0 ]; then
    cd ../build/
    cmake ..
fi

if [ $? -eq 0 ]; then
    make
fi

if [ $? -eq 0 ]; then
    make test
fi
