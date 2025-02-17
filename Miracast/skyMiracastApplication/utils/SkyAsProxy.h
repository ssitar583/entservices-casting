#ifndef _SKYASPROXY_H_
#define _SKYASPROXY_H_

#include <libsoup/soup.h>
#include <string>
#include <mutex>
#include <utility>

class SkyAsProxy
{
public:
    SkyAsProxy();
    ~SkyAsProxy();
    bool stopAllPlayers();
    class SkyAsHost;
private:
    bool post(const std::string& url, const std::string& payload);
    std::string createUrl(const std::string& resourceName);
    SoupSession* mSession;
};

#endif //_SKYASPROXY_H_

