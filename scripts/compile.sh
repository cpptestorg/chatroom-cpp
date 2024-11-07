#!/bin/bash

#compile
g++ ./src/server.cpp -lpthread -o ./build/server
g++ ./src/client.cpp -lpthread -o ./build/client