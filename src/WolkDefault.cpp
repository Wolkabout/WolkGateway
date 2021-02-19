#include "WolkDefault.h"

#include "repository/ExistingDevicesRepository.h"
#include "service/DeviceStatusService.h"
#include "service/PublishingService.h"
#include "utilities/Logger.h"

namespace
{
const unsigned RECONNECT_DELAY_MSEC = 2000;
}

namespace wolkabout
{
WolkDefault::~WolkDefault() = default;

void WolkDefault::connect()
{
    connectToPlatform(true);
    connectToDevices(true);
}

void WolkDefault::disconnect()
{
    addToCommandBuffer([=]() -> void { m_platformConnectivityService->disconnect(); });
    addToCommandBuffer([=]() -> void {
        if (m_deviceConnectivityService)
        {
            m_deviceConnectivityService->disconnect();
        }
    });
}

WolkDefault::WolkDefault(GatewayDevice device) : Wolk(device) {}

void WolkDefault::deviceRegistered(const std::string& deviceKey)
{
    addToCommandBuffer([=] {
        m_deviceStatusService->sendLastKnownStatusForDevice(deviceKey);
        m_existingDevicesRepository->addDeviceKey(deviceKey);
    });
}

void WolkDefault::deviceUpdated(const std::string& deviceKey)
{
    addToCommandBuffer([=] { m_deviceStatusService->sendLastKnownStatusForDevice(deviceKey); });
}

void WolkDefault::devicesDisconnected()
{
    addToCommandBuffer([=] {
        notifyDevicesDisonnected();
        connectToDevices(true);
    });
}

void WolkDefault::notifyDevicesConnected()
{
    LOG(INFO) << "Connection to local bus established";

    m_devicePublisher->connected();

    m_deviceStatusService->connected();
}

void WolkDefault::notifyDevicesDisonnected()
{
    LOG(INFO) << "Connection to local bus lost";

    m_devicePublisher->disconnected();

    m_deviceStatusService->disconnected();
}

void WolkDefault::connectToDevices(bool firstTime)
{
    if (!m_deviceConnectivityService)
    {
        return;
    }

    addToCommandBuffer([=]() -> void {
        if (firstTime)
            LOG(INFO) << "Connecting to local bus...";

        if (m_deviceConnectivityService->connect())
        {
            notifyDevicesConnected();
        }
        else
        {
            if (firstTime)
                LOG(INFO) << "Failed to connect to local bus";

            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY_MSEC));
            connectToDevices(false);
        }
    });
}

}    // namespace wolkabout
