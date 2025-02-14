/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
*
 * Copyright 2024 Synamedia Ltd.
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
 */

#include "../Rfc.h"
#include "../thunder/Module.h"
#ifndef MODULE_NAME
#define MODULE_NAME RdkServicesL2Test
#endif
#include <core/core.h>
#include <interfaces/IRfc.h>
#include <cassert>
#include "../MockUtils/RfcUtils.h"


#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

class RfcMockProxy {
public:

    RfcMockProxy();  // Constructor
    ~RfcMockProxy();

    static RfcMockProxy& Instance() {
        static RfcMockProxy instance;
        return instance;
    }

    // Interface methods
    static WDMP_STATUS getRFCParameter(char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData);
    static WDMP_STATUS setRFCParameter(char* pcCallerID, const char* pcParameterName, const char* pcParameterValue, DATA_TYPE eDataType);
    static const char* getRFCErrorString(WDMP_STATUS code);

    WDMP_STATUS Initialize();
    // Static member to hold the implementation
    static WPEFramework::Exchange::IRfc* _mockPluginRfc;

private:
    RfcMockProxy(const RfcMockProxy&) = delete;
    RfcMockProxy& operator=(const RfcMockProxy&) = delete;

private:
    WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>> _engine;
    WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient> _communicatorClient;
};


WPEFramework::Exchange::IRfc* RfcMockProxy::_mockPluginRfc = nullptr;

WDMP_STATUS RfcMockProxy::Initialize() {
    TEST_LOG("Initializing COM RPC engine and communicator client");

    if (!_engine.IsValid()) {
        _engine = WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>>::Create();
        if (!_engine.IsValid()) {
            TEST_LOG("Failed to create _engine");
            return WDMP_FAILURE;
        }
    }

    if (!_communicatorClient.IsValid()) {
        _communicatorClient = WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient>::Create(
            WPEFramework::Core::NodeId("/tmp/communicator"),
            WPEFramework::Core::ProxyType<WPEFramework::Core::IIPCServer>(_engine)
        );
        if (!_communicatorClient.IsValid()) {
            TEST_LOG("Failed to create _communicatorClient");
            return WDMP_FAILURE;
        }
    }
        TEST_LOG("Connecting the COM-RPC socket");
        _mockPluginRfc = _communicatorClient->Open<WPEFramework::Exchange::IRfc>(_T("org.rdk.MockPlugin"), ~0, 3000);
        if (_mockPluginRfc) {
            TEST_LOG("Connected to MockPlugin successfully");
        } else {
            TEST_LOG("Failed to create MockPlugin Controller");
        }

    return WDMP_SUCCESS;
}

RfcMockProxy::RfcMockProxy() {
    TEST_LOG("Inside RfcMockProxy constructor");
}

RfcMockProxy::~RfcMockProxy() {
    TEST_LOG("Inside RfcMockProxy destructor");

    if (_mockPluginRfc) {
        _mockPluginRfc->Release();
        _mockPluginRfc = nullptr;
    }

    // Close the communicator client if it's valid
    if (_communicatorClient.IsValid()) {
        _communicatorClient->Close(WPEFramework::Core::infinite);
    }

    // Release the communicator client and engine
    _communicatorClient.Release();
    _engine.Release();
}

WDMP_STATUS RfcMockProxy::getRFCParameter(char* pcCallerID, const char* pcParameterName, RFC_ParamData_t* pstParamData)
{
    TEST_LOG("Inside getRFCParameter");

    if (!_mockPluginRfc) {
        WDMP_STATUS result = Instance().Initialize();
        if (result != WDMP_SUCCESS) {
            TEST_LOG("RPC initialize and connection failed");
            return result;
        }
    }

    if (_mockPluginRfc) {
        TEST_LOG("Serializing getRFCParameter Args");

        std::string callerId(pcCallerID);
        std::string parameterName(pcParameterName);

        WPEFramework::Exchange::IRfc::RFC_ParamData paramInfo;
        utilConvertParamDataToIRfc(pstParamData, paramInfo);

        TEST_LOG("RfcProxy Calling getRFCParameter of Mock Plugin");
        uint32_t result =_mockPluginRfc->getRFCParameter(callerId, parameterName, paramInfo);
        utilConvertParamDataFromIRfc(paramInfo, pstParamData);
        TEST_LOG("RfcProxy getRFCParameter completed type = %d value = %s", paramInfo.type, paramInfo.value.c_str());
    } else {
        TEST_LOG("Mock Plugin is not initialized");
    }

    return WDMP_SUCCESS;
}

WDMP_STATUS RfcMockProxy::setRFCParameter(char* pcCallerID, const char* pcParameterName, const char* pcParameterValue, DATA_TYPE eDataType)
{
    TEST_LOG("Inside setRFCParameter");

    return WDMP_SUCCESS;
}

const char* RfcMockProxy::getRFCErrorString(WDMP_STATUS code)
{
    TEST_LOG("Inside getRFCErrorString");

    return "";
}

WDMP_STATUS (*getRFCParameter)(char*,const char*,RFC_ParamData_t*) = &RfcMockProxy::getRFCParameter;
WDMP_STATUS (*setRFCParameter)(char*,const char*,const char*,DATA_TYPE) = &RfcMockProxy::setRFCParameter;
const char* (*getRFCErrorString)(WDMP_STATUS) = &RfcMockProxy::getRFCErrorString;

