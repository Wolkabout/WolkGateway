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

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

filename="release-v$(cat ../RELEASE_NOTES.txt | grep "**Version" | head -1 | sed -e "s/**Version //" | sed -e "s/\*\*//")"
mkdir -p ./"$filename"

./make_zip.sh
mv *.zip "$filename"

cd ./deb-amd64 || exit
./run-amd64.sh
mv *.deb ../"$filename"/
cd ..

cd ./deb-armv7l || exit
./run-armv7l.sh
mv *.deb ../"$filename"/
cd ..

cd "$filename" || exit
chown "$USER:$USER" *
