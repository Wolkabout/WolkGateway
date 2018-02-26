#include "model/DeviceRegistrationResponseDto.h"

#include <utility>

namespace wolkabout
{
DeviceRegistrationResponseDto::DeviceRegistrationResponseDto(DeviceRegistrationResponseDto::Result result)
: m_result(std::move(result))
{
}

DeviceRegistrationResponseDto::Result DeviceRegistrationResponseDto::getResult() const
{
    return m_result;
}
}    // namespace wolkabout
