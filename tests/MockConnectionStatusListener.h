#ifndef MOCKCONNECTIONSTATUSLISTENER_H
#define MOCKCONNECTIONSTATUSLISTENER_H

#include "ConnectionStatusListener.h"

#include <gmock/gmock.h>

class MockConnectionStatusListener : public wolkabout::ConnectionStatusListener
{
public:
    MockConnectionStatusListener() {}
    virtual ~MockConnectionStatusListener() {}

    MOCK_METHOD0(connected, void());

    MOCK_METHOD0(disconnected, void());

private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockConnectionStatusListener);
};

#endif    // MOCKCONNECTIONSTATUSLISTENER_H
