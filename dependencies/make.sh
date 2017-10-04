#!/usr/bin/env bash

# Copyright 2017 WolkAbout Technology s.r.o.
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

mkdir -pv "$BUILD_DIR"

# libssl
if [ ! -f "$LIB_DIR/libssl.so" ]; then
    echo "Building libssl"
    pushd libssl

    ./config enable-egd enable-heartbeats enable-ssl3                          \
             enable-ssl3-method enable-md2 enable-rc5 enable-rfc3779           \
             enable-shared enable-ssl2 enable-weak-ssl-ciphers enable-zlib     \
             enable-zlib-dynamic --prefix=$BUILD_DIR                           \

    make && make install
    popd
fi

# paho.mqtt.c
if [ ! -f "$LIB_DIR/libpaho-mqtt3as.so" ]; then
    echo "Building paho.mqtt.c"
    pushd paho.mqtt.c

    export CFLAGS="-I$BUILD_DIR/include"
    export LDFLAGS="-L$LIB_DIR -Wl,-rpath,./ -lcrypto -Wl,-rpath,./ -lssl"

    make
    mkdir -p $BUILD_DIR/include/mqtt
    cp src/*.h $BUILD_DIR/include/mqtt
    cp -d build/output/*.so* $LIB_DIR

    unset CFLAGS
    unset LDFLAGS

    popd
fi

# paho.mqtt.cpp
if [ ! -f "$LIB_DIR/libpaho-mqttpp3.so" ]; then
    echo "Building libmqttpp"
    pushd paho.mqtt.cpp

    export LIB_DEP_FLAGS="-Wl,-rpath,./ -lpaho-mqtt3as"
    export PAHO_C_INC_DIR=$BUILD_DIR/include/mqtt
    export PAHO_C_LIB_DIR=$LIB_DIR
    make
    unset LIB_DEP_FLAGS
    unset PAHO_C_INC_DIR
    unset PAHO_C_LIB_DIR

    cp -d lib/*.so* $LIB_DIR

    popd
fi

# Copy paho.mqtt.c headers
#mkdir -p $BASE_DIR/../src/mqtt
cp paho.mqtt.c/src/*.h $BASE_DIR/../src/mqtt

# Copy paho.mqtt.cpp headers
mkdir -p $BASE_DIR/../src/mqtt
cp -d paho.mqtt.cpp/src/mqtt/*.h $BASE_DIR/../src/mqtt

# Copy shared libraries
cp $BUILD_DIR/lib/libcrypto.so* $BASE_DIR/../out
cp $BUILD_DIR/lib/libssl.so* $BASE_DIR/../out
cp $BUILD_DIR/lib/libpaho-mqttpp3.so* $BASE_DIR/../out
cp $BUILD_DIR/lib/libpaho-mqtt3as.so* $BASE_DIR/../out

