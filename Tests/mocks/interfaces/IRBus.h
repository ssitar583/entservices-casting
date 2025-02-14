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
        struct EXTERNAL IRBus : virtual public Core::IUnknown {
            //TODO: to replace the magic number with ID_MOCK_PLUGIN in Ids.h at the completion of L2 OOP.
            enum { ID = 0xA21 };

            // Declare the methods to be implemented by the plugin
            virtual uint32_t rbus_close(const std::string& handle) = 0;

            virtual ~IRBus() = default;
        };
    }
}
