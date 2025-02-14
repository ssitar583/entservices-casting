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

# Curl command for activating HdmiCecSource plugin
activate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.activate", "params":{"callsign":"org.rdk.HdmiCecSource"}}' http://127.0.0.1:55555/jsonrpc'''

# Curl command for deactivating HdmiCecSource plugin
deactivate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.deactivate", "params":{"callsign":"org.rdk.HdmiCecSource"}}' http://127.0.0.1:55555/jsonrpc'''

# Store the expected output response for activate & deactivate curl command
expected_output_response = '{"jsonrpc":"2.0","id":3,"result":null}'

######################################################################################

# HdmiCecSource Methods :

get_device_list = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.getDeviceList"}' http://127.0.0.1:55555/jsonrpc'''

send_standby_message = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", 
"id": 42,"method":"org.rdk.HdmiCecSink.sendStandbyMessage"}' http://127.0.0.1:55555/jsonrpc'''

get_vendor_id = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.getVendorId"}' http://127.0.0.1:55555/jsonrpc'''

set_vendor_id = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setVendorId","params": {"vendorid": "0x4455"}}' http://127.0.0.1:55555/jsonrpc'''

get_osd_name = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.getOSDName"}' http://127.0.0.1:55555/jsonrpc'''

set_osd_name = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setOSDName","params": {"name": "CUSTOM8 TV"}}' http://127.0.0.1:55555/jsonrpc'''

get_enabled = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.getEnabled"}' http://127.0.0.1:55555/jsonrpc'''

set_enabled_false = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setEnabled","params": {"enabled": false}}' http://127.0.0.1:55555/jsonrpc'''

set_enabled_true = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setEnabled","params": {"enabled": true}}' http://127.0.0.1:55555/jsonrpc'''

get_active_source_status = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0",
"id": 42,"method":"org.rdk.HdmiCecSink.getActiveSourceStatus","params": {"status": true}}' http://127.0.0.1:55555/jsonrpc'''

get_otp_enabled = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.getOTPEnabled"}' http://127.0.0.1:55555/jsonrpc'''

set_otp_enabled_false = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setOTPEnabled","params": {"enabled": false}}' http://127.0.0.1:55555/jsonrpc'''

set_otp_enabled_true = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setOTPEnabled","params": {"enabled": true}}' http://127.0.0.1:55555/jsonrpc'''

send_keypress_VOLUME_UP = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 65}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_VOLUME_DOWN = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 66}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_MUTE = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 67}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_UP = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 1}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_DOWN = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 2}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_LEFT = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 3}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_RIGHT = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 4}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_SELECT = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 0}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_HOME = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 9}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_BACK = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 13}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_0 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 32}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_1 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 33}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_2 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 34}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_3 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 35}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_4 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 36}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_5 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 37}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_6 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 38}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_7 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 39}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_8 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 40}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_9 = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendKeyPressEvent", "params": {"logicalAddress": 0,"keyCode": 41}}' http://127.0.0.1:55555/jsonrpc'''


send_keypress_VOLUME_UP_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 65}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_VOLUME_DOWN_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 66}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_MUTE_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 67}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_UP_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 1}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_DOWN_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 2}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_LEFT_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 3}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_RIGHT_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 4}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_SELECT_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 0}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_HOME_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 9}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_BACK_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 13}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_0_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 32}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_1_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 33}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_2_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 34}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_3_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 35}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_4_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 36}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_5_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 37}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_6_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 38}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_7_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 39}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_8_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 40}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_9_USER_Press = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlPressed", "params": {"logicalAddress": 0,"keyCode": 41}}' http://127.0.0.1:55555/jsonrpc'''

send_keypress_VOLUME_UP_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 65}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_VOLUME_DOWN_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 66}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_MUTE_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 67}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_UP_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 1}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_DOWN_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 2}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_LEFT_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 3}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_RIGHT_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 4}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_SELECT_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 0}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_HOME_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 9}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_BACK_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 13}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_0_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 32}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_1_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 33}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_2_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 34}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_3_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 35}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_4_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 36}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_5_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 37}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_6_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 38}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_7_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 39}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_8_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 40}}' http://127.0.0.1:55555/jsonrpc'''
send_keypress_NUMBER_9_USER_Release = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendUserControlReleased", "params": {"logicalAddress": 0,"keyCode": 41}}' http://127.0.0.1:55555/jsonrpc'''



