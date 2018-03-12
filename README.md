# WolkGateway

WolkGateway bridges communication between WolkAbout IoT platform and multiple devices connected to it.

WolkGateway supports bridging of devices using following protocol(s):

* Json protocol

Prerequisite
------

Following tools are required in order to build WolkGateway

* CMake - version 3.5 or later
* Conan
* Mosquitto MQTT broker
* Optional: Docker CE (For running WolkGateway inside Docker container)

Former can be installed on Debian based system from terminal by invoking:

`apt-get install mosquitto cmake python && pip install conan`

Building
------

Before proceeding with steps from this section complete steps listed in 'Prerequisite' section.

1. Generate build system by invoking `./configure` from within WolkGateway base directory.
Build system is generated to `out` directory
2. Change current directory to `out`. Following steps are performed from within this directory
3. Build WolkGateway by invoking `make all -j6`
4. Run WolkGateway tests by invoking `tests`

Running
------

Before proceeding with steps from this section complete steps listed in 'Building' section.

1. Change current directory to `out`
2. Open `gatewayConfiguration.json` and fill name, key and password fields with data provided by WolkAbout IoT platform after gateway device creation
3. Make sure mosquitto is running by invoking `systemctl start mosquitto`
4. Run gateway by invoking `./WolkGatewayApp gatewayConfiguration.json`

Running WolkGateway inside Docker container
------

Before proceeding with steps from this section complete steps listed in 'Building' section.

1. Open `out/gatewayConfiguration.json` and fill name, key and password fields with data provided by WolkAbout IoT platform after gateway device creation
2. Change current directory to `Docker`. Following steps are performed from within this directory
3. Build WolkGateway Docker image by invoking `build_gateway.sh`
4. Build Mosquitto Docker image by invoking `build_mosquitto.sh`
5. Create, and start, Mosquitto container by executing `start_mosquitto.sh`
6. Create, and start, WolkGateway container by executing `start_gateway.sh`. Afterward WolkGateway should be started with `docker start ...` and **not** via this shell script
