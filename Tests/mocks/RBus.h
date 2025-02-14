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

typedef enum _rbusError
{
    RBUS_ERROR_SUCCESS,
    RBUS_ERROR_NOT_INITIALIZED,
    RBUS_ERROR_BUS_ERROR,
} rbusError_t;

struct _rbusHandle
{
};

typedef struct _rbusHandle* rbusHandle_t;

 struct _rbusObject
{
};
typedef struct _rbusObject* rbusObject_t;

struct _rbusValue
{
};

typedef struct _rbusSetOptions
{
    bool commit;
    unsigned int sessionId;
} rbusSetOptions_t;

typedef struct _rbusValue* rbusValue_t;

typedef void (*rbusMethodAsyncRespHandler_t)(rbusHandle_t handle, char const* methodName, rbusError_t error, rbusObject_t params);


class RBusApiImpl {
public:
    virtual ~RBusApiImpl() = default;

    virtual rbusError_t rbus_open(rbusHandle_t* handle, char const* componentName) = 0;
    virtual rbusError_t rbusMethod_InvokeAsync(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusMethodAsyncRespHandler_t callback,  int timeout) = 0;
    virtual rbusValue_t rbusObject_GetValue(rbusObject_t object, char const* name) = 0;
    virtual char const* rbusValue_GetString(rbusValue_t value, int* len) = 0;
    virtual rbusError_t rbus_close(rbusHandle_t handle) = 0;
    virtual rbusValue_t rbusValue_Init(rbusValue_t* pvalue) = 0;
    virtual void rbusValue_SetString(rbusValue_t value, char const* s) = 0;
    virtual rbusError_t rbus_set( rbusHandle_t handle, char const* name, rbusValue_t value, rbusSetOptions_t* opts) = 0;
    virtual void rbusValue_Release(rbusValue_t value) = 0;
};

class RBusApi {
protected:
    static RBusApiImpl* impl;

public:

    RBusApi();
    RBusApi(const RBusApi &obj) = delete;
    static void setImpl(RBusApiImpl* newImpl);
    static rbusError_t rbus_open(rbusHandle_t* handle, char const* componentName);
    static rbusError_t rbusMethod_InvokeAsync(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusMethodAsyncRespHandler_t callback,  int timeout);
    static rbusValue_t rbusObject_GetValue(rbusObject_t object, char const* name);
    static char const* rbusValue_GetString(rbusValue_t value, int* len);
    static rbusError_t rbus_close(rbusHandle_t handle);
    static rbusValue_t rbusValue_Init(rbusValue_t* pvalue);
    static void rbusValue_SetString(rbusValue_t value, char const* s);
    static rbusError_t rbus_set( rbusHandle_t handle, char const* name, rbusValue_t value, rbusSetOptions_t* opts);
    static void rbusValue_Release(rbusValue_t value);
};

extern rbusError_t (*rbus_open)(rbusHandle_t*,char const* );
extern rbusError_t (*rbusMethod_InvokeAsync)(rbusHandle_t,char const*, rbusObject_t,rbusMethodAsyncRespHandler_t,int);
extern rbusValue_t (*rbusObject_GetValue)(rbusObject_t,char const*);
extern char const* (*rbusValue_GetString)(rbusValue_t,int*);
extern rbusError_t (*rbus_close)(rbusHandle_t);
extern rbusValue_t (*rbusValue_Init)(rbusValue_t*);
extern void (*rbusValue_SetString)(rbusValue_t, char const*);
extern rbusError_t (*rbus_set)( rbusHandle_t , char const* , rbusValue_t , rbusSetOptions_t*);
extern void (*rbusValue_Release)(rbusValue_t);

