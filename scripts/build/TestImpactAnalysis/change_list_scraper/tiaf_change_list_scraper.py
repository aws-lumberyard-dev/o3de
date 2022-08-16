#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from git_scraper import GitScraper
from extension_statistics import ExtensionStatistics
from data_writer import DataWriter
import argparse
import pathlib


def parse_args():
    parser = argparse.ArgumentParser()

    def valid_folder(value):
        if(pathlib.Path(value).is_dir()):
            return value
        raise FileNotFoundError(value)

    parser.add_argument(
        "--git",
        action="store_true",
        help="Set this flag to get data from github rather than from local storage.",
        required=False
    )

    parser.add_argument(
        "--include-extensions",
        help="File extensions to include in our filtering, delinated by commas. I.Et \".xyz, .yyx ,\".",
        required=False
    )

    parser.add_argument(
        "--repo",
        type=valid_folder,
        help="The location of the local folder with the repository we want to scrape.",
        required=True
    )

    parser.add_argument(
        "--pr",
        type=int,
        help="Number of Pull Requests to get information on.",
        required=True
    )

    parser.add_argument(
        "--csv",
        help="Path to where to store csv data.",
        required=False
    )

    parser.add_argument(
        "--changelists",
        type=valid_folder,
        help="Path to the folder to store the TIAF compatible change lists for each pull request.",
        required=False
    )

    parser.add_argument(
        "--debug",
        action="store_true",
        required=False
    )

    parser.add_argument(
        "--path_analysis",
        action="store_true",
        help="Should path analysis be used in this run.",
        required=False
    )

    args = parser.parse_args()

    return args


RAW_PATH_FILE = "build/commits_raw.json"

def analyse_associations_by_changelist(change_list_map):
    analysis_map = {}
    for pr, map in change_list_map.items():
        common_paths = {}
        del map['commit']
        purepaths = []
        for change_list in map.values():
            for path in change_list:
                purepaths.append(pathlib.PurePath(path))
                # Sort alphabetically, go from root deeper into structure. Stop when you find the first directory that is different.
        purepaths = sorted(purepaths)
        
        for i in range(0, len(purepaths)-1):
            first_file = purepaths[i]
            second_file = purepaths[i+1]
            previous_path = pathlib.Path('.')
            depth = 0
            for first_path, second_path in zip(first_file.parts, second_file.parts):
                if not first_path == second_path:
                    if previous_path:
                        associated_paths = common_paths.get(previous_path,set())
                        associated_paths.add(first_file)
                        associated_paths.add(second_file)
                        common_paths[previous_path] = associated_paths
                        break
                previous_path = pathlib.Path.joinpath(previous_path, first_path)
                depth += 1
        analysis_map[pr] = common_paths
    return analysis_map

def analyse_associations_holistically(change_list_map):
    purepaths = []
    common_paths = {}
    for pr, map in change_list_map.items():
        del map['commit']
        
        for change_list in map.values():
            for path in change_list:
                purepaths.append(pathlib.PurePath(path))
                # Sort alphabetically, go from root deeper into structure. Stop when you find the first directory that is different.
    purepaths = sorted(purepaths)
        
    for i in range(0, len(purepaths)-1):
        first_file = purepaths[i]
        second_file = purepaths[i+1]
        previous_path = pathlib.Path('.')
        depth = 0
        for first_path, second_path in zip(first_file.parts, second_file.parts):
            if not first_path == second_path:
                if previous_path:
                    associated_paths = common_paths.get(previous_path,set())
                    associated_paths.add(first_file)
                    associated_paths.add(second_file)
                    common_paths[previous_path] = associated_paths
                    break
            previous_path = pathlib.Path.joinpath(previous_path, first_path)
            depth += 1
    return common_paths


def main(include_filters=[], get_from_git=False, repo_path=None, prs_to_get=None, csv_file_path=None, change_lists_folder=None, path_analysis=None):
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
        statistics_obj = ExtensionStatistics(
            file_extension=file, count=count, percentage=percentage)
        extension_statistics[file] = statistics_obj
    if(change_lists_folder):
        change_list_map = scraper.extract_change_lists(pr_commits)
        DataWriter.dump_tiaf_compatible_change_lists_to_json(
            change_list_map, change_lists_folder)

    if(path_analysis):
        PER_CHANGE_LIST = True
        change_list_map = scraper.extract_change_lists(pr_commits)
        if(PER_CHANGE_LIST):
            analysis_map = analyse_associations_by_changelist(change_list_map)
        else:
            analysis_map = analyse_associations_holistically(change_list_map)

    if(DEBUG):
        DataWriter.print_extension_statistics(
            extension_statistics, include_filters)
    if(csv_file_path):
        DataWriter.dump_to_csv(extension_statistics, csv_file_path)


if __name__ == "__main__":
    default_filters = [".h", ".hpp", ".hxx",
                       ".inl", ".c", ".cpp", ".cxx", ".py", ".xml"]

    args = vars(parse_args())

    DEBUG = args.get('debug', False)
    include_filters = args.get('include_extensions', default_filters)
    if(isinstance(include_filters, str)):
        include_filters = include_filters.split(',')
    main(
        include_filters=include_filters,
        get_from_git=args.get('git', False),
        repo_path=args.get('repo'),
        prs_to_get=args.get('pr'),
        csv_file_path=args.get('csv', None),
        change_lists_folder=args.get('changelists', None),
        path_analysis=args.get('path_analysis', None))
