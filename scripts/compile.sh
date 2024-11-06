#!/bin/bash

#compile
g++ ./src/server.cpp -lpthread -o ./src/server
g++ ./src/client.cpp -lpthread -o ./src/client