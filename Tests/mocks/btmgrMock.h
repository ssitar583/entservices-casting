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

#include "btmgr.h"

class BtmgrImplMock : public BtmgrImpl {
public:
    virtual ~BtmgrImplMock() = default;
    
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_RegisterForCallbacks, (const char* apcProcessName), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_UnRegisterFromCallbacks, (const char* apcProcessName), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetNumberOfAdapters, (unsigned char *pNumOfAdapters), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_SetAdapterName, (unsigned char aui8AdapterIdx, const char* pNameOfAdapter), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetAdapterName, (unsigned char aui8AdapterIdx, char* pNameOfAdapter), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_SetAdapterPowerStatus, (unsigned char aui8AdapterIdx, unsigned char power_status), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetAdapterPowerStatus, (unsigned char aui8AdapterIdx, unsigned char *pPowerStatus), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_SetAdapterDiscoverable, (unsigned char aui8AdapterIdx, unsigned char discoverable, int timeout), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_IsAdapterDiscoverable, (unsigned char aui8AdapterIdx, unsigned char *pDiscoverable), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_StartDeviceDiscovery, (unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_StopDeviceDiscovery, (unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetDiscoveredDevices, (unsigned char aui8AdapterIdx, BTRMGR_DiscoveredDevicesList_t *pDiscoveredDevices), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_PairDevice, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_UnpairDevice, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetPairedDevices, (unsigned char aui8AdapterIdx, BTRMGR_PairedDevicesList_t *pPairedDevices), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_ConnectToDevice, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t connectAs), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_DisconnectFromDevice, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetConnectedDevices, (unsigned char aui8AdapterIdx, BTRMGR_ConnectedDevicesList_t *pConnectedDevices), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetDeviceProperties, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DevicesProperty_t *pDeviceProperty), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_StartAudioStreamingOut, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t connectAs), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_StopAudioStreamingOut, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_SetAudioStreamingOutType, (unsigned char aui8AdapterIdx, BTRMGR_StreamOut_Type_t type), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_StartAudioStreamingIn, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t connectAs), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_StopAudioStreamingIn, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_SetEventResponse, (unsigned char aui8AdapterIdx, BTRMGR_EventResponse_t* apstBTRMgrEvtRsp), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_MediaControl, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_MediaControlCommand_t mediaCtrlCmd), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetDeviceVolumeMute, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t deviceOpType, unsigned char *pui8Volume, unsigned char *pui8Mute), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_SetDeviceVolumeMute, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t deviceOpType, unsigned char ui8Volume, unsigned char ui8Mute), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_GetMediaTrackInfo, (unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_MediaTrackInfo_t *mediaTrackInfo), (override));
    MOCK_METHOD(const char*, BTRMGR_GetDeviceTypeAsString, (BTRMGR_DeviceType_t type), (override));
    MOCK_METHOD(BTRMGR_Result_t, BTRMGR_RegisterEventCallback, (BTRMGR_EventCallback afpcBBTRMgrEventOut), (override));
};
// End of btmgrMock.h
