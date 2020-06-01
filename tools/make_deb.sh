#!/bin/bash
#
#  Copyright 2020 WolkAbout Technology s.r.o.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# This script is used to make a release .deb file for WolkGateway release

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

if [ $(dpkg -l | grep -c devscripts) -lt 1 ]
then
  if [ "$EUID" -ne 0 ]
  then
    echo "Please run as sudo since you're missing the package 'devscripts'. Install it with 'apt install devscripts' or rerun the script as sudo."
    exit
  fi

  apt install -y devscripts
fi

if [ $(dpkg -l | grep -c debhelper) -lt 1 ]
then
  if [ "$EUID" -ne 0 ]
  then
    echo "Please run as sudo since you're missing the package 'debhelper'. Install it with 'apt install debhelper' or rerun the script as sudo."
    exit
  fi

  apt install -y debhelper
fi

rm -rf ./tmp-wg-deb
mkdir -p ./tmp-wg-deb
cd ./tmp-wg-deb || exit
git clone https://github.com/Wolkabout/WolkGateway --recurse-submodules
cd ./WolkGateway || exit
git checkout "$branch"
if [ $? -ne 0 ]
then
  echo "Can't checkout to branch named $branch"
  exit
fi
git submodule update
ls
debuild -us -uc -b -j$(nproc)
cd ../
mv *.deb ..
cd ../
rm -rf ./tmp-wg-deb
