```
██╗    ██╗ ██████╗ ██╗     ██╗  ██╗ ██████╗  █████╗ ████████╗███████╗██╗    ██╗ █████╗ ██╗   ██╗
██║    ██║██╔═══██╗██║     ██║ ██╔╝██╔════╝ ██╔══██╗╚══██╔══╝██╔════╝██║    ██║██╔══██╗╚██╗ ██╔╝
██║ █╗ ██║██║   ██║██║     █████╔╝ ██║  ███╗███████║   ██║   █████╗  ██║ █╗ ██║███████║ ╚████╔╝ 
██║███╗██║██║   ██║██║     ██╔═██╗ ██║   ██║██╔══██║   ██║   ██╔══╝  ██║███╗██║██╔══██║  ╚██╔╝  
╚███╔███╔╝╚██████╔╝███████╗██║  ██╗╚██████╔╝██║  ██║   ██║   ███████╗╚███╔███╔╝██║  ██║   ██║   
 ╚══╝╚══╝  ╚═════╝ ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝ ╚══╝╚══╝ ╚═╝  ╚═╝   ╚═╝   
                                                                                                  
```
----
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

`apt-get install mosquitto cmake python python-pip && python -m pip install conan`

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
2. Open `gatewayConfiguration.json` and fill name, key and password fields with data provided by WolkAbout IoT platform after gateway device creation and choose who will be responsible for registering and deleting devices - gateway or platform
3. Make sure mosquitto is running by invoking `systemctl start mosquitto`
4. Run gateway by invoking `./WolkGatewayApp gatewayConfiguration.json`

**Note:** Running additional instances of WolkGateway on the same network requires having an additional mosquitto broker per gateway. Start a mosquitto daemon from the terminal with `mosquitto -p <port> -d`. The port entered here should also be entered into `gatewayConfiguration.json` for the matching gateway and into the configuration file of all of the gateway's modules. 

Connecting devices
------

Devices are connected to the gateway using modules. Module must implement communication protocol for specific device and manage
all communication between devices and the gateway including sending device data and registering devices to WolkAbout platform.

Module communicates with gateway via local mqtt broker which is run along with gateway.

A single Module may handle multiple devices, and there may be multiple modules deployed.

