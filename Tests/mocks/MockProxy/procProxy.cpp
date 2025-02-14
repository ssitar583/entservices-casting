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

#include "../readprocMockInterface.h"
#include "../thunder/Module.h"
#ifndef MODULE_NAME
#define MODULE_NAME RdkServicesL2Test
#endif
#include <core/core.h>
#include <interfaces/IProc.h>
#include <cassert> 
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

class procProxy {
public:
procProxy();  // Constructor
~procProxy();

static procProxy& Instance() {
    static procProxy instance;
    return instance;
}

public:
    // Interface methods
    static PROCTAB* openproc(int flags, ... /* pid_t*|uid_t*|dev_t*|char* [, int n] */ );
    static void closeproc(PROCTAB* PT);
    static proc_t* readproc(PROCTAB *__restrict const PT, proc_t *__restrict p);

    uint32_t Initialize();
    // Static member to hold the implementation
    static WPEFramework::Exchange::IProc* _mockPluginReadproc;

private:
    WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>> _engine;
    WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient> _communicatorClient;

};
PROCTAB proctab = {};
proc_t proc = {};

WPEFramework::Exchange::IProc* procProxy::_mockPluginReadproc = nullptr;

uint32_t procProxy::Initialize() {
    TEST_LOG("Initializing COM RPC engine and communicator client");

    if (!_engine.IsValid()) {
        _engine = WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>>::Create();
        if (!_engine.IsValid()) {
            TEST_LOG("Failed to create _engine");
            return WPEFramework::Core::ERROR_GENERAL;
        }
    }

    if (!_communicatorClient.IsValid()) {
        _communicatorClient = WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient>::Create(
            WPEFramework::Core::NodeId("/tmp/communicator"),
            WPEFramework::Core::ProxyType<WPEFramework::Core::IIPCServer>(_engine)
        );
        if (!_communicatorClient.IsValid()) {
            TEST_LOG("Failed to create _communicatorClient");
            return WPEFramework::Core::ERROR_GENERAL;
        }
    }
        TEST_LOG("Connecting the COM-RPC socket");
        _mockPluginReadproc = _communicatorClient->Open<WPEFramework::Exchange::IProc>(_T("org.rdk.MockPlugin"), ~0, 3000);
        if (_mockPluginReadproc) {
            TEST_LOG("Connected to MockPlugin successfully");
        } else {
            TEST_LOG("Failed to create MockPlugin Controller");
        }

    return WPEFramework::Core::ERROR_NONE;
}

procProxy::procProxy()
{
    TEST_LOG("Inside procProxy constructor");
}

procProxy::~procProxy() {
    TEST_LOG("Inside procProxy destructor");
    if (_mockPluginReadproc) {
        _mockPluginReadproc->Release();
        _mockPluginReadproc = nullptr;
    }

    // Close the communicator client if it's valid
    if (_communicatorClient.IsValid()) {
        _communicatorClient->Close(WPEFramework::Core::infinite);
    }

    // Release the communicator client and engine
    _communicatorClient.Release();
    _engine.Release();
}

PROCTAB* procProxy::openproc(int flags, ... /* pid_t*|uid_t*|dev_t*|char* [, int n] */ )
{
    if (!_mockPluginReadproc) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("RPC initialize and connection failed");
            return nullptr;
        }
    }

    if ( _mockPluginReadproc != nullptr)
    {
        uint32_t PT = 0;
        if (WPEFramework::Core::ERROR_NONE == _mockPluginReadproc->openproc(flags, PT))
        {
            proctab.procfs = reinterpret_cast<DIR*>(PT);
        }
        else
        {
            return nullptr;
        }
    }

    return &proctab;
}

void procProxy::closeproc(PROCTAB* PT)
{
    if (!_mockPluginReadproc) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("RPC initialize and connection failed");
            return;
        }
    }

    if ( _mockPluginReadproc != nullptr)
    {
        _mockPluginReadproc->closeproc((uint32_t)(uintptr_t)proctab.procfs);
    }
}

proc_t* procProxy::readproc(PROCTAB *__restrict const PT, proc_t *__restrict p)
{
    if (!_mockPluginReadproc) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("RPC initialize and connection failed");
            return nullptr;
        }
    }

    if (( _mockPluginReadproc != nullptr) && (PT != nullptr ))
    {
        uint32_t procfs = (uint32_t)(uintptr_t)PT->procfs; int tid , ppid; string cmd;
        if (WPEFramework::Core::ERROR_NONE == _mockPluginReadproc->readproc(procfs, tid , ppid ,cmd ))
        {
            strcpy(p->cmd,cmd.c_str());
            p->tid = tid;
            p->ppid = ppid;
            proc = *p;
        }
        else
        {
            return nullptr;
        }
    }
    return &proc;
}

PROCTAB* (*openproc)(int, ...) = &procProxy::openproc;
void (*closeproc)(PROCTAB*) = &procProxy::closeproc;
proc_t* (*readproc)(PROCTAB*, proc_t*) = &procProxy::readproc;

