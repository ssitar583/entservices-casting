#include "PowerManagerMock.h"

// createInterface specializing IPowerManager (for returning mock instance)
template <>
Exchange::IPowerManager* Plugin::createInterface(Plugin::PluginInterfaceBuilder<Exchange::IPowerManager>& builder)
{
    auto *mockInterface = PowerManagerMock::Get();
    mockInterface->AddRef();
    return mockInterface;
}