[WolkGatewayModule-Cpp](https://github.com/Wolkabout/WolkGatewayModule-Cpp) is a SDK which implements data handling and device registration with the gateway,
and the user needs to implement communication with the devices.

Module may be implemented in any desired language as a mqtt client.

<p align="center">
  <img src="gateway_architecture.png" title="Gateway architecture">
</p>


Deleting devices
------

If subdevice management was set to `platform` in `gatewayConfiguration.json`, then the deletion of devices is enabled on the web application and the gateway stores no information about registered devices, it simply forwards all received data to the platform.

If subdevice management was set to `gateway` in `gatewayConfiguration.json`, then the gateway keeps track of all devices that are registered via modules. Keys of all registered devices are stored in a file `existingDevices.json` which is located in the same directory as gateway executable.

To delete device from gateway open `existingDevices.json` file and remove the line containing the key of device which should be deleted, and restart gateway.
Upon starting the gateway will delete all registered devices whose keys no longer appear in `existingDevices.json` file.

Note that the device will be registered again if the gateway receives registration request for that device from module.


Advanced usage
------

Gateway can have its own feeds, configuration and firmware update.

**Specifying gateway template**

```cpp
    std::map<std::string, std::string> typeParameters;
    typeParameters.insert(std::pair<std::string, std::string>("subdeviceManagement", "GATEWAY"));

    wolkabout::DeviceTemplate gatewayTemplate(
        {wolkabout::ConfigurationTemplate("Heating start hour", "HSH", wolkabout::DataType::NUMERIC, "", "8", 0, 24)},
        {wolkabout::SensorTemplate("Temperature", "T", wolkabout::DataType::NUMERIC, "", -20, 70)},
        {wolkabout::AlarmTemplate("Humidity alarm", "HH", "")},
        {wolkabout::ActuatorTemplate("Light switch", "SW", wolkabout::DataType::BOOLEAN, "")},
        "",
        typeParameters,
        {},
        {});


    wolkabout::GatewayDevice device(gatewayConfiguration.getKey(), gatewayConfiguration.getPassword(), gatewayTemplate);

    auto builder = wolkabout::Wolk::newBuilder(device)
    	.gatewayHost(gatewayConfiguration.getLocalMqttUri())
    	.platformHost(gatewayConfiguration.getPlatformMqttUri())
        .build();

        wolk->connect();
```

**Specifying actuation and configuration handlers/providers**
```cpp

auto builder = wolkabout::Wolk::newBuilder(device)
	.gatewayHost(gatewayConfiguration.getLocalMqttUri())
	.platformHost(gatewayConfiguration.getPlatformMqttUri())
    .actuationHandler([](const std::string& reference, const std::string& value) -> void {
        // TODO Invoke your code which activates your actuator.

        std::cout << "Actuation request received - Reference: " << reference << " value: " << value << std::endl;
    })
    .actuatorStatusProvider([](const std::string& reference) -> wolkabout::ActuatorStatus {
        // TODO Invoke code which reads the state of the actuator.

        if (reference == "ACTUATOR_REFERENCE_ONE") {
            return wolkabout::ActuatorStatus("65", wolkabout::ActuatorStatus::State::READY);
        } else if (reference == "ACTUATOR_REFERENCE_TWO") {
            return wolkabout::ActuatorStatus("false", wolkabout::ActuatorStatus::State::READY);
        }

        return wolkabout::ActuatorStatus("", wolkabout::ActuatorStatus::State::READY);
    })
    .configurationHandler([](const std::map<std::string, std::string>& configuration) -> void {
        // TODO invoke code which sets gateway configuration
    })
    .configurationProvider([]() -> const std::map<std::string, std::string>& {
        // TODO invoke code which reads gateway configuration
        return std::map<std::string, std::string>();
    })
    .build();

    wolk->connect();
```

**Publishing sensor readings**
```cpp
wolk->addSensorReading("TEMPERATURE_REF", 23.4);
wolk->addSensorReading("BOOL_SENSOR_REF", true);
```

**Publishing actuator statuses**
```cpp
wolk->publishActuatorStatus("ACTUATOR_REFERENCE_ONE");
```
This will invoke the ActuationStatusProvider to read the actuator status,
and publish actuator status.

**Publish device configuration to platform**
```cpp
wolk->publishConfiguration();
```

**Publishing events**
```cpp
wolk->addAlarm("ALARM_REF", true);
```

**Data publish strategy**

Sensor readings, and alarms are pushed to WolkAbout IoT platform on demand by calling
```cpp
wolk->publish();
```

Whereas actuator statuses are published automatically by calling:

```cpp
wolk->publishActuatorStatus("ACTUATOR_REFERENCE_ONE");
```

**Trust store**

By default Gateway searches for file named 'ca.crt' in the current directory.

See code snippet below on how to specify trust store for gateway.

```c++

wolkabout::GatewayDevice device(gatewayConfiguration.getKey(), gatewayConfiguration.getPassword(),
                                gatewayConfiguration.getSubdeviceManagement());

auto builder = wolkabout::Wolk::newBuilder(device)
	.gatewayHost(gatewayConfiguration.getLocalMqttUri())
	.platformHost(gatewayConfiguration.getPlatformMqttUri())
	// Specify trust store
	.platformTrustStore("path_to_trust_store");
    .build();

    wolk->connect();
```

**Firmware Update**

WolkAbout Gateway provides mechanism for updating gateway and subdevices firmware.

By default this feature is disabled for gateway and enabled for subdevices.
See code snippet below on how to enable device firmware update for gateway.

```c++

class CustomFirmwareInstaller: public wolkabout::FirmwareInstaller
{
public:
	bool install(const std::string& firmwareFile) override
	{
		// Mock install
		std::cout << "Updating firmware with file " << firmwareFile << std::endl;

		// Optionally delete 'firmwareFile
		return true;
	}
};

auto installer = std::make_shared<CustomFirmwareInstaller>();

wolkabout::GatewayDevice device(gatewayConfiguration.getKey(), gatewayConfiguration.getPassword(), 
                                wolkabout::SubdeviceManagement::GATEWAY, true, true);

auto builder = wolkabout::Wolk::newBuilder(device)
	.gatewayHost(gatewayConfiguration.getLocalMqttUri())
	.platformHost(gatewayConfiguration.getPlatformMqttUri())
	// Enable firmware update
	.withFirmwareUpdate("2.1.0",								// Current firmware version of the gateway
						installer,								// Implementation of FirmwareInstaller, which performs installation of obtained gateway firmware
						".",									// Directory where downloaded firmware files will be stored
						10 * 1024 * 1024,						// Maximum acceptable size of firmware file, in bytes
						1024 * 1024,							// Size of firmware file transfer chunk, in bytes
						urlDownloader)							// Optional implementation of UrlFileDownloader for cases when one wants to download device firmware via given URL
    .build();

    wolk->connect();
```

Firmware update for gateway subdevices is enabled by default. To change default settings without enabling firmware update for gateway use the above api
and pass false for firmwareUpdateEnabled in GatewayDevice.
