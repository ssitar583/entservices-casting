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

// #ifndef _TV_SETTINGS_H
// #define _TV_SETTINGS_H

#include <stdbool.h>
#include "tvError.h"
#include "tvTypes.h"
#include "tvSettingsODM.h"
#include "tvSettingsExtODM.h"

class TvSettingsImpl {
public:
    virtual ~TvSettingsImpl() = default;

    virtual tvError_t TvInit() = 0;
    virtual tvError_t TvTerm() = 0;
    virtual tvError_t RegisterVideoFormatChangeCB(tvVideoFormatCallbackData* cbData) = 0;
    virtual tvError_t RegisterVideoContentChangeCB(tvVideoContentCallbackData* cbData) = 0;
    virtual tvError_t RegisterVideoResolutionChangeCB(tvVideoResolutionCallbackData* cbData) = 0;
    virtual tvError_t RegisterVideoFrameRateChangeCB(tvVideoFrameRateCallbackData* cbData) = 0;
    virtual tvError_t GetTVSupportedVideoFormats(tvVideoFormatType_t** videoFormats, unsigned short* numberOfFormats) = 0;
    virtual tvError_t GetCurrentVideoFormat(tvVideoFormatType_t* videoFormat) = 0;
    virtual tvError_t GetCurrentVideoResolution(tvResolutionParam_t* res) = 0;
    virtual tvError_t GetCurrentVideoFrameRate(tvVideoFrameRate_t* format) = 0;
    virtual tvError_t GetBacklight(int* backlight) = 0;
    virtual tvError_t SetBacklight(int backlight) = 0;
    virtual tvError_t SaveBacklight(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value) = 0;
    virtual tvError_t SetTVDimmingMode(const char* dimmingMode) = 0;
    virtual tvError_t GetTVDimmingMode(char* dimmingMode) = 0;
    virtual tvError_t SaveTVDimmingMode(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDimmingMode_t dimmingMode) = 0;
    virtual tvError_t SetBrightness(int brightness) = 0;
    virtual tvError_t GetBrightness(int* brightness) = 0;
    virtual tvError_t SaveBrightness(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value) = 0;
    virtual tvError_t SetContrast(int contrast) = 0;
    virtual tvError_t GetContrast(int* contrast) = 0;
    virtual tvError_t SaveContrast(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value) = 0;
    virtual tvError_t SetSharpness(int sharpness) = 0;
    virtual tvError_t GetSharpness(int* sharpness) = 0;
    virtual tvError_t SaveSharpness(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value) = 0;
    virtual tvError_t SetSaturation(int saturation) = 0;
    virtual tvError_t GetSaturation(int* saturation) = 0;
    virtual tvError_t SaveSaturation(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value) = 0;
    virtual tvError_t SetHue(int hue) = 0;
    virtual tvError_t GetHue(int* hue) = 0;
    virtual tvError_t SaveHue(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value) = 0;
    virtual tvError_t SetColorTemperature(tvColorTemp_t colorTemp) = 0;
    virtual tvError_t GetColorTemperature(tvColorTemp_t* colorTemp) = 0;
    virtual tvError_t SaveColorTemperature(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvColorTemp_t value) = 0;
    virtual tvError_t SetAspectRatio(tvDisplayMode_t dispMode) = 0;
    virtual tvError_t GetAspectRatio(tvDisplayMode_t* dispMode) = 0;
    virtual tvError_t SaveAspectRatio(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDisplayMode_t value) = 0;
    virtual tvError_t SetLowLatencyState(int lowLatencyIndex) = 0;
    virtual tvError_t GetLowLatencyState(int* lowLatencyState) = 0;
    virtual tvError_t SaveLowLatencyState(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value) = 0;
    virtual tvError_t GetTVSupportedDolbyVisionModes(tvDolbyMode_t** dvModes, unsigned short* count) = 0;
    virtual tvError_t SetTVDolbyVisionMode(tvDolbyMode_t dolbyMode) = 0;
    virtual tvError_t SaveTVDolbyVisionMode(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDolbyMode_t value) = 0;
    virtual tvError_t GetTVSupportedPictureModes(pic_modes_t** pictureModes, unsigned short* count) = 0;
    virtual tvError_t GetTVPictureMode(char* pictureMode) = 0;
    virtual tvError_t SetTVPictureMode(const char* pictureMode) = 0;
    virtual tvError_t SaveSourcePictureMode(tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, int pictureMode) = 0;
    virtual tvError_t GetDefaultPQParams(int pqIndex, tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, tvPQParameterIndex_t pqParamIndex, int* value) = 0;
    //tvSettingsODM.h
    virtual tvError_t TvSyncCalibrationInfoODM() = 0;
    virtual tvError_t GetCurrentSource(tvVideoSrcType_t *currentSource) = 0;
    virtual tvError_t GetTVSupportedDolbyVisionModesODM(pic_modes_t** dvModes,unsigned short *count) = 0;
    virtual tvError_t SetTVDolbyVisionModeODM(const char * dolbyMode) = 0;
    virtual int ConvertHDRFormatToContentFormatODM(tvhdr_type_t hdrFormat) = 0;
    //tvSettingsExtODM.h
    virtual int ReadCapablitiesFromConfODM(std::string& rangeInfo, std::string& pqmodeInfo, std::string& formatInfo, std::string& sourceInfo, std::string param, std::string& platformsupport, std::string& index) = 0;
    virtual int  GetPanelIDODM(char* panelid) = 0;
};

