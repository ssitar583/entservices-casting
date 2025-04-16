#include <unordered_map>
#include <vector>
#include<openssl/evp.h>
#include <WPEFramework/core/JSON.h>
#include "AppLaunchDetails.h"
#include "MiracastAppLogging.hpp"
#include "util_base64.h"

namespace LaunchDetails{
    AppLaunchDetails *AppLaunchDetails::m_AppLauncherInfo{nullptr};

    std::string AppLaunchDetails::getEnvVariable(const std::string& key)
    {
        const char* envVariable = std::getenv(key.c_str());
        return envVariable ? envVariable : "";
    }

    AppLaunchMethod AppLaunchDetails::parseLaunchMethod(const std::string& method)
    {
        MIRACASTLOG_INFO("method: %s\n",method.c_str());

        AppLaunchMethod launchMethod = AppLaunchMethod::Suspend;

        const std::unordered_map<std::string, AppLaunchMethod> launchMethods =
        {
            {"Suspend", AppLaunchMethod::Suspend},
            {"PARTNER_BUTTON", AppLaunchMethod::PartnerButton}
        };

        auto it = launchMethods.find(method);
        if (it != launchMethods.end())
        {
            launchMethod = it->second;
        }

        return launchMethod;
    }

    std::string AppLaunchDetails::base64Decode(const std::string &input)
    {
        std::string decodedStr;
        char decode_str[4000] = {0};
        decodedStr.resize(input.size()+4000);
        MIRACASTLOG_INFO("MiracastApp Widget: Entered, encoded sting length:%d\n", input.size()+1);
        int decodedStrLen = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(&decodedStr[0]), reinterpret_cast<const unsigned char*>(&input[0]), input.size());
        if(decodedStrLen != -1){
            decodedStr.resize(decodedStrLen);
            MIRACASTLOG_INFO("MiracastApp Widget: Decoded completed:%s decodedStrLen:%d\n", decodedStr, decodedStr.size());
        }
        MIRACASTLOG_VERBOSE("MiracastApp Widget: Decoded Str:%s\n", decodedStr.c_str());
        return decodedStr;
    }

    bool AppLaunchDetails::parseLaunchDetails(const std::string& launchMethodStr, const std::string& encodedLaunchParamsStr)
    {
        JsonObject MiracastAppParams, parameters;
        char decode_str[1000] = {0};
        size_t decode_len=0;
        std::string config_option;
        std::vector<std::string> argVector;
        bool returnValue = true;

        MIRACASTLOG_TRACE("Entering ...");
        
        mLaunchMethod = parseLaunchMethod(launchMethodStr);
        MIRACASTLOG_INFO("MiracastApp Widget:Received launch parameters: %s\n",encodedLaunchParamsStr.c_str());
        int status = util_base64_decode(encodedLaunchParamsStr.c_str(), encodedLaunchParamsStr.size(), decode_str,999,&decode_len);
        if(status == 0)
        {
            decode_str[decode_len] = '\0';
            MIRACASTLOG_INFO("MiracastApp Widget:Decoded launch parameters: %s decode_len:%d\n",decode_str, decode_len);
            std::string paramStr = decode_str;
            MiracastAppParams.FromString(paramStr);
            if(MiracastAppParams.HasLabel("params"))
            {
				parameters = MiracastAppParams["params"].Object();

                if(parameters.HasLabel("video_rectangle"))
                {
                    JsonObject video_rectangle = parameters["video_rectangle"].Object();

                    video_rectangle = parameters["video_rectangle"].Object();

                    mVideoRect.startX = video_rectangle["X"].Number();
                    mVideoRect.startY = video_rectangle["Y"].Number();
                    mVideoRect.width = video_rectangle["W"].Number();
                    mVideoRect.height = video_rectangle["H"].Number();

                    MIRACASTLOG_INFO("video_rectangle received: startX:%d, startY:%d, width:%d, height:%d\n",
                        mVideoRect.startX, mVideoRect.startY, mVideoRect.width, mVideoRect.height);
                }
                else
                {
                    MIRACASTLOG_INFO("missing video_rectangle from the received parameters\n");
                    returnValue = false;
                }

                if(parameters.HasLabel("launchAppInForeground")){
                    mlaunchAppInForeground = parameters["launchAppInForeground"].Boolean();
                    MIRACASTLOG_INFO("launchAppInForeground flag set as: %s\n", mlaunchAppInForeground?"true":"false");
                } else {
                    MIRACASTLOG_INFO("missing launchAppInForeground from the received parameters\n");
                    returnValue = false;
                }
			}
        } else {
            returnValue = false;
            MIRACASTLOG_ERROR("Widget:Failed to Decoded launch parameters are empty\n");
        }
        return returnValue;
    }

    AppLaunchDetails* parseLaunchDetailsFromEnv()
    {
        AppLaunchDetails* appLaunchInstance = AppLaunchDetails::getInstance();

        if (nullptr != appLaunchInstance)
        {
            MIRACASTLOG_INFO("MiracastApp Widget: Entered\n");
            std::string launchMethodStr = appLaunchInstance->getEnvVariable("APPLICATION_LAUNCH_METHOD");
            MIRACASTLOG_INFO(" launchMethodStr set as: %s\n", launchMethodStr.c_str());
            std::string launchArgumentsStr = appLaunchInstance->getEnvVariable("APPLICATION_LAUNCH_PARAMETERS");

            if ( true != appLaunchInstance->parseLaunchDetails(launchMethodStr, launchArgumentsStr))
            {
                MIRACASTLOG_ERROR("Failed to do parseLaunchDetails \n");
                AppLaunchDetails::destroyInstance();
                appLaunchInstance = nullptr;
            }
        }
        return appLaunchInstance;
    }

    AppLaunchDetails *AppLaunchDetails::getInstance()
    {
        if (m_AppLauncherInfo == nullptr)
        {
            m_AppLauncherInfo = new AppLaunchDetails();
        }
        return m_AppLauncherInfo;
    }

    void AppLaunchDetails::destroyInstance()
    {
        if (m_AppLauncherInfo != nullptr)
        {
            delete m_AppLauncherInfo;
            m_AppLauncherInfo = nullptr;
        }
    }

    AppLaunchDetails::AppLaunchDetails()
    {
        
    }

    AppLaunchDetails::~AppLaunchDetails()
    {
        
    }
}