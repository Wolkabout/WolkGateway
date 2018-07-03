# WolkGateway

WolkGateway bridges communication between WolkAbout IoT platform and multiple devices connected to it.

WolkGateway supports bridging of devices using following protocol(s):

* Json protocol

Installing from source
----------------------

This repository must be cloned from the command line using:
```sh
git clone --recurse-submodules https://github.com/Wolkabout/WolkGateway.git
```

Prerequisite
------

Following tools are required in order to build WolkGateway

* CMake - version 3.5 or later
* Conan
* Mosquitto MQTT broker
* Python
* Python PIP
* Optional: Docker CE (For running WolkGateway inside Docker container)

Former can be installed on Debian based system from terminal by invoking:

`apt-get install mosquitto cmake python python-pip && pip install conan`

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


<p align="center">
  <img src="gateway_architecture.png" title="Gateway architecture">
</p>
