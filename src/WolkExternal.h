#ifndef WOLKEXTERNAL_H
#define WOLKEXTERNAL_H

#include "Wolk.h"
#include "service/DataHandlerApiFacade.h"
#include "service/status/ExternalDeviceStatusService.h"

namespace wolkabout
{
class WolkExternal : public Wolk
{
public:
    using Wolk::Wolk;

    void connect() override;
    void disconnect() override;

private:
    friend class WolkBuilder;

    std::unique_ptr<ExternalDeviceStatusService> m_deviceStatusService;

    std::unique_ptr<DataHandlerApiFacade> m_dataApi;
};
}    // namespace wolkabout

#endif    // WOLKEXTERNAL_H
