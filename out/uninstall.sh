#!/bin/bash
#
# Copyright 2020 WolkAbout Technology s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

echo "Uninstalling WolkGateway..."
service wolk_gateway stop
echo "Stopped running service"
rm /etc/systemd/system/wolk_gateway.service
echo "Removed service unit file"
systemctl daemon-reload
echo "Systemctl reloaded"
rm -rf /etc/wolkGateway/
echo "Removed configuration files"

for lib in $( ls -1 ./lib | grep *so* )
do
	rm "/usr/lib/$lib"
done
rm /usr/bin/WolkGatewayApp
echo "Removed application files"
echo "Done."
