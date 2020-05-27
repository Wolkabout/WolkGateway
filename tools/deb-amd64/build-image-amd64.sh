#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

cp ../make_deb.sh .

docker build -t wolkabout:wg-amd64 .
rm make_deb.sh
