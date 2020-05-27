#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

./build-image-armv7l.sh

docker container stop debuilder
docker container rm debuilder

branch=$(git rev-parse --abbrev-ref HEAD)
if [ $? -eq 1 ]
then
  branch=master
fi

docker run -dit --name debuilder --cpus $(nproc) wolkabout:wg-armv7l || exit
docker exec -it debuilder /build/make_deb.sh $branch || exit
docker cp debuilder:/build/ .

docker container stop debuilder
docker container rm debuilder

mv ./build/*.deb .
rm -rf ./build/

rm *dbgsym*

chown "$USER:$USER" *.deb
