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
#include <interfaces/ILifeCycleManager.h>

using ::WPEFramework::Exchange::ILifeCycleManager;

class LifeCycleManagerMock : public ILifeCycleManager {
public:
    LifeCycleManagerMock() = default;
    virtual ~LifeCycleManagerMock() = default;

    MOCK_METHOD(WPEFramework::Core::hresult, Register, (INotification *notification), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Unregister, (INotification *notification), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, GetLoadedApps, (const bool verbose, std::string &apps), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, IsAppLoaded, (const std::string &appId, bool &loaded), (const, override));
    MOCK_METHOD(WPEFramework::Core::hresult, SpawnApp, (const string& appId , const string& appPath , const string& appConfig , const string& runtimeAppId , const string& runtimePath , const string& runtimeConfig , const string& launchIntent , const string& environmentVars , const bool enableDebugger , const LifeCycleState targetLifecycleState , const string& launchArgs , string& appInstanceId , string& errorReason , bool& success ), (override));
    MOCK_METHOD(uint32_t, SetTargetAppState, (const string& appInstanceId , const LifeCycleState targetLifecycleState , const string& launchIntent),(override));
    MOCK_METHOD(WPEFramework::Core::hresult, UnloadApp,(const string& appInstanceId , string& errorReason , bool& success) , (override));
    MOCK_METHOD(WPEFramework::Core::hresult, KillApp,(const string& appInstanceId , string& errorReason , bool& success) , (override));
    MOCK_METHOD(WPEFramework::Core::hresult, SendIntentToActiveApp,(const string& appInstanceId , const string& intent , string& errorReason , bool& success), (override)) ;

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));

};


class LifeCycleManagerNotificationMock : public ILifeCycleManager::INotification {
    public:
    LifeCycleManagerNotificationMock() = default;
    virtual ~LifeCycleManagerNotificationMock() = default;
};
