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

#include "btmgr.h"
#include <gmock/gmock.h>

BtmgrImpl* Btmgr::impl = nullptr;

Btmgr::Btmgr() {}

 void Btmgr::setImpl(BtmgrImpl* newImpl)
{
    // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}


BTRMGR_Result_t Btmgr::BTRMGR_RegisterForCallbacks(const char* apcProcessName)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_RegisterForCallbacks(apcProcessName);
}

BTRMGR_Result_t Btmgr::BTRMGR_UnRegisterFromCallbacks(const char* apcProcessName)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_UnRegisterFromCallbacks(apcProcessName);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetNumberOfAdapters(unsigned char *pNumOfAdapters)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetNumberOfAdapters(pNumOfAdapters);
}

BTRMGR_Result_t Btmgr::BTRMGR_SetAdapterName(unsigned char aui8AdapterIdx, const char* pNameOfAdapter)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_SetAdapterName(aui8AdapterIdx,pNameOfAdapter);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetAdapterName(unsigned char aui8AdapterIdx, char* pNameOfAdapter)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetAdapterName(aui8AdapterIdx,pNameOfAdapter);
}

BTRMGR_Result_t Btmgr::BTRMGR_SetAdapterPowerStatus(unsigned char aui8AdapterIdx, unsigned char power_status)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_SetAdapterPowerStatus(aui8AdapterIdx, power_status);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetAdapterPowerStatus(unsigned char aui8AdapterIdx, unsigned char *pPowerStatus)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetAdapterPowerStatus(aui8AdapterIdx, pPowerStatus);
}

BTRMGR_Result_t Btmgr::BTRMGR_SetAdapterDiscoverable(unsigned char aui8AdapterIdx, unsigned char discoverable, int timeout)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_SetAdapterDiscoverable(aui8AdapterIdx, discoverable, timeout);
}

BTRMGR_Result_t Btmgr::BTRMGR_IsAdapterDiscoverable(unsigned char aui8AdapterIdx, unsigned char *pDiscoverable)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_IsAdapterDiscoverable(aui8AdapterIdx, pDiscoverable);
}

BTRMGR_Result_t Btmgr::BTRMGR_StartDeviceDiscovery(unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_StartDeviceDiscovery(aui8AdapterIdx, aenBTRMgrDevOpT);
}

BTRMGR_Result_t Btmgr::BTRMGR_StopDeviceDiscovery(unsigned char aui8AdapterIdx, BTRMGR_DeviceOperationType_t aenBTRMgrDevOpT)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_StopDeviceDiscovery(aui8AdapterIdx, aenBTRMgrDevOpT);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetDiscoveredDevices(unsigned char aui8AdapterIdx, BTRMGR_DiscoveredDevicesList_t *pDiscoveredDevices)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetDiscoveredDevices(aui8AdapterIdx, pDiscoveredDevices);
}

BTRMGR_Result_t Btmgr::BTRMGR_PairDevice(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_PairDevice(aui8AdapterIdx, ahBTRMgrDevHdl);
}

BTRMGR_Result_t Btmgr::BTRMGR_UnpairDevice(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_UnpairDevice(aui8AdapterIdx, ahBTRMgrDevHdl);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetPairedDevices(unsigned char aui8AdapterIdx, BTRMGR_PairedDevicesList_t *pPairedDevices)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetPairedDevices(aui8AdapterIdx, pPairedDevices);
}

BTRMGR_Result_t Btmgr::BTRMGR_ConnectToDevice(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t connectAs)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_ConnectToDevice(aui8AdapterIdx, ahBTRMgrDevHdl, connectAs);
}

BTRMGR_Result_t Btmgr::BTRMGR_DisconnectFromDevice(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_DisconnectFromDevice(aui8AdapterIdx, ahBTRMgrDevHdl);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetConnectedDevices(unsigned char aui8AdapterIdx, BTRMGR_ConnectedDevicesList_t *pConnectedDevices)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetConnectedDevices(aui8AdapterIdx, pConnectedDevices);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetDeviceProperties(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DevicesProperty_t *pDeviceProperty)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetDeviceProperties(aui8AdapterIdx, ahBTRMgrDevHdl, pDeviceProperty);
}

BTRMGR_Result_t Btmgr::BTRMGR_StartAudioStreamingOut(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t connectAs)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_StartAudioStreamingOut(aui8AdapterIdx, ahBTRMgrDevHdl, connectAs);
}

BTRMGR_Result_t Btmgr::BTRMGR_StopAudioStreamingOut(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_StopAudioStreamingOut(aui8AdapterIdx, ahBTRMgrDevHdl);
}

