#pragma once

#include <gmock/gmock.h>

#include "AudioStereoMode.hpp"

class AudioStereoModeMock : public device::AudioStereoModeImpl {
public:
    virtual ~AudioStereoModeMock() = default;


    MOCK_METHOD(const std::string&, getName, (), (const, override));
    MOCK_METHOD( std::string, toString, (), (override));


};
