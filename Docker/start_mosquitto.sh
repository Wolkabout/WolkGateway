#!/usr/bin/env bash

docker run -p1883:1883 --rm --net="host" wolkabout/gateway_mosquitto
