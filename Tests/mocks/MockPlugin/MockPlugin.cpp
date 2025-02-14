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

#include "MockPlugin.h"
#include "MockAccessor.h"
#include "../Rfc.h"
#include "../MockUtils/RfcUtils.h"


#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

namespace WPEFramework
{
    namespace Plugin
    {
        // Register the MockPlugin 
        SERVICE_REGISTRATION(MockPlugin, 1, 0);

        MockPlugin::MockPlugin()
            : _connectionId(0), _service(nullptr)
        {
            TEST_LOG("Inside Mock plugin constructor");
        }

        MockPlugin::~MockPlugin()
        {
            TEST_LOG("Inside Mock plugin destructor");
        }

        IarmBusImpl* MockPlugin::_iarmImpl = nullptr;
        readprocImpl* MockPlugin::_procImpl = nullptr;
        RBusApiImpl* MockPlugin::_rbusImpl = nullptr;
        RfcApiImpl* MockPlugin::_rfcImpl = nullptr;

        std::list<Exchange::IIarm::INotification*> MockPlugin::_iarmNotificationCallbacks;

        const std::string MockPlugin::Initialize(PluginHost::IShell *service)
        {
            TEST_LOG("Inside Mock plugin Initialize");
            _service = service;
            _iarmImpl = MockAccessor<IarmBusImpl>::getPtr();
            assert (nullptr != _iarmImpl);
            _procImpl = MockAccessor<readprocImpl>::getPtr();
            assert (nullptr != _procImpl);
            _rbusImpl = MockAccessor<RBusApiImpl>::getPtr();
            assert (nullptr != _rbusImpl);
            _rfcImpl = MockAccessor<RfcApiImpl>::getPtr();
            assert (nullptr != _rfcImpl);

            return "";
        }

        void MockPlugin::Deinitialize(PluginHost::IShell *service)
        {
            TEST_LOG("Inside Mock plugin Deinitialize Entry _iarmNotificationCallbacks size = %ld", _iarmNotificationCallbacks.size());
            std::lock_guard<std::mutex> lock(_notificationMutex);
            _iarmNotificationCallbacks.clear();
            _service = nullptr;
            _iarmImpl = nullptr;
            _procImpl = nullptr;
            _rbusImpl = nullptr;
            _rfcImpl = nullptr;
            TEST_LOG("Inside Mock plugin Deinitialize Exit _iarmNotificationCallbacks size = %ld", _iarmNotificationCallbacks.size());
        }

        std::string MockPlugin::Information() const
        {
            TEST_LOG("Inside Mock plugin Information");
            return "MockPlugin Information";
        }

//
/* ********************************     IARM related mocks starts. *********************************/
//
        uint32_t MockPlugin::IARM_Bus_Init(const string& message)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_Init IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_Init");

            return (uint32_t)_iarmImpl-> IARM_Bus_Init("Thunder_Plugins");
        }

        uint32_t MockPlugin::IARM_Bus_Connect()
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_Connect IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_Connect");

