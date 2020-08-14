#!/bin/bash -e
#
#  http://free-random.cn
#  

echo
echo This script will install APie and all its dependencies.
echo It has been tested on CentOS 7 64位.
echo

set -e   # Exit immediately if a simple command exits with a non-zero status.
set -x   # activate debugging from here

BASE_DIR="$(cd "$(dirname -- "$0")" ; pwd)"
echo "CurDir: $BASE_DIR"


yaml:

mkdir build
cd build

cmake ..
make

make test
make install