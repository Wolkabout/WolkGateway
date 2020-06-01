#!/bin/bash

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
