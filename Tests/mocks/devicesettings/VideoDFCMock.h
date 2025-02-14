#pragma once

#include <gmock/gmock.h>

#include "VideoDFC.hpp"

class VideoDFCMock : public device::VideoDFCImpl {
public:
    virtual ~VideoDFCMock() = default;


    MOCK_METHOD(const std::string&, getName, (), (const, override));
};
