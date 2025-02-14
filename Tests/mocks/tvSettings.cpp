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

#include "tvSettings.h"
#include <gmock/gmock.h>

TvSettingsImpl* TvSettings::impl = nullptr;

TvSettings::TvSettings() {}

void TvSettings::setImpl(TvSettingsImpl* newImpl)
{
    // Ensures that we can either reset 'impl' to nullptr or assign a new value
    EXPECT_TRUE((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

tvError_t TvSettings::TvInit()
{
    EXPECT_NE(impl, nullptr);
    return impl->TvInit();
}

tvError_t TvSettings::TvTerm()
{
    EXPECT_NE(impl, nullptr);
    return impl->TvTerm();
}

tvError_t TvSettings::TvSyncCalibrationInfoODM()
{
    EXPECT_NE(impl, nullptr);
    return impl->TvTerm();
}

tvError_t TvSettings::RegisterVideoFormatChangeCB(tvVideoFormatCallbackData *cbData)
{
    EXPECT_NE(impl, nullptr);
    return impl->RegisterVideoFormatChangeCB(cbData);
}

tvError_t TvSettings::RegisterVideoContentChangeCB(tvVideoContentCallbackData *cbData)
{
    EXPECT_NE(impl, nullptr);
    return impl->RegisterVideoContentChangeCB(cbData);
}

tvError_t TvSettings::RegisterVideoResolutionChangeCB(tvVideoResolutionCallbackData *cbData)
{
    EXPECT_NE(impl, nullptr);
    return impl->RegisterVideoResolutionChangeCB(cbData);
}

tvError_t TvSettings::RegisterVideoFrameRateChangeCB(tvVideoFrameRateCallbackData *cbData)
{
    EXPECT_NE(impl, nullptr);
    return impl->RegisterVideoFrameRateChangeCB(cbData);
}

tvError_t TvSettings::GetTVSupportedVideoFormats(tvVideoFormatType_t **videoFormats, unsigned short *numberOfFormats)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetTVSupportedVideoFormats(videoFormats, numberOfFormats);
}

tvError_t TvSettings::GetCurrentVideoFormat(tvVideoFormatType_t *videoFormat)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetCurrentVideoFormat(videoFormat);
}

tvError_t TvSettings::GetCurrentVideoResolution(tvResolutionParam_t *res)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetCurrentVideoResolution(res);
}

tvError_t TvSettings::GetCurrentVideoFrameRate(tvVideoFrameRate_t *format)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetCurrentVideoFrameRate(format);
}

tvError_t TvSettings::GetBacklight(int *backlight)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetBacklight(backlight);
}

tvError_t TvSettings::SetBacklight(int backlight)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetBacklight(backlight);
}

tvError_t TvSettings::SaveBacklight(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveBacklight(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetTVDimmingMode(const char *dimmingMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetTVDimmingMode(dimmingMode);
}

tvError_t TvSettings::GetTVDimmingMode(char *dimmingMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetTVDimmingMode(dimmingMode);
}

tvError_t TvSettings::SaveTVDimmingMode(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDimmingMode_t dimmingMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveTVDimmingMode(videoSrcType, pq_mode, videoFormatType, dimmingMode);
}

tvError_t TvSettings::SetBrightness(int brightness)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetBrightness(brightness);
}

tvError_t TvSettings::GetBrightness(int *brightness)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetBrightness(brightness);
}

tvError_t TvSettings::SaveBrightness(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveBrightness(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetContrast(int contrast)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetContrast(contrast);
}

tvError_t TvSettings::GetContrast(int *contrast)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetContrast(contrast);
}

tvError_t TvSettings::SaveContrast(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveContrast(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetSharpness(int sharpness)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetSharpness(sharpness);
}

tvError_t TvSettings::GetSharpness(int *sharpness)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetSharpness(sharpness);
}

tvError_t TvSettings::SaveSharpness(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveSharpness(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetSaturation(int saturation)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetSaturation(saturation);
}

tvError_t TvSettings::GetSaturation(int *saturation)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetSaturation(saturation);
}

tvError_t TvSettings::SaveSaturation(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveSaturation(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetHue(int hue)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetHue(hue);
}

tvError_t TvSettings::GetHue(int *hue)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetHue(hue);
}

tvError_t TvSettings::SaveHue(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveHue(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetColorTemperature(tvColorTemp_t colorTemp)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetColorTemperature(colorTemp);
}

tvError_t TvSettings::GetColorTemperature(tvColorTemp_t *colorTemp)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetColorTemperature(colorTemp);
}

