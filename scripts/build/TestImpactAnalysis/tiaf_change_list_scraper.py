from unittest import skip
import git
import json
DEBUG = False

def main():
    x = 10
    pr_commits = {}
    with git.Repo("C:/o3de") as repo:
        head = repo.head.commit
        for i in range(1,x):
            parent_count = len(head.parents)
            if parent_count > 1:
                if DEBUG: print(head.summary)
                diff = head.diff(head.parents[0])
                pr_commits[str(head)] = diff
            head = head.parents[0]
            
    print(len(pr_commits))
    with open("jsonchangelists.json", "w+") as f:
        json.dump(pr_commits, f, skipkeys=True)




if __name__ == "__main__":
    main()