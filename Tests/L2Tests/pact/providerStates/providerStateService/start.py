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

from flask import Flask
from router import router
import logging
import os

log_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "logs")
lookup_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "providerStateService", "lookup_excels")
if not os.path.exists(log_dir):
    os.makedirs(log_dir)

if not os.path.exists(lookup_dir):
    os.makedirs(lookup_dir)

log_file = os.path.join(log_dir, "provider-state-service.log")
logging.basicConfig(filename=log_file, level=logging.DEBUG, format='[%(levelname)s][%(asctime)s][%(name)s] - %(message)s')
app = Flask(__name__)
app.register_blueprint(router)
app.run(debug=True, port=5003)
