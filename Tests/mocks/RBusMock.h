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

#include "RBus.h"

class RBusApiImplMock : public RBusApiImpl {
public:
    virtual ~RBusApiImplMock() = default;

    MOCK_METHOD(rbusError_t, rbus_open, (rbusHandle_t* handle, char const* componentName), (override));
    MOCK_METHOD(rbusError_t, rbusMethod_InvokeAsync, (rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusMethodAsyncRespHandler_t callback,  int timeout), (override));
    MOCK_METHOD(rbusValue_t, rbusObject_GetValue, (rbusObject_t object, char const* name), (override));
    MOCK_METHOD(char const*, rbusValue_GetString, (rbusValue_t value, int* len), (override));
    MOCK_METHOD(rbusError_t, rbus_close, (rbusHandle_t handle) , (override));
    MOCK_METHOD(rbusValue_t,rbusValue_Init,(rbusValue_t* pvalue), (override));;
    MOCK_METHOD(void,rbusValue_SetString,(rbusValue_t value, char const* s), (override));;
    MOCK_METHOD(rbusError_t,rbus_set, (rbusHandle_t handle, char const* name, rbusValue_t value, rbusSetOptions_t* opts), (override));;
    MOCK_METHOD(void,rbusValue_Release,(rbusValue_t value), (override));;
};
