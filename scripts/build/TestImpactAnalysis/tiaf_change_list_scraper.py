
import csv
from distutils import extension
import git
import json


DEBUG = False
RAW_PATH_FILE = "commits_raw.json"

class GitScraper():

    def __init__(self):
        self.file_count = 0

    def get_commit_data_from_file(self, file_path) -> dict:
        with open(file_path) as f:
            data = json.load(f)
        return data

    def get_commit_data_from_git(self, repo_path, commits) -> dict:
        pr_commits = {}
        commit_range = range(0, commits)
        with git.Repo(repo_path) as repo:
            head = repo.head.commit
            for i in commit_range:
                parent_count = len(head.parents)
                if parent_count > 1:
                    diff = head.diff(head.parents[0])
                    change_file_paths_for_this_commit = self.diff_to_paths(diff)
                    pr_commits[str(head)] = change_file_paths_for_this_commit
                head = head.parents[0]
        return pr_commits

    def dump_to_file(self, data, file_path):
        with open(file_path, "w+") as f:
            json.dump(data, f)

    def diff_to_paths(self, changelist):
        paths = []
        for change in changelist:
            paths.append(f"{change.a_path}")
        return paths

    def extract_files_by_extension(self, commits):
        files_by_extension = {}
        for commit, changelist in commits.items():
            for file_name in changelist:
                file_extension = "." + file_name.split(".")[-1]
                files_for_extension = files_by_extension.get(file_extension, [])
                files_for_extension.append(file_name)
                files_by_extension[file_extension] = files_for_extension
                self.file_count += 1
        return files_by_extension

    @property
    def total_changes(self):
        return self.file_count


def main(include_filters: list, get_from_git, repo_path=None, no_of_commits=None, csv_file_path=None):
    files_by_extension = {}
    extension_statistics = {}
    total_changes = 0

    scraper = GitScraper()

    if(get_from_git):
        pr_commits = scraper.get_commit_data_from_git(repo_path, no_of_commits)
        scraper.dump_to_file(pr_commits, RAW_PATH_FILE)
    else:
        pr_commits = scraper.get_commit_data_from_file(RAW_PATH_FILE)

    files_by_extension = scraper.extract_files_by_extension(pr_commits)
    total_changes = scraper.total_changes
    for file, entry in files_by_extension.items():
        count = len(entry)
        percentage = count/total_changes * 100
        statistics_obj = ExtensionStatistics(file_extension=file, count=count, percentage=percentage)
        extension_statistics[file] = statistics_obj

    if(DEBUG):
        print_extension_statistics(extension_statistics, include_filters)
    if(csv_file_path):
        dump_to_csv(extension_statistics, csv_file_path)
    
def dump_to_csv(extension_statistics, csv_file_path):

    with open(csv_file_path, "w+", newline='') as file:
        writer = csv.writer(file, delimiter=",")
        writer.writerow(ExtensionStatistics.writeable_properties)
        for extension, obj in extension_statistics.items():
            property_values = []
            for property_name in ExtensionStatistics.writeable_properties:
                property_values.append(getattr(obj, property_name))
            writer.writerow(property_values)

def print_extension_statistics(extension_statistics, include_filter):
    include_list = []
    exclude_list = []
    for extension_name, extension in extension_statistics.items():
        if extension_name in include_filters:
            include_list.append((extension.percentage, str(extension)))
        else:
            exclude_list.append((extension.percentage, str(extension)))

    get_percentage = lambda x : x[0]
    sorted_include = sorted(include_list, key=get_percentage, reverse=True)
    sorted_exclude = sorted(exclude_list, key=get_percentage, reverse=True)
    print("---- INCLUDED FILE EXTENSIONS ----")
    for percentage, extension in sorted_include:
        print(extension)
    print("---- EXCLUDED FILE EXTENSIONS ----")
    for percentage, extension in sorted_exclude:
        print(extension)

class ExtensionStatistics():

    writeable_properties = ["percentage", "percentage_raw","file_ext","count"]
    
    def __init__(self, *args, **kwargs):
        self._count = kwargs.pop('count')
        self._percentage = kwargs.pop('percentage')
        self._file_ext = kwargs.pop('file_extension')

    @property
    def percentage(self):
        return round(self._percentage, 2)

    @property
    def percentage_raw(self):
        return self._percentage

    @property
    def file_ext(self):
        return self._file_ext

    @property
    def count(self):
        return self._count

    def __str__(self):
        return f"{self.file_ext} occurred {self.count} times, and made up {self.percentage}% of total file changes."

if __name__ == "__main__":
    include_filters = [".h", ".hpp", ".hxx", ".inl", ".c", ".cpp", ".cxx", ".py", ".xml"]
    main(include_filters, False, "C:/o3de", 2000, "commit_info.csv")