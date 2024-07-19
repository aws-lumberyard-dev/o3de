#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import tkinter as tk
from tkinter import ttk
import os
import sys

import multiple_file_picker
sys.path.append('..')

import export_project


class MainWindow(tk.Tk):

    def __init__(self):
        super().__init__()

        self.title("Project Export Settings")
        self.geometry("600x250")
        self.eval('tk::PlaceWindow . center')

        try:
            self.export_config = export_project.get_export_project_config(os.getcwd())
        except:
            self.export_config = export_project.get_export_project_config(None)

        if self.export_config.is_global:
            self.title("Project Export Settings (Global)")
            pass
        else:
            self.title(f"Project Export Settings ({self.export_config.project_name})")

        self.columnconfigure(0, weight=1)

        self._main_frame = tk.Frame(self)
        self._main_frame.columnconfigure(0, weight=1)
        self._main_frame.grid(sticky=tk.NSEW)

        self._init_general_settings(self._main_frame)

        result_string = multiple_file_picker.Dialog(self, (("Seed Files", "*.seed")), "Seed1.seedlist; Seed2.seedlist").get_result()

        pass

    def add_labeled_text_entry(self, parent: tk.Frame, label_text: str, default_value: str = "", entry_colspan: int = 1,
                               label_width: int = 0, entry_read_only: bool = False):
        label = tk.Label(parent, text=label_text, anchor=tk.W, width=label_width)
        label.grid(column=0, padx=5, pady=2, sticky=tk.W)
        row_line = label.grid_info().get("row")

        entry = tk.Entry(parent, justify='right', state=tk.DISABLED if entry_read_only else tk.NORMAL)
        entry.grid(row=row_line, column=1, padx=5, pady=2, sticky=tk.EW, columnspan=entry_colspan)

        entry_var = tk.StringVar()
        entry_var.set(default_value)
        entry["textvariable"] = entry_var
        return entry_var

    def add_labeled_dropdown_entry(self, parent: tk.Frame, label_text: str, choices: list[str], default_value: str = "", entry_colspan: int = 1,
                                   label_width: int = 0):
        label = tk.Label(parent, text=label_text, anchor=tk.W, width=label_width)
        label.grid(column=0, padx=5, pady=2, sticky=tk.W)
        row_line = label.grid_info().get("row")

        assert len(choices) > 0, "choices must not be empty"

        if default_value == '':
            default_value = choices[0]

        assert default_value in choices, f"{default_value} is not in the list of choices ({','.join(choices)})"

        entry_var = tk.StringVar()
        entry_var.set(default_value)

        entry = tk.OptionMenu(parent, entry_var,  *choices)
        entry.grid(row=row_line, column=2, padx=5, pady=2, sticky=tk.E, columnspan=entry_colspan)

        return entry_var

    def add_labeled_checkbox(self, parent: tk.Frame, label_text: str,default_value: bool, entry_colspan: int = 1,
                                   label_width: int = 0):

        check_button_var = tk.IntVar()
        check_button_var.set(1 if default_value else 0)
        button = tk.Checkbutton(parent, text=label_text, variable=check_button_var,onvalue=1, offvalue=0, width=label_width)
        button.grid(column=2, sticky=tk.W)
        return check_button_var



    def _init_general_settings(self, parent):
        general_settings_frame = tk.LabelFrame(parent, text="General Settings")
        general_settings_frame.columnconfigure(0, weight=0)
        general_settings_frame.columnconfigure(1, weight=1)
        general_settings_frame.grid(padx=4, pady=4, sticky=tk.NSEW)

        self.a0 = self.add_labeled_text_entry(general_settings_frame,"Name", "Value", entry_colspan=3)

        build_config_choices = ['debug', 'profile', 'release']
        default_project_build_config = self.export_config.get_value('project.build.config', 'profile')
        self.a1 = self.add_labeled_dropdown_entry(parent=general_settings_frame,
                                                  label_text='Project Build Configuration',
                                                  default_value=default_project_build_config,
                                                  choices=build_config_choices,
                                                  entry_colspan=1)

        default_tool_build_config = self.export_config.get_value('tool.build.config', 'profile')
        self.a2 = self.add_labeled_dropdown_entry(parent=general_settings_frame,
                                                  label_text='Tool Build Configuration',
                                                  default_value=default_tool_build_config,
                                                  choices=build_config_choices,
                                                  entry_colspan=1)

        archive_format_choices = ['none', 'zip', 'gzip', 'bz2', 'xz']
        default_archive_format = 'none'
        self.a3 = self.add_labeled_dropdown_entry(parent=general_settings_frame,
                                                  label_text='Project Build Configuration',
                                                  default_value=default_archive_format,
                                                  choices=archive_format_choices,
                                                  entry_colspan=1)

        default_build_assets = self.export_config.get_boolean_value('option.build.assets')
        self.a4 = self.add_labeled_checkbox(parent=general_settings_frame,
                                            label_text="Build Assets",
                                            default_value=default_build_assets)

        default_fail_on_build_asset_errors = self.export_config.get_boolean_value('option.fail.on.asset.errors')
        self.a5 = self.add_labeled_checkbox(parent=general_settings_frame,
                                            label_text="Fail on Build Assets Errors",
                                            default_value=default_fail_on_build_asset_errors)
        




if __name__ == '__main__':

    project_export_ui = MainWindow()

    project_export_ui.mainloop()