tvError_t TvSettings::SaveColorTemperature(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvColorTemp_t value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveColorTemperature(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetAspectRatio(tvDisplayMode_t dispMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetAspectRatio(dispMode);
}

tvError_t TvSettings::GetAspectRatio(tvDisplayMode_t *dispMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetAspectRatio(dispMode);
}

tvError_t TvSettings::SaveAspectRatio(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDisplayMode_t value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveAspectRatio(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::SetLowLatencyState(int lowLatencyIndex)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetLowLatencyState(lowLatencyIndex);
}

tvError_t TvSettings::GetLowLatencyState(int *lowLatencyState)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetLowLatencyState(lowLatencyState);
}

tvError_t TvSettings::SaveLowLatencyState(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveLowLatencyState(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::GetTVSupportedDolbyVisionModes(tvDolbyMode_t **dvModes, unsigned short *count)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetTVSupportedDolbyVisionModes(dvModes, count);
}

tvError_t TvSettings::SetTVDolbyVisionMode(tvDolbyMode_t dolbyMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetTVDolbyVisionMode(dolbyMode);
}

tvError_t TvSettings::SaveTVDolbyVisionMode(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDolbyMode_t value)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveTVDolbyVisionMode(videoSrcType, pq_mode, videoFormatType, value);
}

tvError_t TvSettings::GetTVSupportedPictureModes(pic_modes_t **pictureModes, unsigned short *count)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetTVSupportedPictureModes(pictureModes, count);
}

tvError_t TvSettings::GetTVPictureMode(char *pictureMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetTVPictureMode(pictureMode);
}

tvError_t TvSettings::SetTVPictureMode(const char *pictureMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetTVPictureMode(pictureMode);
}

tvError_t TvSettings::SaveSourcePictureMode(tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, int pictureMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->SaveSourcePictureMode(videoSrcType, videoFormatType, pictureMode);
}

tvError_t TvSettings::GetDefaultPQParams(int pqIndex, tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, tvPQParameterIndex_t pqParamIndex, int *value)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetDefaultPQParams(pqIndex, videoSrcType, videoFormatType, pqParamIndex, value);
}

int TvSettings::ReadCapablitiesFromConfODM(std::string& rangeInfo, std::string& pqmodeInfo, std::string& formatInfo, std::string& sourceInfo, std::string param, std::string& platformsupport, std::string& index)
{
    EXPECT_NE(impl, nullptr);
    return impl->ReadCapablitiesFromConfODM(rangeInfo, pqmodeInfo, formatInfo, sourceInfo, param, platformsupport, index);
}

int TvSettings::GetPanelIDODM(char* panelid)
{
    printf("Enter GetPanelIDODM \n");
    EXPECT_NE(impl, nullptr);
    return impl->GetPanelIDODM(panelid);
}

tvError_t TvSettings::GetCurrentSource(tvVideoSrcType_t *currentSource)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetCurrentSource(currentSource );
}

tvError_t TvSettings::GetTVSupportedDolbyVisionModesODM(pic_modes_t** dvModes,unsigned short *count)
{
    EXPECT_NE(impl, nullptr);
    return impl->GetTVSupportedDolbyVisionModesODM(dvModes,count);
}

tvError_t TvSettings::SetTVDolbyVisionModeODM(const char * dolbyMode)
{
    EXPECT_NE(impl, nullptr);
    return impl->SetTVDolbyVisionModeODM(dolbyMode);
}

int TvSettings::ConvertHDRFormatToContentFormatODM(tvhdr_type_t hdrFormat)
{
    EXPECT_NE(impl, nullptr);
    return impl->ConvertHDRFormatToContentFormatODM(hdrFormat);
}

