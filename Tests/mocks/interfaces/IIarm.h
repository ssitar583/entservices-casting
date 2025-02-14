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

namespace WPEFramework {
    namespace Exchange {

        //This is the COM-RPC interface for the Mock Plugin
        //The requirement to make an interface a COMRPC interface is that it should inherit
        //from Core::IUnknown
        struct EXTERNAL IIarm : virtual public Core::IUnknown {
            //TODO: to replace the magic number with ID_MOCK_PLUGIN in Ids.h at the completion of L2 OOP.
            enum { ID = 0xA01 };

            // The Notification interface, containing the virtual functions to be
            // implemented by any listener
            // to receive notifications from the plugin
            struct EXTERNAL INotification : virtual public Core::IUnknown {
                //TODO: to replace the magic number with ID_MOCK_PLUGIN_NOTIFICATION in Ids.h at the completion of L2 OOP.
                enum { ID = 0xA02 };

                virtual  uint32_t IARM_EventHandler(const std::string& ownerName, int eventId, const std::string& data) = 0;
                virtual ~INotification() = default;
            };

            // Declare the methods to be implemented by the plugin
            virtual uint32_t IARM_Bus_IsConnected(const string& message /* @in */, int& result /* @out */) = 0;
            virtual uint32_t IARM_Bus_Init(const string& message/* @in */) = 0;
            virtual uint32_t IARM_Bus_Connect() = 0;

            virtual uint32_t IARM_Bus_RegisterEventHandler(const string& ownerName /* @in */, int eventId /* @in */) = 0;
            virtual uint32_t IARM_Bus_RemoveEventHandler(const string& ownerName /* @in */, int eventId /* @in */) = 0;
            virtual uint32_t IARM_Bus_Call(const string& ownerName /* @in */, const string& methodName /* @in */, uint8_t* arg /* @in @out @length:argLen */, uint32_t argLen ) = 0;
            virtual uint32_t IARM_Bus_BroadcastEvent(const string& ownerName /* @in */, int eventId /* @in */, uint8_t *arg /* @in @out @length:argLen */, uint32_t argLen ) = 0;
            virtual uint32_t IARM_Bus_CallWithIPCTimeout(const string& ownerName /* @in */, const string& methodName /* @in */, uint8_t *arg /* @out @length:argLen */, uint32_t argLen, int timeout /* @in */) = 0;
            virtual uint32_t IARM_Bus_RegisterCall(const string& methodName /* @in */, const uint8_t* handler/* @length:4 */) = 0;


            // Declare the methods to register for notifications
            virtual uint32_t Register(INotification* notification /* @in */) = 0;
            virtual uint32_t Unregister(INotification* notification /* @in */) = 0;
            virtual ~IIarm() = default;
        };
    }
}
