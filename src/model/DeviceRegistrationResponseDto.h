#ifndef DEVICEREGISTRATIONRESPONSEDTO_H
#define DEVICEREGISTRATIONRESPONSEDTO_H

namespace wolkabout
{
class DeviceRegistrationResponseDto
{
public:
    enum class Result
    {
        OK,
        KEY_CONFLICT,
        KEY_AND_MANIFEST_CONFLICT,
        MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED
    };

    DeviceRegistrationResponseDto() = default;
    DeviceRegistrationResponseDto(DeviceRegistrationResponseDto::Result result);

    virtual ~DeviceRegistrationResponseDto() = default;

    DeviceRegistrationResponseDto::Result getResult() const;

private:
    DeviceRegistrationResponseDto::Result m_result;
};
}    // namespace wolkabout

#endif    // DEVICEREGISTRATIONRESPONSEDTO_H
