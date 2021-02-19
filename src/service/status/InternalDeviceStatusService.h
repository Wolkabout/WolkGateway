#ifndef INTERNALDEVICESTATUSSERVICE_H
#define INTERNALDEVICESTATUSSERVICE_H

#include "DeviceStatusService.h"

namespace wolkabout
{
class InternalDeviceStatusService : public DeviceStatusService, public DeviceMessageListener
{
public:
    InternalDeviceStatusService(std::string gatewayKey, StatusProtocol& protocol,
                                GatewayStatusProtocol& gatewayProtocol, DeviceRepository* deviceRepository,
                                OutboundMessageHandler& outboundPlatformMessageHandler,
                                OutboundMessageHandler& outboundDeviceMessageHandler,
                                std::chrono::seconds statusRequestInterval);

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getGatewayProtocol() const override;

    void connected() override;
    void disconnected() override;

    void sendLastKnownStatusForDevice(const std::string& deviceKey);

private:
    void requestDeviceStatus(const std::string& deviceKey) override;

    void requestDevicesStatus();
    void validateDevicesStatus();

    void sendStatusRequestForDevice(const std::string& deviceKey);
    void sendStatusRequestForAllDevices();

    bool containsDeviceStatus(const std::string& deviceKey);
    std::pair<std::time_t, DeviceStatus::Status> getDeviceStatus(const std::string& deviceKey);
    void logDeviceStatus(const std::string& deviceKey, DeviceStatus::Status status);

    GatewayStatusProtocol& m_gatewayProtocol;

    DeviceRepository* m_deviceRepository;

    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    const std::chrono::seconds m_statusRequestInterval;
    const std::chrono::seconds m_statusResponseInterval;
    Timer m_requestTimer;
    Timer m_responseTimer;

    std::mutex m_deviceStatusMutex;
    std::map<std::string, std::pair<std::time_t, DeviceStatus::Status>> m_deviceStatuses;
};
}    // namespace wolkabout

#endif    // INTERNALDEVICESTATUSSERVICE_H
