#!/usr/bin/env bash

pushd mosquitto
docker rmi wolkabout/gateway_mosquitto
docker build -t wolkabout/gateway_mosquitto .
popd
