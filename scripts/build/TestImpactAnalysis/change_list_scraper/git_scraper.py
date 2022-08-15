import json
import git

class GitScraper():

    def __init__(self):
        self.file_count = 0

    def get_commit_data_from_file(self, file_path) -> dict:
        try:
            with open(file_path) as f:
                data = json.load(f)
            return data
        except FileNotFoundError as e:
            print("Error, file not found. Perhaps it hasn't been generated yet! Try running with --git")

    def get_commit_data_from_git(self, repo_path, prs_to_get) -> dict:
        """
        Gets prs_to_get number of pull requests from the specified repo, and returns a map of each pull_request commit to the changed files in that commit
        Pull requests are collected in chronological order, latest first.
        @param repo_path: File path to the local representation of the repository to explore
        @param prs_to_get: The number of PR's to collect and return in the map.
        """
        pr_commits = {}
        with git.Repo(repo_path) as repo:
            head = repo.head.commit
            # While our map is smaller than the number of prs we need to collect
            while len(pr_commits) < prs_to_get:
                parent_count = len(head.parents)
                # If head.parents is greater than 1, then we can assume this is a Pull Request merge commit
                if parent_count > 1:
                    # Diff these two commits to get the change list of the pull request, extract the data for it and then map it to the commit hash of the pr commit
                    diff = head.diff(head.parents[0])
                    change_list_for_this_commit = self.get_change_objects(diff)
                    pr_commits[str(head)] = change_list_for_this_commit
                head = head.parents[0]
        return pr_commits

    def get_change_objects(self, changelist):
        """
        
        """
        change_map = {}
        for change in changelist:
            list_for_this_type = change_map.get(change.change_type, [])
            list_for_this_type.append((f"{change.a_path}", change.change_type))
            change_map[change.change_type] = list_for_this_type
        return change_map

    def extract_files_by_extension(self, commits):
        files_by_extension = {}
        for commit, change_map in commits.items():
            for change_list in change_map.values():
                for file_name, change_type in change_list:
                    file_extension = "." + file_name.split(".")[-1]
                    files_for_extension = files_by_extension.get(file_extension, [])
                    files_for_extension.append(file_name)
                    files_by_extension[file_extension] = files_for_extension
                    self.file_count += 1
        return files_by_extension

    @property
    def total_changes(self):
        return self.file_count