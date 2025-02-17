#include <netdb.h>
#include <utility>
#include <sstream>
#include <arpa/inet.h>

#include "SkyAsProxy.h"
#include "MiracastAppLogging.hpp"

class SkyAsProxy::SkyAsHost
{
public:
    static std::pair<std::string, uint16_t> getAsAddress();
private:
    static std::mutex mMutex;
    static std::string mAsIpAddress;
    static uint16_t mAsPort;
};

std::mutex SkyAsProxy::SkyAsHost::mMutex;
std::string SkyAsProxy::SkyAsHost::mAsIpAddress = "";
uint16_t SkyAsProxy::SkyAsHost::mAsPort = 0;

std::pair<std::string, uint16_t> SkyAsProxy::SkyAsHost::getAsAddress()
{
    const std::lock_guard<std::mutex> lock(mMutex);

    MIRACASTLOG_TRACE("Entering ...");

    if (mAsIpAddress.empty() || mAsPort == 0)
    {
        addrinfo* lookup = nullptr;
        int err = getaddrinfo("as", "as", NULL, &lookup);
        if ((err != 0) || (lookup == nullptr) || (lookup->ai_addr == nullptr))
        {
            MIRACASTLOG_ERROR("Failed to get AS host; err %d, lookup %p", err, lookup);
        }
        else
        {
            sockaddr_in* address = reinterpret_cast<sockaddr_in*>(lookup->ai_addr);
            mAsIpAddress = inet_ntoa(address->sin_addr);
            mAsPort = ntohs(address->sin_port);

            freeaddrinfo(lookup);

            MIRACASTLOG_INFO("Successfuly got AS host; IP %s, port %u", mAsIpAddress.c_str(), mAsPort);
        }
    }
    MIRACASTLOG_TRACE("Exiting ...");
    return std::make_pair(mAsIpAddress, mAsPort);
}

SkyAsProxy::SkyAsProxy()
:mSession(soup_session_new_with_options("timeout", 1/*One Second*/, NULL))
{
    MIRACASTLOG_TRACE("Entered ...");
}

SkyAsProxy::~SkyAsProxy(){
    soup_session_abort(mSession);
    g_object_unref(mSession);
}

std::string SkyAsProxy::createUrl(const std::string& resourceName)
{
    MIRACASTLOG_TRACE("Entering ...");
    std::pair<std::string, uint16_t> asHost = SkyAsHost::getAsAddress();
    std::stringstream uri;
    uri << "http://" << asHost.first << ":" << asHost.second << resourceName;

    MIRACASTLOG_TRACE("Exiting ...");

    return uri.str();
}

bool SkyAsProxy::post(const std::string& url, const std::string& payload){
    MIRACASTLOG_TRACE("Entering ...");
    SoupMessage* msg = soup_message_new("POST", url.c_str());
    bool result = false;
    soup_message_set_flags(msg, SOUP_MESSAGE_NO_REDIRECT);
    soup_message_set_request(msg, "application/x-www-form-urlencoded", SOUP_MEMORY_COPY, payload.c_str(), payload.size());
    guint statusCode = soup_session_send_message(mSession, msg);
    if (SOUP_STATUS_IS_SUCCESSFUL(statusCode))
    {
	    MIRACASTLOG_VERBOSE("Successful %s POST call", url.c_str());
	    result = true;
    }else{
	    MIRACASTLOG_ERROR("Failed to POST %s, status code: %u", url.c_str(), statusCode);
    }
    g_object_unref(msg);

    MIRACASTLOG_TRACE("Exiting ...");

    return result;
}

bool SkyAsProxy::stopAllPlayers() {
    MIRACASTLOG_TRACE("Entering ...");
    std::string stopPlayer1Url = createUrl("/as/players/1/action/stop");
    MIRACASTLOG_TRACE("Exiting ...");
    return post(stopPlayer1Url, "");
}