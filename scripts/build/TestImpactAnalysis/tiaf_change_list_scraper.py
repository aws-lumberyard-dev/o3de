
import git
import json
from collections import namedtuple
import math
DEBUG = False

def main(include_filters: list):

    files_by_extension = {}
    total_changes = 0

    def diff_list_to_str_paths(changelist):
        paths = []
        for change in changelist:
            file_name = change.a_path
            file_extension = "." + file_name.split(".")[-1]
            files_for_extension = files_by_extension.get(file_extension, [])
            files_for_extension.append(file_name)
            files_by_extension[file_extension] = files_for_extension
            paths.append(f"{change.a_path}")
        return paths

    x = 100
    pr_commits = {}
    with git.Repo("C:/o3de") as repo:
        head = repo.head.commit
        for i in range(1,x):
            parent_count = len(head.parents)
            if parent_count > 1:
                if DEBUG: print(head.summary)
                diff = head.diff(head.parents[0])
                change_paths_for_this_commit = diff_list_to_str_paths(diff)
                total_changes = total_changes + len(change_paths_for_this_commit)
                pr_commits[str(head)] = change_paths_for_this_commit
            head = head.parents[0]

    extension_statistics = {}
    
    for file, entry in files_by_extension.items():
        count = len(entry)
        percentage = count/total_changes * 100
        statistics_obj = ExtensionStatistics(file_extension=file, count=count, percentage=percentage)
        extension_statistics[file] = statistics_obj
    
    for extension in include_filters:
        print(extension_statistics.get(extension, f"The extension {extension} did not appear in this set of changelists."))

class ExtensionStatistics():
    
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
    include_filters = [".h", ".hpp", ".hxx", ".inl", ".c", ".cpp", ".cxx", ".py"]
    main(include_filters)