#!/bin/bash
# This script is used to make a release .zip file for WolkGatewayModule-Modbus release

if [ $# -eq 1 ]
then
  branch=$1
else
  branch=$(git rev-parse --abbrev-ref HEAD)

  if [ "$branch" == "" ]
  then
    echo "You must specify a branch as parameter to the script (if the script is not part of a repo)"
    exit
  fi
fi

if [ $(dpkg -l | grep -c zip) -lt 1 ]
then
  if [ "$EUID" -ne 0 ]
  then
    echo "Please run as sudo since you're missing the package 'zip'. Install it with 'apt install zip' or rerun the script as sudo."
    exit
  fi

  apt install -y zip
fi

mkdir -p ./tmp-wg
cd ./tmp-wg || exit
git clone https://github.com/Wolkabout/WolkGateway --recurse-submodules
cd ./WolkGateway || exit
git checkout "$branch"
if [ $? -ne 0 ]
then
  echo "Can't checkout to branch named $branch"
  exit
fi
filename="WolkGateway-v$(cat RELEASE_NOTES.txt | grep "**Version" | head -1 | sed -e "s/**Version //" | sed -e "s/\*\*//").zip"
echo "filename: $filename"
zip -qr $filename *
mv "$filename" ../..
cd ../..
rm -rf ./tmp-wg
