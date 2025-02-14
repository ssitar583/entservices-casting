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

import datetime
import logging
from logging.handlers import RotatingFileHandler
import os


def setup_logger(name):
    # Create a logger
    logger = logging.getLogger(name)

    logger.setLevel(logging.INFO)
    log_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "logs")
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)

    log_file = os.path.join(log_dir, "pact_service.log")
    # Create a rotating file handler
    handler = logging.handlers.TimedRotatingFileHandler(filename=log_file, when='M', backupCount=5)
    handler.setLevel(logging.DEBUG)
    # Create a formatter
    formatter = logging.Formatter('[%(levelname)s][%(asctime)s][%(name)s] - %(message)s')
    # Set the formatter for the handler
    handler.setFormatter(formatter)
    # Add the handler to the logger
    logger.addHandler(handler)

    # # Optionally, add a stream handler for console output
    # console_handler = logging.StreamHandler()
    # console_handler.setLevel(logging.DEBUG)
    # console_handler.setFormatter(formatter)
    # logger.addHandler(console_handler)

    return logger
