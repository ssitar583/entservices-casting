#include "Communicator.h"
#include <gmock/gmock.h>

ICommunicatorClient* WPEFramework::RPC::CommunicatorClient::impl = nullptr;

void WPEFramework::RPC::CommunicatorClient::setImpl(ICommunicatorClient* newImpl)
{
   // Handles both resetting 'impl' to nullptr and assigning a new value to 'impl'
   EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
   impl = newImpl;
}