class TvSettings {
protected:
    static TvSettingsImpl* impl;

public:
    TvSettings();
    TvSettings(const TvSettings &obj) = delete;
    static void setImpl(TvSettingsImpl* newImpl);

    static tvError_t TvInit();
    static tvError_t TvTerm();
    static tvError_t RegisterVideoFormatChangeCB(tvVideoFormatCallbackData *cbData);
    static tvError_t RegisterVideoContentChangeCB(tvVideoContentCallbackData *cbData);
    static tvError_t RegisterVideoResolutionChangeCB(tvVideoResolutionCallbackData *cbData);
    static tvError_t RegisterVideoFrameRateChangeCB(tvVideoFrameRateCallbackData *cbData);
    static tvError_t GetTVSupportedVideoFormats(tvVideoFormatType_t **videoFormats, unsigned short *numberOfFormats);
    static tvError_t GetCurrentVideoFormat(tvVideoFormatType_t *videoFormat);
    static tvError_t GetCurrentVideoResolution(tvResolutionParam_t *res);
    static tvError_t GetCurrentVideoFrameRate(tvVideoFrameRate_t *format);
    static tvError_t GetBacklight(int *backlight);
    static tvError_t SetBacklight(int backlight);
    static tvError_t SaveBacklight(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value);
    static tvError_t SetTVDimmingMode(const char *dimmingMode);
    static tvError_t GetTVDimmingMode(char *dimmingMode);
    static tvError_t SaveTVDimmingMode(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDimmingMode_t dimmingMode);
    static tvError_t SetBrightness(int brightness);
    static tvError_t GetBrightness(int *brightness);
    static tvError_t SaveBrightness(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value);
    static tvError_t SetContrast(int contrast);
    static tvError_t GetContrast(int *contrast);
    static tvError_t SaveContrast(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value);
    static tvError_t SetSharpness(int sharpness);
    static tvError_t GetSharpness(int *sharpness);
    static tvError_t SaveSharpness(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value);
    static tvError_t SetSaturation(int saturation);
    static tvError_t GetSaturation(int *saturation);
    static tvError_t SaveSaturation(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value);
    static tvError_t SetHue(int hue);
    static tvError_t GetHue(int *hue);
    static tvError_t SaveHue(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value);
    static tvError_t SetColorTemperature(tvColorTemp_t colorTemp);
    static tvError_t GetColorTemperature(tvColorTemp_t *colorTemp);
    static tvError_t SaveColorTemperature(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvColorTemp_t value);
    static tvError_t SetAspectRatio(tvDisplayMode_t dispMode);
    static tvError_t GetAspectRatio(tvDisplayMode_t *dispMode);
    static tvError_t SaveAspectRatio(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDisplayMode_t value);
    static tvError_t SetLowLatencyState(int lowLatencyIndex);
    static tvError_t GetLowLatencyState(int *lowLatencyState);
    static tvError_t SaveLowLatencyState(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, int value);
    static tvError_t GetTVSupportedDolbyVisionModes(tvDolbyMode_t **dvModes, unsigned short *count);
    static tvError_t SetTVDolbyVisionMode(tvDolbyMode_t dolbyMode);
    static tvError_t SaveTVDolbyVisionMode(tvVideoSrcType_t videoSrcType, int pq_mode, tvVideoFormatType_t videoFormatType, tvDolbyMode_t value);
    static tvError_t GetTVSupportedPictureModes(pic_modes_t **pictureModes, unsigned short *count);
    static tvError_t GetTVPictureMode(char *pictureMode);
    static tvError_t SetTVPictureMode(const char *pictureMode);
    static tvError_t SaveSourcePictureMode(tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, int pictureMode);
    static tvError_t GetDefaultPQParams(int pqIndex, tvVideoSrcType_t videoSrcType, tvVideoFormatType_t videoFormatType, tvPQParameterIndex_t pqParamIndex, int *value);
    //tvSettingsODM.h
    static tvError_t TvSyncCalibrationInfoODM();
    static tvError_t GetCurrentSource(tvVideoSrcType_t *currentSource);
    static tvError_t GetTVSupportedDolbyVisionModesODM(pic_modes_t** dvModes,unsigned short *count);
    static tvError_t SetTVDolbyVisionModeODM(const char * dolbyMode);
    static int ConvertHDRFormatToContentFormatODM(tvhdr_type_t hdrFormat);
    //tvSettingsExtODM.h
    static int ReadCapablitiesFromConfODM(std::string& rangeInfo, std::string& pqmodeInfo, std::string& formatInfo, std::string& sourceInfo, std::string param, std::string& platformsupport, std::string& index);
    static int  GetPanelIDODM(char* panelid);
};

