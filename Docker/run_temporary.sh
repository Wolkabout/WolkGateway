#!/usr/bin/env bash

docker build -t wolkabout/gateway .
docker run -p1883:1883 --rm wolkabout/gateway
docker rmi wolkabout/gateway

