# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(PLUGIN_NAME XCast)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

set(PLUGIN_XCAST_AUTOSTART "false" CACHE STRING "Automatically start XCast plugin")
set(PLUGIN_XCAST_STARTUPORDER "" CACHE STRING "To configure startup order of XCast plugin")

find_package(${NAMESPACE}Plugins REQUIRED)
if (USE_THUNDER_R4)
    find_package(${NAMESPACE}COM REQUIRED)
else ()
    find_package(${NAMESPACE}Protocols REQUIRED)
endif (USE_THUNDER_R4)


add_library(${MODULE_NAME} SHARED
        XCast.cpp
        Module.cpp)

#add_executable(${MODULE_NAME} XCast.cpp
#        Module.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)


target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})


include_directories(
        ../helpers)

target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

#find_package(RFC)
find_package(IARMBus)
find_package(GLIB REQUIRED)
find_package(WPEFrameworkSecurityUtil)
if(NOT WPEFRAMEWORK_SECURITYUTIL_FOUND)
    add_definitions(-DSECURITY_TOKEN_ENABLED=0)
endif()

find_library(XDIAL_LIBRARIES NAMES gdial-server)
target_include_directories(${MODULE_NAME} PRIVATE ${GLIB_INCLUDE_DIRS})

add_library(${PLUGIN_IMPLEMENTATION} SHARED
        XCastImplementation.cpp
        Module.cpp)

#add_executable(${PLUGIN_IMPLEMENTATION}
#        XCastImplementation.cpp
#        Module.cpp)

target_link_libraries(${PLUGIN_IMPLEMENTATION}
        PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins)

set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)	

if (USE_THUNDER_R4)
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}COM::${NAMESPACE}COM)
else ()
target_link_libraries(${PLUGIN_IMPLEMENTATION} PRIVATE ${NAMESPACE}Protocols::${NAMESPACE}Protocols)
endif (USE_THUNDER_R4)

find_package(DS)
find_package(IARMBus)
find_package(CEC)


add_definitions (-DRT_PLATFORM_LINUX)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${IARMBUS_INCLUDE_DIRS})
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${DS_INCLUDE_DIRS})
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ../helpers)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ${GLIB_INCLUDE_DIRS})

set_source_files_properties(XCast.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

if(NOT RDK_SERVICES_L1_TEST)
	target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${RFC_LIBRARIES} ${IARMBUS_LIBRARIES} ${XDIAL_LIBRARIES})
else(RDK_SERVICES_L1_TEST)
      target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${RFC_LIBRARIES} ${IARMBUS_LIBRARIES})
endif()

target_link_libraries(${PLUGIN_IMPLEMENTATION} PUBLIC ${NAMESPACE}Plugins::${NAMESPACE}Plugins ${IARMBUS_LIBRARIES} ${DS_LIBRARIES})

install(TARGETS ${PLUGIN_IMPLEMENTATION}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
