"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import azlmbr.asset as asset
import azlmbr.atomtools.materialutils as materialutils
from PySide2 import QtWidgets

def main():
    print("==== Begin re-save all materials script ==========================================================")
    
    folder = QtWidgets.QFileDialog.getExistingDirectory()

    if not folder:
        print("==== Canceled re-save all materials script ==========================================================")
        return

    for root, dirs, files in os.walk(folder, topdown=False):
        for name in files:
            if os.path.splitext(name)[1] == ".material":
                materialPath = os.path.join(root, name)
                print("Re-saving '{}'...".format(materialPath))
                azlmbr.atomtools.materialutils.UpgradeMaterialFile(materialPath)


    print("==== End re-save all materials script ==========================================================")

if __name__ == "__main__":
    main()