            return (uint32_t)_iarmImpl-> IARM_Bus_Connect();
        }

        uint32_t MockPlugin::IARM_Bus_RegisterEventHandler(const string& ownerName, int eventId)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_RegisterEventHandler IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_RegisterEventHandler");

            return (uint32_t)_iarmImpl-> IARM_Bus_RegisterEventHandler(ownerName.c_str(),eventId,reinterpret_cast<IARM_EventHandler_t>(&sendNotificationIarm));
        }

        uint32_t MockPlugin::IARM_Bus_RemoveEventHandler(const string& ownerName, int eventId)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_RemoveEventHandler IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_RemoveEventHandler");

            return (uint32_t)_iarmImpl-> IARM_Bus_RemoveEventHandler(ownerName.c_str(),eventId,reinterpret_cast<IARM_EventHandler_t>(&sendNotificationIarm));
        }

        uint32_t MockPlugin::IARM_Bus_Call(const string& ownerName, const string& methodName, uint8_t* arg, uint32_t argLen)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_Call IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_Call, argLen: %u",argLen);

            return (uint32_t)_iarmImpl-> IARM_Bus_Call(ownerName.c_str(), methodName.c_str(), reinterpret_cast<void*>(arg), argLen);
        }

        uint32_t MockPlugin::IARM_Bus_BroadcastEvent(const string& ownerName, int eventId, uint8_t *arg, uint32_t argLen)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_BroadcastEvent IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_BroadcastEvent");

            return (uint32_t)_iarmImpl-> IARM_Bus_BroadcastEvent(ownerName.c_str(), eventId, reinterpret_cast<void*>(arg), argLen);
        }

        uint32_t MockPlugin::IARM_Bus_RegisterCall(const string& methodName, const uint8_t* handler)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_RegisterCall IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_RegisterCall");

            return (uint32_t)_iarmImpl-> IARM_Bus_RegisterCall(methodName.c_str(), reinterpret_cast<IARM_BusCall_t>(handler));
        }

        uint32_t MockPlugin::IARM_Bus_CallWithIPCTimeout(const string& ownerName, const string& methodName, uint8_t *arg, uint32_t argLen, int timeout)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_CallWithIPCTimeout IarmBusImpl = %p", _iarmImpl);
            TEST_LOG("Calling IARM_Bus_CallWithIPCTimeout");

            return (uint32_t)_iarmImpl-> IARM_Bus_Call_with_IPCTimeout(ownerName.c_str(), methodName.c_str(), reinterpret_cast<void*>(arg), argLen, timeout);
        }

        uint32_t MockPlugin::IARM_Bus_IsConnected(const string& message, int& result)
        {
            TEST_LOG("Inside Mock plugin IARM_Bus_IsConnected IarmBusImpl = %p", _iarmImpl);
            int isRegistered = 0;
            IARM_Result_t returnValue;
            TEST_LOG("Calling IARM_Bus_IsConnected");
            returnValue = _iarmImpl-> IARM_Bus_IsConnected(message.c_str(),&isRegistered);
            TEST_LOG("IARM_Bus_IsConnected completed isRegistered: %d", isRegistered);
            result = isRegistered;
            return (uint32_t)returnValue;
        }

        void MockPlugin::sendNotificationIarm(const char* ownerName,IARM_EventId_t eventId,void* data,size_t len)
        {
            TEST_LOG("sendNotificationIarm called from gtest mock");
            for (const auto& notifyCb : _iarmNotificationCallbacks) 
            {
                std::string eventData(static_cast<char*>(data), len);
                uint32_t result = notifyCb->IARM_EventHandler(ownerName,eventId, eventData);
                TEST_LOG("sendNotificationIarm completed result: %d", result);
            }
        }

        uint32_t MockPlugin::Register(Exchange::IIarm::INotification *notification)
        {
            TEST_LOG("MockPlugin::Register Entry");
            if (nullptr == notification)
            {
                TEST_LOG("Error: Received a null notification pointer.");
                return Core::ERROR_GENERAL;
            }

            std::lock_guard<std::mutex> locker(_notificationMutex);

            if (std::find(_iarmNotificationCallbacks.begin(), _iarmNotificationCallbacks.end(), notification) == _iarmNotificationCallbacks.end())
            {
                _iarmNotificationCallbacks.push_back(notification);
                notification->AddRef();
            }
            else
            {
                TEST_LOG(" Already registered the notification");
            }

            return Core::ERROR_NONE;
            TEST_LOG("MockPlugin::Register Exit");
        }

        uint32_t MockPlugin::Unregister(IIarm::INotification *notification)
        {
            std::lock_guard<std::mutex> locker(_notificationMutex);

            auto itr = std::find(_iarmNotificationCallbacks.begin(), _iarmNotificationCallbacks.end(), notification);
            if (itr != _iarmNotificationCallbacks.end())
            {
                (*itr)->Release();
                _iarmNotificationCallbacks.erase(itr);
            }
            else
            {
                TEST_LOG("Error Item not found in the list");
            }
            return Core::ERROR_NONE;
        }