// Function pointer assignments
tvError_t (*TvInit)() = &TvSettings::TvInit;
tvError_t (*TvTerm)() = &TvSettings::TvTerm;
tvError_t (*RegisterVideoFormatChangeCB)(tvVideoFormatCallbackData*) = &TvSettings::RegisterVideoFormatChangeCB;
tvError_t (*RegisterVideoContentChangeCB)(tvVideoContentCallbackData*) = &TvSettings::RegisterVideoContentChangeCB;
tvError_t (*RegisterVideoResolutionChangeCB)(tvVideoResolutionCallbackData*) = &TvSettings::RegisterVideoResolutionChangeCB;
tvError_t (*RegisterVideoFrameRateChangeCB)(tvVideoFrameRateCallbackData*) = &TvSettings::RegisterVideoFrameRateChangeCB;
tvError_t (*GetTVSupportedVideoFormats)(tvVideoFormatType_t**, unsigned short*) = &TvSettings::GetTVSupportedVideoFormats;
tvError_t (*GetCurrentVideoFormat)(tvVideoFormatType_t*) = &TvSettings::GetCurrentVideoFormat;
tvError_t (*GetCurrentVideoResolution)(tvResolutionParam_t*) = &TvSettings::GetCurrentVideoResolution;
tvError_t (*GetCurrentVideoFrameRate)(tvVideoFrameRate_t*) = &TvSettings::GetCurrentVideoFrameRate;
tvError_t (*GetBacklight)(int*) = &TvSettings::GetBacklight;
tvError_t (*SetBacklight)(int) = &TvSettings::SetBacklight;
tvError_t (*SaveBacklight)(tvVideoSrcType_t, int, tvVideoFormatType_t, int) = &TvSettings::SaveBacklight;
tvError_t (*SetTVDimmingMode)(const char*) = &TvSettings::SetTVDimmingMode;
tvError_t (*GetTVDimmingMode)(char*) = &TvSettings::GetTVDimmingMode;
tvError_t (*SaveTVDimmingMode)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvDimmingMode_t) = &TvSettings::SaveTVDimmingMode;
tvError_t (*SetBrightness)(int) = &TvSettings::SetBrightness;
tvError_t (*GetBrightness)(int*) = &TvSettings::GetBrightness;
tvError_t (*SaveBrightness)(tvVideoSrcType_t, int, tvVideoFormatType_t, int) = &TvSettings::SaveBrightness;
tvError_t (*SetContrast)(int) = &TvSettings::SetContrast;
tvError_t (*GetContrast)(int*) = &TvSettings::GetContrast;
tvError_t (*SaveContrast)(tvVideoSrcType_t, int, tvVideoFormatType_t, int) = &TvSettings::SaveContrast;
tvError_t (*SetSharpness)(int) = &TvSettings::SetSharpness;
tvError_t (*GetSharpness)(int*) = &TvSettings::GetSharpness;
tvError_t (*SaveSharpness)(tvVideoSrcType_t, int, tvVideoFormatType_t, int) = &TvSettings::SaveSharpness;
tvError_t (*SetSaturation)(int) = &TvSettings::SetSaturation;
tvError_t (*GetSaturation)(int*) = &TvSettings::GetSaturation;
tvError_t (*SaveSaturation)(tvVideoSrcType_t, int, tvVideoFormatType_t, int) = &TvSettings::SaveSaturation;
tvError_t (*SetHue)(int) = &TvSettings::SetHue;
tvError_t (*GetHue)(int*) = &TvSettings::GetHue;
tvError_t (*SaveHue)(tvVideoSrcType_t, int, tvVideoFormatType_t, int) = &TvSettings::SaveHue;
tvError_t (*SetColorTemperature)(tvColorTemp_t) = &TvSettings::SetColorTemperature;
tvError_t (*GetColorTemperature)(tvColorTemp_t*) = &TvSettings::GetColorTemperature;
tvError_t (*SaveColorTemperature)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvColorTemp_t) = &TvSettings::SaveColorTemperature;
tvError_t (*SetAspectRatio)(tvDisplayMode_t) = &TvSettings::SetAspectRatio;
tvError_t (*GetAspectRatio)(tvDisplayMode_t*) = &TvSettings::GetAspectRatio;
tvError_t (*SaveAspectRatio)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvDisplayMode_t) = &TvSettings::SaveAspectRatio;
tvError_t (*SetLowLatencyState)(int) = &TvSettings::SetLowLatencyState;
tvError_t (*GetLowLatencyState)(int*) = &TvSettings::GetLowLatencyState;
tvError_t (*SaveLowLatencyState)(tvVideoSrcType_t, int, tvVideoFormatType_t, int) = &TvSettings::SaveLowLatencyState;
tvError_t (*GetTVSupportedDolbyVisionModes)(tvDolbyMode_t**, unsigned short*) = &TvSettings::GetTVSupportedDolbyVisionModes;
tvError_t (*SetTVDolbyVisionMode)(tvDolbyMode_t) = &TvSettings::SetTVDolbyVisionMode;
tvError_t (*SaveTVDolbyVisionMode)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvDolbyMode_t) = &TvSettings::SaveTVDolbyVisionMode;
tvError_t (*GetTVSupportedPictureModes)(pic_modes_t**, unsigned short*) = &TvSettings::GetTVSupportedPictureModes;
tvError_t (*GetTVPictureMode)(char*) = &TvSettings::GetTVPictureMode;
tvError_t (*SetTVPictureMode)(const char*) = &TvSettings::SetTVPictureMode;
tvError_t (*SaveSourcePictureMode)(tvVideoSrcType_t, tvVideoFormatType_t, int) = &TvSettings::SaveSourcePictureMode;
tvError_t (*GetDefaultPQParams)(int, tvVideoSrcType_t, tvVideoFormatType_t, tvPQParameterIndex_t, int*) = &TvSettings::GetDefaultPQParams;
tvError_t (*TvSyncCalibrationInfoODM)() = &TvSettings::TvSyncCalibrationInfoODM;
tvError_t (*GetCurrentSource)(tvVideoSrcType_t *) = &TvSettings::GetCurrentSource;
int (*ReadCapablitiesFromConfODM)(std::string&, std::string&, std::string&, std::string&, std::string, std::string& , std::string& ) = &TvSettings::ReadCapablitiesFromConfODM;
int (*GetPanelIDODM)(char*) = &TvSettings::GetPanelIDODM;
tvError_t (*GetTVSupportedDolbyVisionModesODM)(pic_modes_t** dvModes,unsigned short *count) = &TvSettings::GetTVSupportedDolbyVisionModesODM;
tvError_t (*SetTVDolbyVisionModeODM)(const char * dolbyMode) = &TvSettings::SetTVDolbyVisionModeODM;
 int (*ConvertHDRFormatToContentFormatODM)(tvhdr_type_t hdrFormat) = &TvSettings::ConvertHDRFormatToContentFormatODM;
