#pragma once

#include <gmock/gmock.h>

#include "audioOutputPortType.hpp"

class AudioOutputPortTypeMock : public device::AudioOutputPortTypeImpl {
public:
    virtual ~AudioOutputPortTypeMock() = default;

    MOCK_METHOD(int, getId, (), (const, override));
    // MOCK_METHOD(const device::List<device::VideoResolution>, getSupportedResolutions, (), (const, override));
};