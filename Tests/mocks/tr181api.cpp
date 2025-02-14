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

#include "tr181api.h"
#include <gmock/gmock.h>

Tr181ApiImpl* Tr181Api::impl = nullptr;

Tr181Api::Tr181Api() {}

void Tr181Api::setImpl(Tr181ApiImpl* newImpl) {
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

tr181ErrorCode_t Tr181Api::getParam(char* pcCallerID, const char* pcParameterName, TR181_ParamData_t* pstParamData) {
    EXPECT_NE(impl, nullptr);
    return impl->getParam(pcCallerID, pcParameterName, pstParamData);
}

tr181ErrorCode_t Tr181Api::setParam(char* pcCallerID, const char* pcParameterName, const char* pcParameterValue) {
    EXPECT_NE(impl, nullptr);
    return impl->setParam(pcCallerID, pcParameterName, pcParameterValue);
}

tr181ErrorCode_t Tr181Api::clearParam(char* pcCallerID, const char* pcParameterName) {
    EXPECT_NE(impl, nullptr);
    return impl->clearParam(pcCallerID, pcParameterName);
}

const char* Tr181Api::getTR181ErrorString(tr181ErrorCode_t code) {
    EXPECT_NE(impl, nullptr);
    return impl->getTR181ErrorString(code);
}

tr181ErrorCode_t Tr181Api::getLocalParam(char* pcCallerID, const char* pcParameterName, TR181_ParamData_t* pstParamData) {
    EXPECT_NE(impl, nullptr);
    return impl->getLocalParam(pcCallerID, pcParameterName, pstParamData);
}

tr181ErrorCode_t Tr181Api::setLocalParam(char* pcCallerID, const char* pcParameterName, const char* pcParameterValue) {
    EXPECT_NE(impl, nullptr);
    return impl->setLocalParam(pcCallerID, pcParameterName, pcParameterValue);
}

tr181ErrorCode_t Tr181Api::clearLocalParam(char* pcCallerID, const char* pcParameterName) {
    EXPECT_NE(impl, nullptr);
    return impl->clearLocalParam(pcCallerID, pcParameterName);
}

// Dynamic access function pointers
tr181ErrorCode_t (*getParam)(char*, const char*, TR181_ParamData_t*) = &Tr181Api::getParam;
tr181ErrorCode_t (*setParam)(char*, const char*, const char*) = &Tr181Api::setParam;
tr181ErrorCode_t (*clearParam)(char*, const char*) = &Tr181Api::clearParam;
const char* (*getTR181ErrorString)(tr181ErrorCode_t) = &Tr181Api::getTR181ErrorString;
tr181ErrorCode_t (*getLocalParam)(char*, const char*, TR181_ParamData_t*) = &Tr181Api::getLocalParam;
tr181ErrorCode_t (*setLocalParam)(char*, const char*, const char*) = &Tr181Api::setLocalParam;
tr181ErrorCode_t (*clearLocalParam)(char*, const char*) = &Tr181Api::clearLocalParam;
