#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import csv
from extension_statistics import ExtensionStatistics

class DataWriter():
    """
    Class for helper functions for I/O operations.
    """
    
    @classmethod
    def dump_to_csv(self, extension_statistics, csv_file_path):
        """
        Write out our extension statistics map into an easily digestable .csv file for data analysis. 
        This function will support writing out a;; properties that are in the writeable_properties list of ExtensionStatistics as long as they can be written by CSVWriter.
        @param extension_statistics: A map of file extensions to their respective extension_statistics objects
        @param csv_file_path: Path to store our csv at.
        """
        try:
            with open(f"{csv_file_path}.csv", "w+", newline='') as file:
                writer = csv.writer(file, delimiter=",")
                writer.writerow(ExtensionStatistics.writeable_properties)
                for extension, obj in extension_statistics.items():
                    property_values = []
                    for property_name in ExtensionStatistics.writeable_properties:
                        property_values.append(getattr(obj, property_name))
                    writer.writerow(property_values)
        except PermissionError or FileExistsError or FileNotFoundError as e:
            print(e)

    @classmethod
    def print_extension_statistics(self, extension_statistics, include_filters):
        """
        Prints out extension statistics. Prints out the extensions in our include filters list first, then prints out the excluded extensions.
        @param extension_statistics: A map of ExtensionStatistics objects to write out to the console
        @param include_filters: List of file extension names that we are including.
        """
        include_list = []
        exclude_list = []

        for extension_name, extension in extension_statistics.items():
            if extension_name in include_filters:
                include_list.append(extension)
            else:
                exclude_list.append(extension)

        # Sort our lists so that they are displayed in a nicer fashion
        get_percentage = lambda extension_statistic: extension_statistic.percentage_raw
        sorted_include = sorted(include_list, key=get_percentage, reverse=True)
        sorted_exclude = sorted(exclude_list, key=get_percentage, reverse=True)

        print("---- INCLUDED FILE EXTENSIONS ----")
        for extension in sorted_include:
            print(extension)
        print("---- EXCLUDED FILE EXTENSIONS ----")
        for extension in sorted_exclude:
            print(extension)

    @classmethod
    def dump_tiaf_compatible_change_lists_to_json(self, changelist_map, file_path):
        """
        Takes a the pr_commits map and breaks it down into individual created, updated, and deleted changelists for each commit, dumps it to the location file_path as json.
        @param pr_commits: A dictionary mapping pr_commits to their associated change objects.
        @param file_path: The location to store our JSON.
        """        
        # For each commit, we have an associated map of changes. Write out that map.
        for pr, change_map in changelist_map.items():
            commit = change_map['commit']
            DataWriter.dump_to_file(change_map, f"{file_path}/{commit}_changelist.json")

    @classmethod
    def dump_to_file(self, data, file_path):
        """
        Dumps a data object to the specified file.
        @param data: The data to store.
        @param file_path: The file path to store the data at.
        """
        try:
            with open(file_path, "w") as f:
                json.dump(data, f, indent=4, sort_keys=True)
        except PermissionError or FileExistsError or FileNotFoundError as e:
            print(e)
                    