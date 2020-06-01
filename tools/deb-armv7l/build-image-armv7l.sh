#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

sudo apt-get install qemu binfmt-support qemu-user-static
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes

cp ../make_deb.sh .

docker build -t wolkabout:wg-armv7l .
rm make_deb.sh
