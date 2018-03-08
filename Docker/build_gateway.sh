#!/usr/bin/env bash

pushd ..
tar --exclude='./Docker' -zcvf ./Docker/gateway/gateway.tar.gz .
popd

pushd gateway
docker rmi wolkabout/gateway
docker build -t wolkabout/gateway .
popd