// External function declarations
extern tvError_t (*TvInit)();
extern tvError_t (*TvTerm)();
extern tvError_t (*RegisterVideoFormatChangeCB)(tvVideoFormatCallbackData*);
extern tvError_t (*RegisterVideoContentChangeCB)(tvVideoContentCallbackData*);
extern tvError_t (*RegisterVideoResolutionChangeCB)(tvVideoResolutionCallbackData*);
extern tvError_t (*RegisterVideoFrameRateChangeCB)(tvVideoFrameRateCallbackData*);
extern tvError_t (*GetTVSupportedVideoFormats)(tvVideoFormatType_t**, unsigned short*);
extern tvError_t (*GetCurrentVideoFormat)(tvVideoFormatType_t*);
extern tvError_t (*GetCurrentVideoResolution)(tvResolutionParam_t*);
extern tvError_t (*GetCurrentVideoFrameRate)(tvVideoFrameRate_t*);
extern tvError_t (*GetBacklight)(int*);
extern tvError_t (*SetBacklight)(int);
extern tvError_t (*SaveBacklight)(tvVideoSrcType_t, int, tvVideoFormatType_t, int);
extern tvError_t (*SetTVDimmingMode)(const char*);
extern tvError_t (*GetTVDimmingMode)(char*);
extern tvError_t (*SaveTVDimmingMode)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvDimmingMode_t);
extern tvError_t (*SetBrightness)(int);
extern tvError_t (*GetBrightness)(int*);
extern tvError_t (*SaveBrightness)(tvVideoSrcType_t, int, tvVideoFormatType_t, int);
extern tvError_t (*SetContrast)(int);
extern tvError_t (*GetContrast)(int*);
extern tvError_t (*SaveContrast)(tvVideoSrcType_t, int, tvVideoFormatType_t, int);
extern tvError_t (*SetSharpness)(int);
extern tvError_t (*GetSharpness)(int*);
extern tvError_t (*SaveSharpness)(tvVideoSrcType_t, int, tvVideoFormatType_t, int);
extern tvError_t (*SetSaturation)(int);
extern tvError_t (*GetSaturation)(int*);
extern tvError_t (*SaveSaturation)(tvVideoSrcType_t, int, tvVideoFormatType_t, int);
extern tvError_t (*SetHue)(int);
extern tvError_t (*GetHue)(int*);
extern tvError_t (*SaveHue)(tvVideoSrcType_t, int, tvVideoFormatType_t, int);
extern tvError_t (*SetColorTemperature)(tvColorTemp_t);
extern tvError_t (*GetColorTemperature)(tvColorTemp_t*);
extern tvError_t (*SaveColorTemperature)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvColorTemp_t);
extern tvError_t (*SetAspectRatio)(tvDisplayMode_t);
extern tvError_t (*GetAspectRatio)(tvDisplayMode_t*);
extern tvError_t (*SaveAspectRatio)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvDisplayMode_t);
extern tvError_t (*SetLowLatencyState)(int);
extern tvError_t (*GetLowLatencyState)(int*);
extern tvError_t (*SaveLowLatencyState)(tvVideoSrcType_t, int, tvVideoFormatType_t, int);
extern tvError_t (*GetTVSupportedDolbyVisionModes)(tvDolbyMode_t**, unsigned short*);
extern tvError_t (*SetTVDolbyVisionMode)(tvDolbyMode_t);
extern tvError_t (*SaveTVDolbyVisionMode)(tvVideoSrcType_t, int, tvVideoFormatType_t, tvDolbyMode_t);
extern tvError_t (*GetTVSupportedPictureModes)(pic_modes_t**, unsigned short*);
extern tvError_t (*GetTVPictureMode)(char*);
extern tvError_t (*SetTVPictureMode)(const char*);
extern tvError_t (*SaveSourcePictureMode)(tvVideoSrcType_t, tvVideoFormatType_t, int);
extern tvError_t (*GetDefaultPQParams)(int, tvVideoSrcType_t, tvVideoFormatType_t, tvPQParameterIndex_t, int*);
//tvSettingsODM.h
extern tvError_t (*TvSyncCalibrationInfoODM)();
extern tvError_t (*GetCurrentSource)(tvVideoSrcType_t *);
extern tvError_t (*GetTVSupportedDolbyVisionModesODM)(pic_modes_t**, unsigned short*);
extern tvError_t (*SetTVDolbyVisionModeODM)(const char *);
extern int (*ConvertHDRFormatToContentFormatODM)(tvhdr_type_t);
//tvSettingsExtODM.h
extern int (*ReadCapablitiesFromConfODM)(std::string& rangeInfo, std::string& pqmodeInfo, std::string& formatInfo, std::string& sourceInfo, std::string param, std::string& platformsupport, std::string& index);
extern int (*GetPanelIDODM)(char*);
