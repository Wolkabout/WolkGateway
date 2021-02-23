#ifndef WOLKABOUT_INTERNALDATASERVICE_H
#define WOLKABOUT_INTERNALDATASERVICE_H

#include "DataService.h"
#include "InboundDeviceMessageHandler.h"

namespace wolkabout
{
class DeviceRepository;

class InternalDataService : public DataService, public DeviceMessageListener
{
public:
    InternalDataService(const std::string& gatewayKey, DataProtocol& protocol, GatewayDataProtocol& gatewayProtocol,
                        DeviceRepository* deviceRepository, OutboundMessageHandler& outboundPlatformMessageHandler,
                        OutboundMessageHandler& outboundDeviceMessageHandler, MessageListener* gatewayDevice = nullptr);

    const GatewayProtocol& getGatewayProtocol() const override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    void requestActuatorStatusesForDevice(const std::string& deviceKey) override;
    void requestActuatorStatusesForAllDevices() override;

private:
    void handleMessageForDevice(std::shared_ptr<Message> message) override;

    void routePlatformToDeviceMessage(std::shared_ptr<Message> message);

    DeviceRepository* m_deviceRepository;

    OutboundMessageHandler& m_outboundDeviceMessageHandler;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_INTERNALDATASERVICE_H
