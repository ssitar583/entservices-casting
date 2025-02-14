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

#ifndef RFC_UTILS_H

#define RFC_UTILS_H

inline void utilConvertParamDataToRfc(const WPEFramework::Exchange::IRfc::RFC_ParamData& iRfcParamData /* @in */, RFC_ParamData_t* rfcParamData /* @out */)
 {
     strcpy(rfcParamData->name, iRfcParamData.name.c_str());
     strcpy(rfcParamData->value, iRfcParamData.value.c_str());
     rfcParamData->type = (DATA_TYPE) iRfcParamData.type;
 }
 
inline void utilConvertParamDataFromRfc(const RFC_ParamData_t* rfcParamData /* @in */, WPEFramework::Exchange::IRfc::RFC_ParamData& iRfcParamData /* @out */)
 {
     /* pstParamData.name will be remain unchanged */
     iRfcParamData.value.assign(rfcParamData->value);
     iRfcParamData.type = (uint32_t)rfcParamData->type;
 }

inline void utilConvertParamDataToIRfc(const RFC_ParamData_t* rfcParamData /* @in */, WPEFramework::Exchange::IRfc::RFC_ParamData& iRfcParamData /* @out */)
 {
     iRfcParamData.name.assign(rfcParamData->name);
     iRfcParamData.value.assign(rfcParamData->value);
     iRfcParamData.type = (uint32_t) rfcParamData->type;
 }
 
inline void utilConvertParamDataFromIRfc(const WPEFramework::Exchange::IRfc::RFC_ParamData& iRfcParamData /* @in */, RFC_ParamData_t* rfcParamData /* @out */)
 {
     /* pstParamData->name will be remain unchanged */
     strcpy(rfcParamData->value, iRfcParamData.value.c_str());
     rfcParamData->type = (DATA_TYPE)iRfcParamData.type;
 }


#endif /* RFC_UTILS_H */
