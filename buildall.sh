#!/bin/bash

make clean
cd arm_chainloader
make clean
cd ..

set -e

echo "-----------------------------------------"
echo "Building chainloader ..."
echo "-----------------------------------------"
cd arm_chainloader
make
echo "-----------------------------------------"
echo "Building firmware ..."
echo "-----------------------------------------"
cd ..
make

# stage through WSL
if [ "$1" = "sw" ]; then
	tools/wslstage.py
fi