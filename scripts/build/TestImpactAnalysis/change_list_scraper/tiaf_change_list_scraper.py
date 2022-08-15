#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from .git_scraper import GitScraper
from .extension_statistics import ExtensionStatistics
from .data_writer import DataWriter

DEBUG = True
RAW_PATH_FILE = "commits_raw.json"


def main(include_filters=[], get_from_git=False, repo_path=None, prs_to_get=None, csv_file_path=None, change_lists_folder=None):
    files_by_extension = {}
    extension_statistics = {}
    total_changes = 0

    scraper = GitScraper()

    if(get_from_git):
        pr_commits = scraper.get_commit_data_from_git(repo_path, prs_to_get)
        DataWriter.dump_to_file(pr_commits, RAW_PATH_FILE)
    else:
        pr_commits = scraper.get_commit_data_from_file(RAW_PATH_FILE)

    files_by_extension = scraper.extract_files_by_extension(pr_commits)
    total_changes = scraper.total_changes
    for file, entry in files_by_extension.items():
        count = len(entry)
        percentage = count/total_changes * 100
        statistics_obj = ExtensionStatistics(file_extension=file, count=count, percentage=percentage)
        extension_statistics[file] = statistics_obj

    if(change_lists_folder):
        DataWriter.dump_tiaf_compatible_change_lists_to_json(pr_commits, change_lists_folder)
    if(DEBUG):
        DataWriter.print_extension_statistics(extension_statistics, include_filters)
    if(csv_file_path):
        DataWriter.dump_to_csv(extension_statistics, csv_file_path)

if __name__ == "__main__":
    include_filters = [".h", ".hpp", ".hxx", ".inl", ".c", ".cpp", ".cxx", ".py", ".xml"]
    main(include_filters=include_filters, get_from_git=True, repo_path="C:/o3de", prs_to_get=10, csv_file_path="commit_info.csv")
