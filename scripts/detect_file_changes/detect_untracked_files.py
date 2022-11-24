#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import git
import sys


def main():
    repo = git.Repo(".", search_parent_directories=True)
    if len(repo.untracked_files):
        print("There are untracked files in the repository:")
        for file in repo.untracked_files:
            print("\t", file)
        sys.exit(1)

    print("There are no untracked files in the repository.")
    sys.exit(0)

if __name__ == '__main__':
    main()