BTRMGR_Result_t Btmgr::BTRMGR_SetAudioStreamingOutType(unsigned char aui8AdapterIdx, BTRMGR_StreamOut_Type_t type)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_SetAudioStreamingOutType(aui8AdapterIdx, type);
}

BTRMGR_Result_t Btmgr::BTRMGR_StartAudioStreamingIn(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t connectAs)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_StartAudioStreamingIn(aui8AdapterIdx, ahBTRMgrDevHdl, connectAs);
}

BTRMGR_Result_t Btmgr::BTRMGR_StopAudioStreamingIn(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_StopAudioStreamingIn(aui8AdapterIdx, ahBTRMgrDevHdl);
}

BTRMGR_Result_t Btmgr::BTRMGR_SetEventResponse(unsigned char aui8AdapterIdx, BTRMGR_EventResponse_t* apstBTRMgrEvtRsp)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_SetEventResponse(aui8AdapterIdx, apstBTRMgrEvtRsp);
}

BTRMGR_Result_t Btmgr::BTRMGR_MediaControl(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_MediaControlCommand_t mediaCtrlCmd)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_MediaControl(aui8AdapterIdx, ahBTRMgrDevHdl, mediaCtrlCmd);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetDeviceVolumeMute(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t deviceOpType, unsigned char *pui8Volume, unsigned char *pui8Mute)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetDeviceVolumeMute(aui8AdapterIdx, ahBTRMgrDevHdl, deviceOpType, pui8Volume, pui8Mute);
}

BTRMGR_Result_t Btmgr::BTRMGR_SetDeviceVolumeMute(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_DeviceOperationType_t deviceOpType, unsigned char ui8Volume, unsigned char ui8Mute)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_SetDeviceVolumeMute(aui8AdapterIdx, ahBTRMgrDevHdl, deviceOpType, ui8Volume, ui8Mute);
}

BTRMGR_Result_t Btmgr::BTRMGR_GetMediaTrackInfo(unsigned char aui8AdapterIdx, BTRMgrDeviceHandle ahBTRMgrDevHdl, BTRMGR_MediaTrackInfo_t *mediaTrackInfo)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetMediaTrackInfo(aui8AdapterIdx, ahBTRMgrDevHdl, mediaTrackInfo);
}

const char* Btmgr::BTRMGR_GetDeviceTypeAsString(BTRMGR_DeviceType_t type)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_GetDeviceTypeAsString(type);
}

BTRMGR_Result_t Btmgr::BTRMGR_RegisterEventCallback(BTRMGR_EventCallback afpcBBTRMgrEventOut)
{
    EXPECT_NE(impl, nullptr);
    return impl->BTRMGR_RegisterEventCallback(afpcBBTRMgrEventOut);
}