perform_otp_action = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.performOTPAction"}' http://127.0.0.1:55555/jsonrpc'''

request_short_audio_descriptor = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.requestShortAudioDescriptor"}' http://127.0.0.1:55555/jsonrpc'''

send_audio_device_power_on_message = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.sendAudioDevicePowerOnMessage"}' http://127.0.0.1:55555/jsonrpc'''

get_audio_status_message = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.getAudioStatusMessage"}' http://127.0.0.1:55555/jsonrpc'''

request_active_source = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.requestActiveSource"}' http://127.0.0.1:55555/jsonrpc'''

get_audio_device_connected_status =  '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.getAudioDeviceConnectedStatus"}' http://127.0.0.1:55555/jsonrpc'''

set_otp_enabled_invalid = '''curl --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.HdmiCecSink.setOTPEnabled","params": {"ennable": true}}' http://127.0.0.1:55555/jsonrpc'''

set_osd_name_invalid = '''curl --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.HdmiCecSink.setOSDName","params": {"nnamme": "LG TV"}}' http://127.0.0.1:55555/jsonrpc'''

set_vendor_id_invalid_1 = '''curl --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.HdmiCecSink.setVendorId","params": {"vendorid": "]]"}}' http://127.0.0.1:55555/jsonrpc'''

set_vendor_id_invalid_2 = '''curl --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.HdmiCecSink.setVendorId","params": {"vllendorid": "]]"}}' http://127.0.0.1:55555/jsonrpc'''


get_active_route = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.getActiveRoute"}' http://127.0.0.1:55555/jsonrpc'''


set_active_path = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,"method":"org.rdk.HdmiCecSink.setActivePath", "params": {"activePath": "1.0.0.0"}}' http://127.0.0.1:55555/jsonrpc'''
set_active_source = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.setActiveSource"}' http://127.0.0.1:55555/jsonrpc'''

set_menu_language = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setMenuLanguage","params": {"language": "chi"}}' http://127.0.0.1:55555/jsonrpc'''

set_latency_info ='''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setLatencyInfo","params": {"videoLatency": "2","lowLatencyMode": "1","audioOutputCompensated": "1","audioOutputDelay": "20"}}' http://127.0.0.1:55555/jsonrpc'''

set_routing_change = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setRoutingChange","params": {"oldPort": "HDMI0","newPort":"TV"}}' http://127.0.0.1:55555/jsonrpc'''

set_routing_change_nav = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setRoutingChange","params": {"oldPort": "HDMI0","newPort":"TV"}}' http://127.0.0.1:55555/jsonrpc'''

set_arc_routing_params_enabled = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setupARCRouting","params":{"enabled":true}' http://127.0.0.1:55555/jsonrpc'''

set_arc_routing_params_disabled = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setupARCRouting","params":{"enabled":false}' http://127.0.0.1:55555/jsonrpc'''

print_device_list = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.printDeviceList"}' http://127.0.0.1:55555/jsonrpc'''

request_audio_device_power_status = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.requestAudioDevicePowerStatus"}' http://127.0.0.1:55555/jsonrpc'''

get_Active_Source = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.getActiveSource"}' http://127.0.0.1:55555/jsonrpc'''

sendGetAudioStatusMessage = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.HdmiCecSink.sendGetAudioStatusMessage"}' http://127.0.0.1:55555/jsonrpc'''

set_vendor_id_invalid_2 = '''curl --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0","id": 42,"method": "org.rdk.HdmiCecSource.setVendorId","params": {"vllendorid": "]]"}}' http://127.0.0.1:55555/jsonrpc'''

set_routing_change_negative = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.HdmiCecSink.setRoutingChange","params": {"oldPort": "HDMI0","newPort":"NEW"}}' http://127.0.0.1:55555/jsonrpc'''
