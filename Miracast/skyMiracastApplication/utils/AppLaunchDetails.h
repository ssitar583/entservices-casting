#ifndef APPLAUNCHDETAILS_H
#define APPLAUNCHDETAILS_H

#include <string>
#include <vector>
#include "helpers.hpp"

namespace LaunchDetails
{

enum AppLaunchMethod
{
    Suspend,
    PartnerButton
};

class AppLaunchDetails
{
public:
    static AppLaunchDetails *getInstance();
    static void destroyInstance();
    std::string getEnvVariable(const std::string& key);
    bool parseLaunchDetails(const std::string& launchMethodStr, const std::string& encodedLaunchParamsStr);

    AppLaunchMethod mLaunchMethod;
    std::string mSourceDevIP;
    std::string mSourceDevMAC;
    std::string mSourceDevName;
    std::string mSinkDevIP;
    VideoRectangleInfo mVideoRect;
    bool mlaunchAppInForeground;

private:
    static AppLaunchDetails *m_AppLauncherInfo;
    AppLaunchDetails();
    virtual ~AppLaunchDetails();
    AppLaunchDetails &operator=(const AppLaunchDetails &) = delete;
    AppLaunchDetails(const AppLaunchDetails &) = delete;

    AppLaunchMethod parseLaunchMethod(const std::string& method);
    std::string base64Decode(const std::string &input);
};

AppLaunchDetails* parseLaunchDetailsFromEnv();

}

#endif //APPLAUNCHDETAILS_H