//
/* ********************************     IARM related mocks End. *********************************/
//

//
/* ********************************     Proc related mocks starts. *********************************/
//
        uint32_t MockPlugin::openproc(int flags , uint32_t &PT)
        {
            PROCTAB *pTab = NULL;
            if ((pTab = _procImpl->openproc(flags)) == NULL)
            {
                return Core::ERROR_GENERAL;
            }
            else
            {
                PT = (uint32_t)(uintptr_t)pTab->procfs;
            }
            return Core::ERROR_NONE;
        }

        uint32_t MockPlugin::closeproc(const uint32_t PT )
        {
            PROCTAB pTab = {};
            pTab.procfs = reinterpret_cast<DIR*>(PT);
            _procImpl->closeproc(&pTab);
            return Core::ERROR_NONE;
        }

        uint32_t MockPlugin::readproc(const uint32_t PT, int &tid , int &ppid , string &cmd )
        {
            PROCTAB pTab = {};
            proc_t p = {};
            pTab.procfs = reinterpret_cast<DIR*>(PT);
            if ((_procImpl->readproc(&pTab,&p)) == NULL)
            {
                return Core::ERROR_GENERAL;
            }
            else
            {
                cmd = string(p.cmd,16);
                tid = p.tid;
                ppid = p.ppid;
            }
            return Core::ERROR_NONE;
        }

//
/* ********************************     Proc related mocks End. *********************************/
//

//
/* ********************************     Rbus related mocks starts. *********************************/
//
        uint32_t MockPlugin::rbus_close(const std::string& handle)
        {
            TEST_LOG("Inside Mock plugin rbus_close, rbusImpl = %p", _rbusImpl);
            TEST_LOG("Calling rbus_close with handle: %s", handle.c_str());

            // Call the mocked implementation of rbus_close
            _rbusHandle* rbusHandle = nullptr;
            uint32_t result = static_cast<uint32_t>(_rbusImpl->rbus_close(rbusHandle));

            return result;
        }

//
/* ********************************     Rbus related mocks Ends. *********************************/
//

//
/* ********************************     Rfc related mocks starts. *********************************/
//
        uint32_t MockPlugin::getRFCParameter(const string& pcCallerID, const string& pcParameterName, RFC_ParamData& pstParamData)
        {
            TEST_LOG("Inside Mock plugin getRFCParameter pcCallerID = %s pcParameterName = %s", pcCallerID.c_str(), pcParameterName.c_str());

            char* callerId = new char[pcCallerID.length() + 1];
            strcpy(callerId, pcCallerID.c_str());

            char* parameterName = new char[pcParameterName.length() + 1];
            strcpy(parameterName, pcParameterName.c_str());

            RFC_ParamData_t paramData = {0};
            utilConvertParamDataToRfc(pstParamData, &paramData);

            TEST_LOG("Calling gmock getRFCParameter _rfcImpl = %p", _rfcImpl);
            assert (nullptr != _rfcImpl);
            _rfcImpl-> getRFCParameter(callerId, parameterName, &paramData);
            utilConvertParamDataFromRfc(&paramData, pstParamData);

            delete[] callerId;
            delete[] parameterName;

            TEST_LOG("Completed gmock getRFCParameter type = %d name = %s value = %s", paramData.type, paramData.name, paramData.value);
            return Core::ERROR_NONE;
        }

        uint32_t MockPlugin::setRFCParameter(const string& pcCallerID, const string& pcParameterName, const string& pcParameterValue, uint32_t eDataType)
        {
            TEST_LOG("Inside Mock plugin setRFCParameter _rfcImpl = %p pcParameterName = %s", _rfcImpl, pcParameterName);
            return Core::ERROR_NONE;
        }

        string rfcError;
        string& MockPlugin::getRFCErrorString(uint32_t code)
        {
            TEST_LOG("Inside Mock plugin getRFCErrorString _rfcImpl = %p", _rfcImpl);
            return rfcError;
        }
//
/* ********************************     Rfc related mocks Ends. *********************************/
//
    }
}

