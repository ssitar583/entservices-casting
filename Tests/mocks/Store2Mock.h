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
#include <interfaces/IStore2.h>

using ::WPEFramework::Exchange::IStore2;

class Store2Mock : public IStore2 {
public:
    Store2Mock() = default;
    virtual ~Store2Mock() = default;

    MOCK_METHOD(uint32_t , Register, (INotification *notification), (override));
    MOCK_METHOD(uint32_t , Unregister, (INotification *notification), (override));
    MOCK_METHOD(uint32_t ,SetValue,(const ScopeType scope, const string& ns , const string& key, const string& value, const uint32_t ttl), (override));
    MOCK_METHOD(uint32_t ,GetValue,(const ScopeType scope, const string& ns , const string& key, string& value , uint32_t& ttl ), (override));
    MOCK_METHOD(uint32_t ,DeleteKey,(const ScopeType scope, const string& ns , const string& key) ,(override));
    MOCK_METHOD(uint32_t ,DeleteNamespace,(const ScopeType scope, const string& ns ),(override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));

};


class Store2NotificationMock : public IStore2::INotification {
    public:
    Store2NotificationMock() = default;
    virtual ~Store2NotificationMock() = default;
};
