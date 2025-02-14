#** *****************************************************************************
# *
# * If not stated otherwise in this file or this component's LICENSE file the
# * following copyright and licenses apply:
# *
# * Copyright 2024 RDK Management
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *
# http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *
#* ******************************************************************************

# Testcase ID : TCID002_DS_FrontPanel_getPreferences
# Testcase Description : Returns the preferences that are saved in the /opt/fp_service_preferences.json file.

from Utilities import Utils, ReportGenerator
from FrontPanel import FrontPanelApis

print("TC Description - Returns the preferences that are saved in the /opt/fp_service_preferences.json file.")
print("---------------------------------------------------------------------------------------------------------------------------")
# store the expected output response
expected_output_response = '{"jsonrpc":"2.0","id":42,"result":{"preferences":{},"success":true}}'

# send the curl command and fetch the output json response
curl_response = Utils.send_curl_command(FrontPanelApis.get_preferences)
if curl_response:
     Utils.info_log("curl command to get preferences is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed {}" .format(FrontPanelApis.get_preferences))

print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
if str(curl_response) == str(expected_output_response):
    status = 'Pass'
    message = 'Output response is matching with expected one. The preferences set in opt/fp_service_preferences.json ' \
              'is obtained in output response'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

# generate logs in terminal
tc_id = 'TCID002_DS_FrontPanel_getPreferences'
print("Testcase ID : " + tc_id)
print("Testcase Output Response : " + curl_response)
print("Testcase Status : " + status)
print("Testcase Message : " + message)
print("")

if status == 'Pass':
    ReportGenerator.passed_tc_list.append(tc_id)
else:
    ReportGenerator.failed_tc_list.append(tc_id)
# push the testcase execution details to report file
ReportGenerator.append_test_results_to_csv(tc_id, curl_response, status, message)
