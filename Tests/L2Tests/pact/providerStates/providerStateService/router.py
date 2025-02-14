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
from flask import Flask
from flask import request
from flask import Blueprint
import requests
from ProviderState import *
import json
import log_handler
import os
from excelParser import ExcelParser
import signal
router = Blueprint("app", __name__)

logger = log_handler.setup_logger('Router')

# import env variables : port and host
port = os.environ.get("PORT","9998")
host = os.environ.get("HOST","127.0.0.1")

jsonrpc_url = "http://" + host + ":" + port + "/jsonrpc"

"""Provider state end point is defined below"""
@router.route('/',methods=['POST','GET'])
def index():
    logger.info(request.method)
    if request.method == 'POST':
        logger.info("Request method is post")
        args = request.get_json(force=True)

        # Get commands based on the state passed
        command_reader = ExcelParser()
        commands = command_reader.get_commands(args['state'])

        # Execute the commands
        for command in commands:
            if command == 'NOT_AVAILABLE':
                logger.debug("Command is not available")
                return "NOT_AVAILABLE"
            elif command == 'FAILURE':
                logger.debug("Command failed")
                return "FAILED"
            else:
                logger.info("command for state : {}".format(command))
                post_request = requests.post(jsonrpc_url, data=command)
                logger.info("############################# Response #############################\n")
                logger.info("response : ", post_request, "\n")
                logger.info("####################################################################")
                return "OK"
    else:
        logger.info("Request method is get")
        return 'GET'

# Shutdown route
@router.route('/shutdown', methods=['POST'])
def shutdown():
    if request.method == 'POST':
        shutdown_server()
        return jsonify({"message": "Server shutting down..."}), 200
    return jsonify({"error": "Invalid request"}), 400

# Function to initiate the shutdown
def shutdown_server():
    # Get the process ID (PID) of the current server process
    pid = os.getpid()
    
    # Send a termination signal to the process
    os.kill(pid, signal.SIGTERM)

# @router.before_app_first_request
# def startup():
#     #Check RDK SHELL is running
#     #todo
#
#     #Activating RDK Shell
#     try:
#         jsonrpc = json.dumps(ProviderStateRegexPatterns.jsonrpc_provider_state_mapping["ACTIVATE_RDKSHELL"]["jsonrpc"])
#         # jsonrpc = "{\"jsonrpc\":\"2.0\",\"id\":\"42\",\"method\": \"Controller.1.activate\", \"params\":{\"callsign\":\"org.rdk.RDKShell\"}}"
#         logger.info("jsonrpc message for state : {}".format(jsonrpc))
#         # Check if the command executed successfully
#         r = requests.post(jsonrpc_url, data=jsonrpc)
#         logger.info("################# Response from RDK SHELL activation ############### \n"+r.text)
#         logger.info("####################################################################")
#     except Exception as e:
#         logger.error("Exception while requesting " + str(e))


# if __name__ == "__main__":
#     app.run(debug = True,port=5003)
