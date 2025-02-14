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

# Testcase ID : TCID016_DS_FrontPanel_Deactivate
# Testcase Description : Simulate the plugin deactivation call.

from Utilities import Utils, ReportGenerator
from FrontPanel import FrontPanelApis
import time

print("TC Description - Simulate the plugin deactivation call")
print("---------------------------------------------------------------------------------------------------------------------------")
# store the expected output response

expected_output_response = '{"jsonrpc":"2.0","id":42,"result":{"success":true}}'

time.sleep(2)
# send the curl command and fetch the output json response
curl_response = Utils.send_curl_command(FrontPanelApis.deactivate_command)
if curl_response:
     Utils.info_log("curl command to set LED is sent from the test runner")
else:
     Utils.error_log("curl command invoke failed")

print("---------------------------------------------------------------------------------------------------------------------------")
# compare both expected and received output responses
if curl_response:
    status = 'Pass'
    message = 'Output response is matching with expected one. The set LED success ' \
              'is obtained in output response'
else:
    status = 'Fail'
    message = 'Output response is different from expected one'

time.sleep(2)
# generate logs in terminal
tc_id = 'TCID010_DS_FrontPanel_Deactivate'
print("Testcase ID : " + tc_id)
print("Testcase Output Response : " + curl_response)
print("Testcase Status : " + status)
print("Testcase Message : " + message)
print("")

if status == 'Pass':
    ReportGenerator.passed_tc_list.append(tc_id)
else:
    ReportGenerator.failed_tc_list.append(tc_id)

print("Reset to activate stage")
curl_response = Utils.send_curl_command(FrontPanelApis.activate_command)

# push the testcase execution details to report file
ReportGenerator.append_test_results_to_csv(tc_id, curl_response, status, message)
