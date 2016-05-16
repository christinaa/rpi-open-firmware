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