/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Synamedia Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MockAccessor.h"

template <class TMOCK_PTR>
TMOCK_PTR* MockAccessor<TMOCK_PTR>::_implPtr = nullptr;

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); fflush(stderr);

template <class TMOCK_PTR>
void MockAccessor<TMOCK_PTR>::setPtr(TMOCK_PTR* incoming)
{
    TEST_LOG(" setPtr entry _implPtr = %p",_implPtr);
    assert ((nullptr == _implPtr) || (nullptr == incoming));
    _implPtr = incoming;
    TEST_LOG(" setPtr exit _implPtr = %p",_implPtr);
}

template <class TMOCK_PTR>
TMOCK_PTR* MockAccessor<TMOCK_PTR>::getPtr(void)
{
    assert (nullptr != _implPtr);
    TEST_LOG(" getPtr _implPtr = %p",_implPtr);
    return _implPtr;
}

template class MockAccessor<IarmBusImpl>;
template class MockAccessor<readprocImpl>;
template class MockAccessor<RBusApiImpl>;
template class MockAccessor<RfcApiImpl>;


