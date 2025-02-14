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

#include "tr181api.h"

class Tr181ApiImplMock : public Tr181ApiImpl {
public:
    virtual ~Tr181ApiImplMock() = default;

    MOCK_METHOD(tr181ErrorCode_t, getParam, (char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData), ());
    MOCK_METHOD(tr181ErrorCode_t, setParam, (char *pcCallerID, const char* pcParameterName, const char* pcParameterValue), ());
    MOCK_METHOD(tr181ErrorCode_t, clearParam, (char *pcCallerID, const char* pcParameterName), ());
    MOCK_METHOD(const char*, getTR181ErrorString, (tr181ErrorCode_t code), ());
    MOCK_METHOD(tr181ErrorCode_t, getLocalParam, (char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData), ());
    MOCK_METHOD(tr181ErrorCode_t, setLocalParam, (char *pcCallerID, const char* pcParameterName, const char* pcParameterValue), ());
    MOCK_METHOD(tr181ErrorCode_t, clearLocalParam, (char *pcCallerID, const char* pcParameterName), ());
};
