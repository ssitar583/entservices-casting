/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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
#pragma once

// #include <string.h>
// #include <stdlib.h>
// #ifndef TR181API_H
// #define TR181API_H

  #define MAX_PARAM_LEN_TR181     (2*1024)

typedef enum
{   
    TR181_STRING = 0,
    TR181_INT,
    TR181_UINT,
    TR181_BOOLEAN,
    TR181_DATETIME,
    TR181_BASE64,
    TR181_LONG,
    TR181_ULONG,
    TR181_FLOAT,
    TR181_DOUBLE,
    TR181_BYTE,
    TR181_NONE,
    TR181_BLOB
} TR181_PARAM_TYPE;

typedef struct _TR181_Param_t {
   char value[MAX_PARAM_LEN_TR181];
   TR181_PARAM_TYPE type;
} TR181_ParamData_t;

/*Error Code type*/
typedef enum _tr181ErrorCodes
{
    tr181Success = 0,
    tr181Failure,
    tr181Timeout,
    tr181InvalidParameterName,
    tr181InvalidParameterValue,
    tr181InvalidType,
    tr181NotWritable,
    tr181ValueIsEmpty,
    tr181ValueIsNull,
    tr181InternalError,
    tr181DefaultValue,
} tr181ErrorCode_t;

class Tr181ApiImpl {
public:
    virtual ~Tr181ApiImpl() = default;

    virtual tr181ErrorCode_t getParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData) = 0;
    virtual tr181ErrorCode_t setParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue) = 0;
    virtual tr181ErrorCode_t clearParam(char *pcCallerID, const char* pcParameterName) = 0;
    virtual const char* getTR181ErrorString(tr181ErrorCode_t code) = 0;

    virtual tr181ErrorCode_t getLocalParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData) = 0;
    virtual tr181ErrorCode_t setLocalParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue) = 0;
    virtual tr181ErrorCode_t clearLocalParam(char *pcCallerID, const char* pcParameterName) = 0;
};

class Tr181Api {
protected:
    static Tr181ApiImpl* impl;

public:
    Tr181Api();
    Tr181Api(const Tr181Api &obj) = delete; // Deleted copy constructor to prevent copying.

    static void setImpl(Tr181ApiImpl* newImpl);
    static tr181ErrorCode_t getParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData);
    static tr181ErrorCode_t setParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue);
    static tr181ErrorCode_t clearParam(char *pcCallerID, const char* pcParameterName);
    static const char* getTR181ErrorString(tr181ErrorCode_t code);

    static tr181ErrorCode_t getLocalParam(char *pcCallerID, const char* pcParameterName, TR181_ParamData_t *pstParamData);
    static tr181ErrorCode_t setLocalParam(char *pcCallerID, const char* pcParameterName, const char* pcParameterValue);
    static tr181ErrorCode_t clearLocalParam(char *pcCallerID, const char* pcParameterName);
};

// Function pointers for dynamic access if needed
extern tr181ErrorCode_t (*getParam)(char*, const char*, TR181_ParamData_t*);
extern tr181ErrorCode_t (*setParam)(char*, const char*, const char*);
extern tr181ErrorCode_t (*clearParam)(char*, const char*);
extern const char* (*getTR181ErrorString)(tr181ErrorCode_t);
extern tr181ErrorCode_t (*getLocalParam)(char*, const char*, TR181_ParamData_t*);
extern tr181ErrorCode_t (*setLocalParam)(char*, const char*, const char*);
extern tr181ErrorCode_t (*clearLocalParam)(char*, const char*);

// #endif // TR181API_H
