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

if [ "$EUID" -ne 0 ]; then
  echo "Please run as sudo."
  exit
fi

./build-image-arm64.sh

docker container stop debuilder
docker container rm debuilder

branch=$(git rev-parse --abbrev-ref HEAD)
if [ $? -eq 1 ]; then
  branch=master
fi

docker run -dit --name debuilder --cpus $(nproc) wolkabout:wg-arm64 || exit
docker exec -it debuilder /build/make_deb.sh $branch || exit
docker cp debuilder:/build/ .

docker container stop debuilder
docker container rm debuilder

mv ./build/*.deb .
rm -rf ./build/

rm *dbgsym*

chown "$USER:$USER" *.deb
