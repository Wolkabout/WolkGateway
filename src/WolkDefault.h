#ifndef WOLKABOUT_WOLKDEFAULT_H
#define WOLKABOUT_WOLKDEFAULT_H

#include "Wolk.h"

namespace wolkabout
{
class DeviceStatusService;

class WolkDefault : public Wolk
{
    friend class WolkBuilder;

public:
    ~WolkDefault();

    void connect() override;
    void disconnect() override;

private:
    explicit WolkDefault(GatewayDevice device);

    void deviceRegistered(const std::string& deviceKey);
    void deviceUpdated(const std::string& deviceKey);

    void devicesDisconnected();

    void notifyDevicesConnected();
    void notifyDevicesDisonnected();

    void connectToDevices(bool firstTime = false);

    std::unique_ptr<ConnectivityService> m_deviceConnectivityService;
    std::unique_ptr<InboundDeviceMessageHandler> m_inboundDeviceMessageHandler;
    std::unique_ptr<PublishingService> m_devicePublisher;

    std::unique_ptr<DeviceStatusService> m_deviceStatusService;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_WOLKDEFAULT_H
