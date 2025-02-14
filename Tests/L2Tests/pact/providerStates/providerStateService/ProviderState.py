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

import json
import re
import log_handler
import os

logger = log_handler.setup_logger('ProviderStateRegexPatterns')


"""Class that holds all methods to be performed on provider state string"""
class ProviderStateRegexPatterns:
    jsonrpc_provider_state_mapping = {}
    file_name = os.path.join(os.path.dirname(__file__), "jsonrpc_config.json")
    with open(file_name, 'r') as file:
        jsonrpc_provider_state_mapping = json.load(file)
    def __init__(self, state):
        """Constructor method to initialize Provider State
        """
        self.state = state
        self.jsonrpc = None

    """Method to get the json rpc message """
    def get_jsonrpc_msg_to_post(self):
        jsonrpc_msg = None
        if len(self.state) > 0:
            logger.info("State string available ----- : ----- {}".format(self.state))
            for configItem in ProviderStateRegexPatterns.jsonrpc_provider_state_mapping:
                self.jsonrpc = ProviderStateRegexPatterns.jsonrpc_provider_state_mapping[configItem]
                if self.jsonrpc["regex_patterns"]:
                    if re.search(self.jsonrpc["regex_patterns"], self.state):
                        params = re.search(self.jsonrpc["regex_patterns"], self.state)
                        jsonrpc_msg = self.add_params_to_jsonrpc(configItem, params)
        else:
            jsonrpc_msg = "NOT_APPLICABLE"

        if jsonrpc_msg is None:
            jsonrpc_msg = "NOT_AVAILABLE"
        elif jsonrpc_msg != "NOT_APPLICABLE":
            jsonrpc_msg = json.dumps(jsonrpc_msg)

        return jsonrpc_msg

    def add_params_to_jsonrpc(self, json_key, params):
        logger.info(f"Method : {json_key}")
        jsonrpc_msg = None
        if json_key == "LAUNCH_APP":
            self.jsonrpc["jsonrpc"]["params"]["callsign"] = params.group(1)
            self.jsonrpc["jsonrpc"]["params"]["client"] = params.group(1)
            jsonrpc_msg = self.jsonrpc["jsonrpc"]
        elif json_key == "SET_PERSISTENT_STORE":
            self.jsonrpc["jsonrpc"]["params"]["namespace"] = params.group(2)
            self.jsonrpc["jsonrpc"]["params"]["key"] = params.group(1)
            self.jsonrpc["jsonrpc"]["params"]["value"] = params.group(3)
            jsonrpc_msg = self.jsonrpc["jsonrpc"]
        elif json_key == "TEXT_TO_SPEECH_SET_ACL":
            logger.info("jsonRpc =====>> {}".format(self.jsonrpc["jsonrpc"]["params"]["accesslist"]))
            jsonrpc_msg = self.jsonrpc["jsonrpc"]

        return jsonrpc_msg


# if __name__ == '__main__':
#     providerStateService = ProviderStateRegexPatterns('"key:testKey", "namespace: testNamesapce" to "testValue" is set in persistentStore')
#     jsonrpc = providerStateService.get_jsonrpc_msg_to_post()
#     print(jsonrpc)
