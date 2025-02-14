#pragma once

#include <gmock/gmock.h>
#include "Communicator.h"

#if 0
class CommunicatorClientMock : public ICommunicatorClient
{
public:
    virtual ~CommunicatorClientMock() = default;

    MOCK_METHOD(uint32_t, Open, (const uint32_t waitTime), (override));
    MOCK_METHOD(INTERFACE*, Open, (const std::string& className, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(INTERFACE*, Acquire, (const uint32_t waitTime, const std::string& className, const uint32_t versionId), (override));
    MOCK_METHOD(uint32_t, Offer, (INTERFACE* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Revoke, (INTERFACE* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Close, (const uint32_t waitTime), (override));
};
#else
class CommunicatorClientMock : public ICommunicatorClient {
public:
    virtual ~CommunicatorClientMock() = default;
    MOCK_METHOD(void, AddRef, (), (const));
    MOCK_METHOD(uint32_t, Release, (), (const));
    MOCK_METHOD(uint32_t, Open, (const uint32_t waitTime), (override));
    MOCK_METHOD(void*, Open, (const std::string& className, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(void*, Acquire, (const uint32_t waitTime, const std::string& className, const uint32_t versionId), (override));
    MOCK_METHOD(uint32_t, Offer, (void* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Revoke, (void* offer, const uint32_t version, const uint32_t waitTime), (override));
    MOCK_METHOD(uint32_t, Close, (const uint32_t waitTime), (override));
    MOCK_METHOD(void*, Acquire, (const std::string& className, const uint32_t interfaceId, const uint32_t versionId), (override));
};

#endif
