# -*- coding: utf-8 -*-
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

"""
Class to parse the Excel file containing provider state lookup
data and return the corresponding commands for a given state.
-----------------------------------------------------------------
Filename        : excelParser.py
-----------------------------------------------------------------

@author: kmurug485
"""
import pandas as pd
import os
import log_handler

logger = log_handler.setup_logger('ExcelParser')


class ExcelParser:
    __state = "Provider state string"
    __commands = "Commands"

    def __init__(self):
        self.file_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), "providerStateService", "lookup_excels", "Provider-state-lookup.xlsx")
        self.data = None
        self.load_data()

    def load_data(self):
        """
        Loads data from the Excel file, ensuring the required columns are present, handling merged cells, and grouping commands by state.
        """
        try:
            # Load the Excel file
            self.data = pd.read_excel(self.file_path)
            # Ensure the required columns are present
            if ExcelParser.__state not in self.data.columns or ExcelParser.__commands not in self.data.columns:
                raise ValueError("Excel sheet must contain 'state' and 'commands' columns")

            # Forward fill the state column to handle merged cells
            # self.data[ExcelParser.__state] = self.data[ExcelParser.__state].fillna(method='ffill')
            self.data[ExcelParser.__state] = self.data[ExcelParser.__state].ffill()

            # Group commands by state
            self.data = self.data.groupby(ExcelParser.__state)[ExcelParser.__commands].apply(
                lambda x: x.dropna().tolist()).reset_index()

        except Exception as e:
            logger.error(f"Error loading Excel file: {e}")
            self.data = None

    def get_commands(self, state):
        """
        Returns a list of commands for the specified state. If no commands are found, returns 'NOT_AVAILABLE'. If the data is not loaded, returns 'FAILURE'.

        Args:
            state (str): The state for which to find the commands.
        """
        if self.data is None:
            logger.error("Lookup table not loaded")
            return ['FAILURE']

        # Find the commands for the specified state
        state_row = self.data[self.data[ExcelParser.__state] == state]
        if state_row.empty:
            logger.info(f"No commands found for state: {state}")
            return ['NOT_AVAILABLE']

        return state_row[ExcelParser.__commands].values[0]


# Example usage
# if __name__ == "__main__":
#     reader = ExcelParser()
#     state = '"key:testKey", "namespace:testNamespace", "scope:device" is retrieved from persistentStore with value "testValue1"'
#     commands = reader.get_commands(state)
#     print(commands)
#     for command in commands:
#         print(command)
#         print("\n")
