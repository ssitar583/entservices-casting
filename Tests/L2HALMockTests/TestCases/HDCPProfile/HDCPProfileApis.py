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

activate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.activate", "params":{"callsign":"org.rdk.HdcpProfile"}}' http://127.0.0.1:55555/jsonrpc'''

# Curl command for deactivating HDCPProfile plugin
deactivate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.deactivate", "params":{"callsign":"org.rdk.HdcpProfile"}}' http://127.0.0.1:55555/jsonrpc'''

# Store the expected output response for activate & deactivate curl command
expected_output_response = '{"jsonrpc":"2.0","id":3,"result":null}'

######################################################################################

# HDCPProfile Methods :

get_hdcpstatus = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.HdcpProfile.getHDCPStatus"}' http://127.0.0.1:55555/jsonrpc'''

get_settophdcpsupport = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.HdcpProfile.getSettopHDCPSupport"}' http://127.0.0.1:55555/jsonrpc'''