BTRMGR_Result_t (*BTRMGR_RegisterForCallbacks)(const char*) = &Btmgr::BTRMGR_RegisterForCallbacks;
BTRMGR_Result_t (*BTRMGR_UnRegisterFromCallbacks)(const char*) = &Btmgr::BTRMGR_UnRegisterFromCallbacks;
BTRMGR_Result_t (*BTRMGR_GetNumberOfAdapters)(unsigned char *) = &Btmgr::BTRMGR_GetNumberOfAdapters;
BTRMGR_Result_t (*BTRMGR_SetAdapterName)(unsigned char, const char*) = &Btmgr::BTRMGR_SetAdapterName;
BTRMGR_Result_t (*BTRMGR_GetAdapterName)(unsigned char, char*) = &Btmgr::BTRMGR_GetAdapterName;
BTRMGR_Result_t (*BTRMGR_SetAdapterPowerStatus)(unsigned char, unsigned char) = &Btmgr::BTRMGR_SetAdapterPowerStatus;
BTRMGR_Result_t (*BTRMGR_GetAdapterPowerStatus)(unsigned char, unsigned char *) = &Btmgr::BTRMGR_GetAdapterPowerStatus;
BTRMGR_Result_t (*BTRMGR_SetAdapterDiscoverable)(unsigned char, unsigned char, int) = &Btmgr::BTRMGR_SetAdapterDiscoverable;
BTRMGR_Result_t (*BTRMGR_IsAdapterDiscoverable)(unsigned char, unsigned char *) = &Btmgr::BTRMGR_IsAdapterDiscoverable;
BTRMGR_Result_t (*BTRMGR_StartDeviceDiscovery)(unsigned char, BTRMGR_DeviceOperationType_t) = &Btmgr::BTRMGR_StartDeviceDiscovery;
BTRMGR_Result_t (*BTRMGR_StopDeviceDiscovery)(unsigned char, BTRMGR_DeviceOperationType_t) = &Btmgr::BTRMGR_StopDeviceDiscovery;
BTRMGR_Result_t (*BTRMGR_GetDiscoveredDevices)(unsigned char, BTRMGR_DiscoveredDevicesList_t *) = &Btmgr::BTRMGR_GetDiscoveredDevices;
BTRMGR_Result_t (*BTRMGR_PairDevice)(unsigned char, BTRMgrDeviceHandle) = &Btmgr::BTRMGR_PairDevice;
BTRMGR_Result_t (*BTRMGR_UnpairDevice)(unsigned char, BTRMgrDeviceHandle) = &Btmgr::BTRMGR_UnpairDevice;
BTRMGR_Result_t (*BTRMGR_GetPairedDevices)(unsigned char, BTRMGR_PairedDevicesList_t *) = &Btmgr::BTRMGR_GetPairedDevices;
BTRMGR_Result_t (*BTRMGR_ConnectToDevice)(unsigned char, BTRMgrDeviceHandle, BTRMGR_DeviceOperationType_t) = &Btmgr::BTRMGR_ConnectToDevice;
BTRMGR_Result_t (*BTRMGR_DisconnectFromDevice)(unsigned char, BTRMgrDeviceHandle) = &Btmgr::BTRMGR_DisconnectFromDevice;
BTRMGR_Result_t (*BTRMGR_GetConnectedDevices)(unsigned char, BTRMGR_ConnectedDevicesList_t *) = &Btmgr::BTRMGR_GetConnectedDevices;
BTRMGR_Result_t (*BTRMGR_GetDeviceProperties)(unsigned char, BTRMgrDeviceHandle, BTRMGR_DevicesProperty_t *) = &Btmgr::BTRMGR_GetDeviceProperties;
BTRMGR_Result_t (*BTRMGR_StartAudioStreamingOut)(unsigned char, BTRMgrDeviceHandle, BTRMGR_DeviceOperationType_t) = &Btmgr::BTRMGR_StartAudioStreamingOut;
BTRMGR_Result_t (*BTRMGR_StopAudioStreamingOut)(unsigned char , BTRMgrDeviceHandle) = &Btmgr::BTRMGR_StopAudioStreamingOut;
BTRMGR_Result_t (*BTRMGR_SetAudioStreamingOutType)(unsigned char, BTRMGR_StreamOut_Type_t) = &Btmgr::BTRMGR_SetAudioStreamingOutType;
BTRMGR_Result_t (*BTRMGR_StartAudioStreamingIn)(unsigned char, BTRMgrDeviceHandle, BTRMGR_DeviceOperationType_t) = &Btmgr::BTRMGR_StartAudioStreamingIn;
BTRMGR_Result_t (*BTRMGR_StopAudioStreamingIn)(unsigned char, BTRMgrDeviceHandle) = &Btmgr::BTRMGR_StopAudioStreamingIn;
BTRMGR_Result_t (*BTRMGR_SetEventResponse)(unsigned char, BTRMGR_EventResponse_t*) = &Btmgr::BTRMGR_SetEventResponse;
BTRMGR_Result_t (*BTRMGR_MediaControl)(unsigned char, BTRMgrDeviceHandle, BTRMGR_MediaControlCommand_t) = &Btmgr::BTRMGR_MediaControl;
BTRMGR_Result_t (*BTRMGR_GetDeviceVolumeMute)(unsigned char, BTRMgrDeviceHandle, BTRMGR_DeviceOperationType_t, unsigned char *, unsigned char *) = &Btmgr::BTRMGR_GetDeviceVolumeMute;
BTRMGR_Result_t (*BTRMGR_SetDeviceVolumeMute)(unsigned char, BTRMgrDeviceHandle, BTRMGR_DeviceOperationType_t, unsigned char, unsigned char) = &Btmgr::BTRMGR_SetDeviceVolumeMute;
BTRMGR_Result_t (*BTRMGR_GetMediaTrackInfo)(unsigned char, BTRMgrDeviceHandle, BTRMGR_MediaTrackInfo_t *) = &Btmgr::BTRMGR_GetMediaTrackInfo;
const char* (*BTRMGR_GetDeviceTypeAsString)(BTRMGR_DeviceType_t) = &Btmgr::BTRMGR_GetDeviceTypeAsString;
BTRMGR_Result_t (*BTRMGR_RegisterEventCallback)(BTRMGR_EventCallback) = &Btmgr::BTRMGR_RegisterEventCallback;
