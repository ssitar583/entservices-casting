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

# Curl command for activating FrontPanel plugin
activate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.activate", "params":{"callsign":"org.rdk.FrontPanel"}}' http://127.0.0.1:55555/jsonrpc'''

# Curl command for deactivating FrontPanel plugin
deactivate_command = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc":"2.0","id":"3"
,"method": "Controller.1.deactivate", "params":{"callsign":"org.rdk.FrontPanel"}}' http://127.0.0.1:55555/jsonrpc'''

# Store the expected output response for activate & deactivate curl command
expected_output_response = '{"jsonrpc":"2.0","id":3,"result":null}'

######################################################################################

# FrontPanel Methods :

get_brightness = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.getBrightness","params": {"index": "power_led"}}' http://127.0.0.1:55555/jsonrpc'''

get_brightness_invalid = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.getBrightness","params": {"index": "invalid_string"}}' http://127.0.0.1:55555/jsonrpc'''

get_clock_brightness = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", 
"id": 42,"method":"org.rdk.FrontPanel.getClockBrightness"}' http://127.0.0.1:55555/jsonrpc'''

get_frontpanel_leds = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.getFrontPanelLights"}' http://127.0.0.1:55555/jsonrpc'''

get_preferences_invalid = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.FrontPanel.getPreferences","params": {"vendorid": "0x4455"}}' http://127.0.0.1:55555/jsonrpc'''

get_preferences = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.getPreferences"}' http://127.0.0.1:55555/jsonrpc'''

is_24_hour_clock = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.is24HourClock"}' http://127.0.0.1:55555/jsonrpc'''

power_led_off = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.FrontPanel.powerLedOff","params": {"index": power_led}}' http://127.0.0.1:55555/jsonrpc'''

power_led_on = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.FrontPanel.powerLedOn","params": {"index": power_led}}' http://127.0.0.1:55555/jsonrpc'''

power_led_on_invalid =  '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id":42,
"method":"org.rdk.FrontPanel.powerLedOn","params": {"index": invalid_led}}' http://127.0.0.1:55555/jsonrpc'''

set_24_hour_clock = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.set24HourClock", "params": {"is24Hour": false}}' http://127.0.0.1:55555/jsonrpc'''

set_24_hour_clock_invalid = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.set24HourClock", "params": {"is24Hour": 1234}}' http://127.0.0.1:55555/jsonrpc'''


set_color_blink = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setBlink", "params": {"blinkInfo": {
            "ledIndicator": "power_led",
            "iterations": 10,
            "pattern": [
                {
                    "brightness": 50,
                    "duration": 100,
                    "color": "red",
                    "red": 0,
                    "green": 0,
                    "blue": 0
                }
            ]
        }
    }}' http://127.0.0.1:55555/jsonrpc'''


set_color_blink_invalid = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setBlink", "params": {"blinkInfo": {
            "ledIndicator": "power_led",
            "iterations": 10,
            "pattern": [
                {
                    "brightness": 0X77,
                    "duration": "invalid",
                    "color": "black",
                    "red": 9876,
                    "green": "ABCD",
                    "blue": 0
                }
            ]
        }
    }}' http://127.0.0.1:55555/jsonrpc'''

set_brightness = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setBrightness", "params": {"brightness": 50,"index": "power_led"}}' http://127.0.0.1:55555/jsonrpc'''

set_brightness_invalid = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setBrightness", "params": {"brightness": "ABCD","index": "power_led"}}' http://127.0.0.1:55555/jsonrpc'''

set_clock_brightness = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setClockBrightness", "params": {"brightness": 50}}' http://127.0.0.1:55555/jsonrpc'''
set_clock_test_pattern = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setClockTestPattern", "params": {"show": true,"timeInterval": 4}}' http://127.0.0.1:55555/jsonrpc'''

set_led = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setLED", "params": {"ledIndicator": "power_led",
        "brightness": 50,
        "color": "red",
        "red": 0,
        "green": 0,
        "blue": 0
    }}' http://127.0.0.1:55555/jsonrpc'''


set_led_invalid = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setLED", "params": {"ledIndicator": "power_led",
        "brightness": 999999999,
        "color": "no_colour",
        "red": "invalid_string",
        "green": 0,
        "blue": 0
    }}' http://127.0.0.1:55555/jsonrpc'''
set_preferences = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setPreferences", "params": {"preferences": {}}}' http://127.0.0.1:55555/jsonrpc'''

set_preferences_invalid = '''curl --silent --header "Content-Type: application/json" --request POST -d '{"jsonrpc": "2.0", "id": 42,
"method":"org.rdk.FrontPanel.setPreferences", "params": {"preferences": {1234}}}' http://127.0.0.1:55555/jsonrpc'''


