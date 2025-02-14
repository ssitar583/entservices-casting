/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
**/

#include "pkg.h"

static opkg_conf_t _conf;
opkg_conf_t *opkg_config = &_conf;

int opkg_update_package_lists(opkg_progress_callback_t progress_callback, void *user_data)
{
	return 0;
}

int opkg_install_package(const char *package_url, opkg_progress_callback_t progress_callback,void *user_data)
{
   return 0;
}

void opkg_conf_deinit(void)
{
	return;
}

int opkg_compare_versions(const char *ver1, const char *ver2)
{
	return 0;
}

int opkg_upgrade_package(const char *package_name, opkg_progress_callback_t callback, void *user_data)
{
	return 0;
}

int opkg_new(void)
{
	return 0;
}

void opkg_download_cleanup(void)
{
	return;
}

int opkg_list_upgradable_packages(opkg_package_callback_t callback, void *user_data)
{
        return 0;
}