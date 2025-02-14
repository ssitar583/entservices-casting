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
#include "tvSettings.h"  // Include the original header for function signatures
#include "tvSettingsODM.h"
#include "tvSettingsExtODM.h"  

class TvSettingsImplMock : public TvSettingsImpl{
public:
    virtual ~TvSettingsImplMock() = default;

    MOCK_METHOD(tvError_t, TvInit, (), ());
    MOCK_METHOD(tvError_t, TvTerm, (), ());
    MOCK_METHOD(tvError_t, RegisterVideoFormatChangeCB, (tvVideoFormatCallbackData* cbData), ());
    MOCK_METHOD(tvError_t, GetCurrentVideoFormat, (tvVideoFormatType_t* videoFormat), ());
    MOCK_METHOD(tvError_t, RegisterVideoContentChangeCB, (tvVideoContentCallbackData* cbData), ());
    MOCK_METHOD(tvError_t, RegisterVideoResolutionChangeCB, (tvVideoResolutionCallbackData* cbData), ());
    MOCK_METHOD(tvError_t, RegisterVideoFrameRateChangeCB, (tvVideoFrameRateCallbackData* cbData), ());
    MOCK_METHOD(tvError_t, GetTVSupportedVideoFormats, (tvVideoFormatType_t* videoFormats[], unsigned short* numberOfFormats), ());
    MOCK_METHOD(tvError_t, GetCurrentVideoResolution, (tvResolutionParam_t* res), ());
    MOCK_METHOD(tvError_t, GetCurrentVideoFrameRate, (tvVideoFrameRate_t* format), ());
    MOCK_METHOD(tvError_t, GetBacklight, (int* backlight), ());
    MOCK_METHOD(tvError_t, SetBacklight, (int backlight), ());
    MOCK_METHOD(tvError_t, SaveBacklight, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value), ());
    MOCK_METHOD(tvError_t, SetTVDimmingMode, (const char* dimmingMode), ());
    MOCK_METHOD(tvError_t, GetTVDimmingMode, (char* dimmingMode), ());
    MOCK_METHOD(tvError_t, SaveTVDimmingMode, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDimmingMode_t dimmingMode), ());
    MOCK_METHOD(tvError_t, SetBrightness, (int brightness), ());
    MOCK_METHOD(tvError_t, GetBrightness, (int* brightness), ());
    MOCK_METHOD(tvError_t, SaveBrightness, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value), ());
    MOCK_METHOD(tvError_t, SetContrast, (int contrast), ());
    MOCK_METHOD(tvError_t, GetContrast, (int* contrast), ());
    MOCK_METHOD(tvError_t, SaveContrast, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value), ());
    MOCK_METHOD(tvError_t, SetSharpness, (int sharpness), ());
    MOCK_METHOD(tvError_t, GetSharpness, (int* sharpness), ());
    MOCK_METHOD(tvError_t, SaveSharpness, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value), ());
    MOCK_METHOD(tvError_t, SetSaturation, (int saturation), ());
    MOCK_METHOD(tvError_t, GetSaturation, (int* saturation), ());
    MOCK_METHOD(tvError_t, SaveSaturation, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value), ());
    MOCK_METHOD(tvError_t, SetHue, (int hue), ());
    MOCK_METHOD(tvError_t, GetHue, (int* hue), ());
    MOCK_METHOD(tvError_t, SaveHue, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value), ());
    MOCK_METHOD(tvError_t, SetColorTemperature, (tvColorTemp_t colorTemp), ());
    MOCK_METHOD(tvError_t, GetColorTemperature, (tvColorTemp_t* colorTemp), ());
    MOCK_METHOD(tvError_t, SaveColorTemperature, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvColorTemp_t value), ());
    MOCK_METHOD(tvError_t, SetAspectRatio, (tvDisplayMode_t dispMode), ());
    MOCK_METHOD(tvError_t, GetAspectRatio, (tvDisplayMode_t* dispMode), ());
    MOCK_METHOD(tvError_t, SaveAspectRatio, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDisplayMode_t value), ());
    MOCK_METHOD(tvError_t, SetLowLatencyState, (int lowLatencyIndex), ());
    MOCK_METHOD(tvError_t, GetLowLatencyState, (int* lowlatencystate), ());
    MOCK_METHOD(tvError_t, SaveLowLatencyState, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value), ());
    MOCK_METHOD(tvError_t, GetTVSupportedDolbyVisionModes, (tvDolbyMode_t* dvModes[], unsigned short* count), ());
    MOCK_METHOD(tvError_t, SetTVDolbyVisionMode, (tvDolbyMode_t dolbyMode), ());
    MOCK_METHOD(tvError_t, SaveTVDolbyVisionMode, (tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDolbyMode_t value), ());
    MOCK_METHOD(tvError_t, GetTVSupportedPictureModes, (pic_modes_t* pictureModes[], unsigned short* count), ());
    MOCK_METHOD(tvError_t, GetTVPictureMode, (char* pictureMode), ());
    MOCK_METHOD(tvError_t, SetTVPictureMode, (const char* pictureMode), ());
    MOCK_METHOD(tvError_t, SaveSourcePictureMode, (tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, int pictureMode), ());
    MOCK_METHOD(tvError_t, GetDefaultPQParams, (int pqIndex, tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, tvPQParameterIndex_t pqParamIndex, int* value), ());
    
    // MOCK_METHOD(void, tvVideoFormatChangeCB, (tvVideoFormatType_t format, void *userData));
    // MOCK_METHOD(void, tvVideoContentChangeCB, (tvContentType_t mode,void *userData));
    // MOCK_METHOD(void, tvVideoResolutionChangeCB, (tvResolutionParam_t resolutionStruct,void *userData));
    // MOCK_METHOD(void, tvVideoFrameRateChangeCB, (tvVideoFrameRate_t frameRate,void *userData));
    //tvSettingsODM.h
    MOCK_METHOD(int, ConvertHDRFormatToContentFormatODM, (tvhdr_type_t hdrFormat), ());
    MOCK_METHOD(tvError_t, SetWBctrl, (char* inputSrc, char* colorTemp, char* color, char* ctrl, int value), ());
    MOCK_METHOD(int, GetPanelIDODM, (char* panelId), ());
    MOCK_METHOD(tvError_t, TvSyncCalibrationInfoODM, (), ());
    MOCK_METHOD(tvError_t, GetTVSupportedDolbyVisionModesODM, (pic_modes_t* dvModes[], unsigned short* count), ());
    MOCK_METHOD(tvError_t, SetTVDolbyVisionModeODM, (const char* dolbyMode), ());
    MOCK_METHOD(tvError_t, GetCurrentSource, (tvVideoSrcType_t* currentSource), ());

    // tvSettingsExtODM.h
    MOCK_METHOD(int,ReadCapablitiesFromConfODM, (std::string& rangeInfo, std::string& pqmodeInfo, std::string& formatInfo, std::string& sourceInfo, std::string param, std::string& platformsupport, std::string& index), ());

};