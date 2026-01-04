#!/bin/bash
cd ./registration
rm -rf ./build/*
cmake -B ./build
make -C ./build

cd ..
npm install
node server.js
