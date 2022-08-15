#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

class ExtensionStatistics():
    """
    A data structure for storing statistics about a file extension.
    """

    # Properties that can be written out to a CSV file by a DataWriter.
    writeable_properties = ["percentage", "percentage_raw","file_ext","count"]
    
    def __init__(self, *args, **kwargs):
        self._count = kwargs.pop('count')
        self._percentage = kwargs.pop('percentage')
        self._file_ext = kwargs.pop('file_extension')

    @property
    def percentage(self):
        """
        Rounded percentage of file changes this extension is responsible for.
        """
        return round(self._percentage, 2)

    @property
    def percentage_raw(self):
        """
        Unrounded percentage of file changes this extension responsible for.
        """
        return self._percentage

    @property
    def file_ext(self):
        """
        The file extension this object represents.
        """
        return self._file_ext

    @property
    def count(self):
        """
        Raw count of how many times this file extension has appeared.
        """
        return self._count

    def __str__(self):
        return f"{self.file_ext} occurred {self.count} times, and made up {self.percentage}% of total file changes."