/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#pragma once

#include <gmock/gmock.h>
#include "Dobby.h"


class DobbyProxyMock : public IDobbyProxy{


public:

    virtual ~DobbyProxyMock() = default;

    MOCK_METHOD(bool, shutdown, (), (const, override));
    MOCK_METHOD(bool, ping, (), (const, override));
    MOCK_METHOD(bool, isAlive, (const std::chrono::milliseconds& timeout), (const, override));
    MOCK_METHOD(bool, setLogMethod, (uint32_t method, int pipeFd), (const, override));
    MOCK_METHOD(bool, setLogLevel, (int level), (const, override));
    MOCK_METHOD(bool, setAIDbusAddress, (bool privateBus, const std::string& address), (const, override));
    MOCK_METHOD(int, getContainerState, (int32_t descriptor),(const));
    MOCK_METHOD(int32_t, startContainerFromSpec, (const std::string& id,
                                   const std::string& jsonSpec,
                                   const std::list<int>& files,
                                   const std::string& command,
                                   const std::string& displaySocket,
                                   const std::vector<std::string>& envVars), (const, override));
    MOCK_METHOD(int32_t, startContainerFromBundle, (const std::string& id,
                                     const std::string& bundlePath,
                                     const std::list<int>& files,
                                     const std::string& command,
                                     const std::string& displaySocket,
                                     const std::vector<std::string>& envVars), (const, override));
    MOCK_METHOD(bool, stopContainer, (int32_t cd, bool withPrejudice), (const, override));
    MOCK_METHOD(bool, pauseContainer, (int32_t cd), (const, override));
    MOCK_METHOD(bool, resumeContainer, (int32_t cd), (const, override));
    MOCK_METHOD(bool, execInContainer, (int32_t cd,
                         const std::string& options,
                         const std::string& command), (const, override));
    MOCK_METHOD(int, registerListener, (const StateChangeListener &listener, const void* cbParams), (override));
    MOCK_METHOD(void, unregisterListener, (int tag), (override));
    MOCK_METHOD(std::string, getContainerInfo, (int32_t descriptor), (const, override));
    MOCK_METHOD((std::list<std::pair<int32_t, std::string>>), listContainers, (), (const));

};

class IpcServiceMock: public AI_IPC::IIpcService{

public:

    virtual ~IpcServiceMock() = default;

    MOCK_METHOD(bool, isValid, (), (const, override));
    MOCK_METHOD(bool, isServiceAvailable, (const std::string& serviceName), (const));
    MOCK_METHOD(void, flush, (), ());
    MOCK_METHOD(bool, start, (), ());
    MOCK_METHOD(bool, stop, (), ());
    MOCK_METHOD(std::string, getBusAddress, (), (const));
};