#!/bin/bash

if [ $(dpkg -l | grep -c devscripts) -lt 1 ]; then
  if [ "$EUID" -ne 0 ]; then
    echo "Please run as sudo since you're missing the package 'devscripts'. Install it with 'apt install devscripts' or rerun the script as sudo."
    exit
  fi

  apt install -y devscripts
fi

if [ $(dpkg -l | grep -c debhelper) -lt 1 ]; then
  if [ "$EUID" -ne 0 ]; then
    echo "Please run as sudo since you're missing the package 'debhelper'. Install it with 'apt install debhelper' or rerun the script as sudo."
    exit
  fi

  apt install -y debhelper
fi

cd ./WolkGateway || exit

debuild -us -uc -b -j$(nproc)
