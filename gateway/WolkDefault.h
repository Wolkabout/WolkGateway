/*
 * Copyright 2021 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WOLKABOUT_WOLKDEFAULT_H
#define WOLKABOUT_WOLKDEFAULT_H

#include "Wolk.h"

namespace wolkabout
{
namespace gateway
{
class InternalDeviceStatusService;

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

    std::unique_ptr<InternalDeviceStatusService> m_deviceStatusService;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKABOUT_WOLKDEFAULT_H
