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

#pragma once

#include "Module.h"
#include <interfaces/IIarm.h>
#include <interfaces/IProc.h>
#include <interfaces/IRBus.h>
#include <interfaces/IRfc.h>


#include <mutex>

namespace WPEFramework
{
    namespace Plugin
    {
        class MockPlugin : public PluginHost::IPlugin, public Exchange::IIarm, public Exchange::IProc, public Exchange::IRBus, public Exchange::IRfc {
        public:
            MockPlugin();
            ~MockPlugin() override;

            MockPlugin(const MockPlugin &) = delete;
            MockPlugin &operator=(const MockPlugin &) = delete;

            //all the supported interfaces are to be added here
            BEGIN_INTERFACE_MAP(MockPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(Exchange::IIarm)
            INTERFACE_ENTRY(Exchange::IProc)
            INTERFACE_ENTRY(Exchange::IRBus)
            INTERFACE_ENTRY(Exchange::IRfc)
            END_INTERFACE_MAP

            // Implement the basic IPlugin interface 
            const std::string Initialize(PluginHost::IShell *service) override;
            void Deinitialize(PluginHost::IShell *service) override;
            std::string Information() const override;

//
/* ********************************     IARM related mocks *********************************/
//
            // Declare the Iarm methods
            uint32_t IARM_Bus_IsConnected(const string& message, int& result ) override;
            uint32_t IARM_Bus_Init(const string& message) override;
            uint32_t IARM_Bus_Connect() override;
            uint32_t IARM_Bus_RegisterEventHandler(const string& ownerName, int eventId) override;
            uint32_t IARM_Bus_RemoveEventHandler(const string& ownerName, int eventId) override;
            uint32_t IARM_Bus_Call(const string& ownerName, const string& methodName, uint8_t* arg, uint32_t argLen) override;
            uint32_t IARM_Bus_BroadcastEvent(const string& ownerName, int eventId, uint8_t *arg, uint32_t argLen) override;
            uint32_t IARM_Bus_RegisterCall(const string& methodName, const uint8_t* handler) override;
            uint32_t IARM_Bus_CallWithIPCTimeout(const string& ownerName, const string& methodName, uint8_t *arg, uint32_t argLen, int timeout) override;

            // Declare the methods to register for Iarm notifications
            uint32_t Register(Exchange::IIarm::INotification* notification ) override;
            uint32_t Unregister(Exchange::IIarm::INotification* notification ) override;

            // non-Iplugin methods
            static void sendNotificationIarm(const char* ownerName,IARM_EventId_t eventId,void* data,size_t len);

//
/* ********************************     Proc related mocks *********************************/
//
            uint32_t openproc(int flags , uint32_t &PT) override;
            uint32_t closeproc(const uint32_t PT ) override;
            uint32_t readproc(const uint32_t PT, int &tid , int &ppid , string &cmd ) override;
//
/* ********************************     RBus related mocks *********************************/
//
            uint32_t rbus_close(const std::string& handle) override;

//
/* ********************************     Rfc related mocks *********************************/
//
            uint32_t getRFCParameter(const string& pcCallerID /* @in */, const string& pcParameterName /* @in */, RFC_ParamData& pstParamData /* @in @out */) override;
            uint32_t setRFCParameter(const string& pcCallerID /* @in */, const string& pcParameterName /* @in */, const string& pcParameterValue /* @in */, uint32_t eDataType /* @in */) override;
            string& getRFCErrorString(uint32_t code /* @in */) override;


        private:
            //plugin common
            uint32_t _connectionId;
            PluginHost::IShell *_service;
            std::mutex _notificationMutex;

            //Iarm related
            static IarmBusImpl* _iarmImpl;
            static std::list<Exchange::IIarm::INotification*> _iarmNotificationCallbacks;

            //Proc related
            static readprocImpl* _procImpl;
            static RBusApiImpl* _rbusImpl;
            static RfcApiImpl* _rfcImpl;
        };
    }
}
