#include "WolkExternal.h"

namespace wolkabout
{
void WolkExternal::connect()
{
    connectToPlatform(true);
}

void WolkExternal::disconnect()
{
    addToCommandBuffer([=]() -> void { m_platformConnectivityService->disconnect(); });
}

}    // namespace wolkabout
