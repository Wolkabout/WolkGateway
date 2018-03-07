/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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

#ifndef PROTOCOLHOLDER_H
#define PROTOCOLHOLDER_H

#include "ProtocolRegistrator.h"

namespace wolkabout
{
class ProtocolHolder
{
public:
    virtual ~ProtocolHolder() = default;
    virtual void accept(ProtocolRegistrator& pc, Wolk& wolk) = 0;
};

template <class T> class TemplateProtocolHolder : public ProtocolHolder
{
public:
    static T t;
    void accept(ProtocolRegistrator& pc, Wolk& wolk);
};

template <class T> void TemplateProtocolHolder<T>::accept(ProtocolRegistrator& pc, Wolk& wolk)
{
    pc.registerProtocol<T>(wolk);
}
}

#endif    // TEMPLATEHOLDER_H
