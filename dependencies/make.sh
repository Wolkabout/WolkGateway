#!/usr/bin/env bash

# Copyright 2018 WolkAbout Technology s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

BASE_DIR="$(pwd)"

BUILD_DIR="$BASE_DIR/build"
LIB_DIR="$BUILD_DIR/lib"
DEPLOY_DIR="$BASE_DIR/../out/lib"

LIB_EXTENSION="so"

if [[ `uname` == "Darwin" ]]; then
    LIB_EXTENSION="dylib"
fi

mkdir -pv "$BUILD_DIR"

# libssl
if [ ! -f "$LIB_DIR/libssl.$LIB_EXTENSION" ]; then
    echo "Building libssl"
    pushd libssl

    ./config enable-egd enable-heartbeats enable-ssl3                          \
             enable-ssl3-method enable-md2 enable-rc5 enable-rfc3779           \
             enable-shared enable-ssl2 enable-weak-ssl-ciphers enable-zlib     \
             enable-zlib-dynamic --prefix=$BUILD_DIR                           \

    make -j8 && make install
    popd
fi

# Copy shared libraries
mkdir -p $DEPLOY_DIR
cp $BUILD_DIR/lib/libcrypto.*$LIB_EXTENSION* $DEPLOY_DIR
cp $BUILD_DIR/lib/libssl.*$LIB_EXTENSION* $DEPLOY_DIR
