#ifndef WOLKABOUT_DATAPROVIDER_H
#define WOLKABOUT_DATAPROVIDER_H

#include "DataHandler.h"

namespace wolkabout
{
class DataProvider
{
public:
    virtual ~DataProvider() = default;

    virtual void setDataHandler(DataHandler* handler, const std::string& gatewayKey) = 0;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_DATAPROVIDER_H
