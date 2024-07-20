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
import multiple_entry

sys.path.append('..')

import export_project


class MainWindow(tk.Tk):

    def __init__(self):
        super().__init__()

        self.title("Project Export Settings")
        # self.geometry("600x250")
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

        self._init_source_engine_build_options(self._main_frame)

        self._init_project_build_options(self._main_frame)

        self._init_asset_bundling_options(self._main_frame)

        self._init_archive_settings(self._main_frame)






    def add_labeled_text_entry(self, parent: tk.Frame or tk.LabelFrame, label_text: str, default_value: str = "", entry_colspan: int = 1,
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

    def add_labeled_dropdown_entry(self, parent: tk.Frame or tk.LabelFrame, label_text: str, choices: list[str], default_value: str = "", entry_colspan: int = 1,
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

    def add_labeled_checkbox(self, parent: tk.Frame or tk.LabelFrame, label_text: str,default_value: bool, entry_colspan: int = 1,
                                   label_width: int = 0,
                                   row_number: int = None,
                                   column_number: int = None):

        check_button_var = tk.IntVar()
        check_button_var.set(1 if default_value else 0)
        button = tk.Checkbutton(parent, text=label_text, variable=check_button_var,onvalue=1, offvalue=0, width=label_width)
        button.grid(row=row_number, column=column_number, sticky=tk.W)
        return check_button_var

    def add_labeled_multifile_entry(self, parent: tk.Frame or tk.LabelFrame, label_text: str, default_value: str = "", entry_colspan: int = 1,
                                    label_width: int = 0, entry_read_only: bool = False, file_filters=None):
        label = tk.Label(parent, text=label_text, anchor=tk.W, width=label_width)
        label.grid(column=0, padx=5, pady=2, sticky=tk.W)
        row_line = label.grid_info().get("row")

        entry_var = tk.StringVar()
        entry_var.set("" if default_value is None else default_value)
        entry = tk.Entry(parent, textvariable=entry_var, justify='right', state=tk.DISABLED if entry_read_only else tk.NORMAL)
        entry.grid(row=row_line, column=1, padx=5, pady=2, sticky=tk.EW, columnspan=entry_colspan)

        button_add = tk.Button(parent, text="...", width=4, command=lambda : MainWindow.open_multifile_dialog(parent=parent,str_var=entry_var,file_filters=file_filters))
        button_add.grid(row=row_line, column=2, sticky=tk.E)

        return entry_var

    def add_labeled_multittext_entry(self, parent: tk.Frame or tk.LabelFrame, label_text: str, default_value: str = "", entry_colspan: int = 1,
                                     label_width: int = 0, entry_read_only: bool = False, file_filters=None):
        label = tk.Label(parent, text=label_text, anchor=tk.W, width=label_width)
        label.grid(column=0, padx=5, pady=2, sticky=tk.W)
        row_line = label.grid_info().get("row")

        entry_var = tk.StringVar()
        entry_var.set("" if default_value is None else default_value)
        entry = tk.Entry(parent, textvariable=entry_var, justify='right', state=tk.DISABLED if entry_read_only else tk.NORMAL)
        entry.grid(row=row_line, column=1, padx=5, pady=2, sticky=tk.EW, columnspan=entry_colspan)

        button_add = tk.Button(parent, text="...", width=4, command=lambda : MainWindow.open_multitext_dialog(parent=parent,
                                                                                                              str_var=entry_var))
        button_add.grid(row=row_line, column=2, sticky=tk.E)

        return entry_var

    @staticmethod
    def open_multifile_dialog(parent, str_var, file_filters):
        value = str_var.get()
        result = multiple_file_picker.Dialog(parent=parent, file_filters=file_filters,initial_list=value).get_result()
        str_var.set(result)

    @staticmethod
    def open_multitext_dialog(parent, str_var):
        value = str_var.get()
        result = multiple_entry.Dialog(parent=parent, input_value=value).get_result()
        str_var.set(result)

    def _init_archive_settings(self, parent):
        general_settings_frame = tk.LabelFrame(parent, text="Archive Settings")
        general_settings_frame.columnconfigure(0, weight=0)
        general_settings_frame.columnconfigure(1, weight=1)
        general_settings_frame.grid(padx=4, pady=4, sticky=tk.NSEW)

        archive_format_choices = ['none', 'zip', 'gzip', 'bz2', 'xz']
        default_archive_format = 'none'
        self.a3 = self.add_labeled_dropdown_entry(parent=general_settings_frame,
                                                  label_text='Archive Format',
                                                  default_value=default_archive_format,
                                                  choices=archive_format_choices,
                                                  entry_colspan=1)

        default_additional_game_project_file_patterns = self.export_config.get_value('additional.game.project.file.pattern.to.copy')
        self.additional_game_project_file_patterns = self.add_labeled_multittext_entry(parent=general_settings_frame,
                                                                                       label_text="Game Project File Patterns",
                                                                                       default_value=default_additional_game_project_file_patterns,
                                                                                       entry_colspan=2)

        default_additional_server_project_file_patterns = self.export_config.get_value('additional.server.project.file.pattern.to.copy')
        self.additional_server_project_file_patterns = self.add_labeled_multittext_entry(parent=general_settings_frame,
                                                                                         label_text="Server Project File Patterns",
                                                                                         default_value=default_additional_server_project_file_patterns,
                                                                                         entry_colspan=2)





        default_max_size = self.export_config.get_value("max.size")
        self.max_size = self.add_labeled_text_entry(parent=general_settings_frame,
                                                    label_text="Max Size",
                                                    default_value=default_max_size)



    def _init_source_engine_build_options(self, parent):
        tool_chain_build_options_frame = tk.LabelFrame(parent, text="Toolchain Build Options")
        tool_chain_build_options_frame.columnconfigure(0, weight=1)
        tool_chain_build_options_frame.grid(padx=8, sticky=tk.EW)
        build_config_choices = ['debug', 'profile', 'release']
        default_option_build_tools = self.export_config.get_boolean_value('option.build.tools')
        self.option_build_tools = self.add_labeled_checkbox(parent=tool_chain_build_options_frame,
                                                            label_text="Build Toolchain",
                                                            default_value=default_option_build_tools,
                                                            column_number=0)

        default_tool_build_config = self.export_config.get_value('tool.build.config', 'profile')
        self.a2 = self.add_labeled_dropdown_entry(parent=tool_chain_build_options_frame,
                                                  label_text='Tool Build Configuration',
                                                  default_value=default_tool_build_config,
                                                  choices=build_config_choices,
                                                  entry_colspan=3)

        default_build_tools_path = self.export_config.get_value("default.build.tools.path")
        self.build_tools_path = self.add_labeled_text_entry(parent=tool_chain_build_options_frame,
                                                            label_text="Toolchain Build Path",
                                                            default_value=default_build_tools_path,
                                                            entry_colspan=3)

    def _init_project_build_options(self, parent):
        project_build_options_frame = tk.LabelFrame(parent, text="Project Build Options")
        project_build_options_frame.columnconfigure(0, weight=1)
        project_build_options_frame.grid(padx=8, sticky=tk.EW)
        build_config_choices = ['debug', 'profile', 'release']
        default_project_build_config = self.export_config.get_value('project.build.config', 'profile')
        self.a1 = self.add_labeled_dropdown_entry(parent=project_build_options_frame,
                                                  label_text='Project Build Configuration',
                                                  default_value=default_project_build_config,
                                                  choices=build_config_choices,
                                                  entry_colspan=3)

        project_build_options_subframe = tk.Frame(project_build_options_frame, borderwidth=1,relief=tk.SOLID)
        project_build_options_subframe.grid(columnspan=3,sticky=tk.NSEW)

        default_option_build_game_launcher = self.export_config.get_boolean_value('option.build.game.launcher')
        self.option_build_game_launcher  = self.add_labeled_checkbox(parent=project_build_options_subframe,
                                                                     label_text="Build Game Launcher",
                                                                     default_value=default_option_build_game_launcher,
                                                                     row_number=0,
                                                                     column_number=0)

        default_option_build_unified_launcher = self.export_config.get_boolean_value('option.build.unified.launcher')
        self.option_build_unified_launcher  = self.add_labeled_checkbox(parent=project_build_options_subframe,
                                                                        label_text="Build Unified Launcher",
                                                                        default_value=default_option_build_unified_launcher,
                                                                        row_number=0,
                                                                        column_number=1)

        default_option_build_server_launcher = self.export_config.get_boolean_value('option.build.server.launcher')
        self.option_build_server_launcher  = self.add_labeled_checkbox(parent=project_build_options_subframe,
                                                                       label_text="Build Server Launcher",
                                                                       default_value=default_option_build_server_launcher,
                                                                       row_number=1,
                                                                       column_number=0)

        default_option_build_headless_server_launcher = self.export_config.get_boolean_value('option.build.headless.server.launcher')
        self.option_build_headless_server_launcher  = self.add_labeled_checkbox(parent=project_build_options_subframe,
                                                                                label_text="Build Headless Server Launcher",
                                                                                default_value=default_option_build_headless_server_launcher,
                                                                                row_number=1,
                                                                                column_number=1)

        default_option_build_monolithic = self.export_config.get_boolean_value('option.build.unified.launcher')
        self.option_build_monolithic  = self.add_labeled_checkbox(parent=project_build_options_frame,
                                                                  label_text="Build Monolithic",
                                                                  default_value=default_option_build_monolithic)

        default_android_build_path = self.export_config.get_value("default.android.build.path")
        self.android_build_path = self.add_labeled_text_entry(parent=project_build_options_frame,
                                                              label_text="Android build path",
                                                              default_value=default_android_build_path,
                                                              entry_colspan=3)

        default_android_build_path = self.export_config.get_value("default.ios.build.path")
        self.android_build_path = self.add_labeled_text_entry(parent=project_build_options_frame,
                                                              label_text="iOS build path",
                                                              default_value=default_android_build_path,
                                                              entry_colspan=3)

        default_launcher_build_path = self.export_config.get_value("default.launcher.build.path")
        self.launcher_build_path = self.add_labeled_text_entry(parent=project_build_options_frame,
                                                               label_text="Launcher Build Path",
                                                               default_value=default_launcher_build_path,
                                                               entry_colspan=3)

    def _init_asset_bundling_options(self, parent):
        project_asset_bundling_options_frame = tk.LabelFrame(parent, text="Asset/Bundling Options")
        project_asset_bundling_options_frame.columnconfigure(0, weight=1)
        project_asset_bundling_options_frame.grid(padx=8, sticky=tk.EW)

        default_build_assets = self.export_config.get_boolean_value('option.build.assets')
        self.a4 = self.add_labeled_checkbox(parent=project_asset_bundling_options_frame,
                                            label_text="Build Assets",
                                            default_value=default_build_assets)

        default_fail_on_build_asset_errors = self.export_config.get_boolean_value('option.fail.on.asset.errors')
        self.a5 = self.add_labeled_checkbox(parent=project_asset_bundling_options_frame,
                                            label_text="Fail on Build Assets Errors",
                                            default_value=default_fail_on_build_asset_errors)

        default_option_allow_registry_overrides = self.export_config.get_boolean_value('option.allow.registry.overrides')
        self.option_allow_registry_overrides  = self.add_labeled_checkbox(parent=project_asset_bundling_options_frame,
                                                                          label_text="Allow Registry Overrides",
                                                                          default_value=default_option_allow_registry_overrides)

        default_seedlist_file_paths = self.export_config.get_value('seedlist.paths')
        # TODO: Make sure the paths are project-relative
        self.seedlist_file_paths = self.add_labeled_multifile_entry(parent=project_asset_bundling_options_frame,
                                                                    label_text="Seed List File Paths",
                                                                    default_value=default_seedlist_file_paths,
                                                                    file_filters=(("Seed List File", "*.seed")))

        default_seed_file_paths = self.export_config.get_value('seedfile.paths')
        # TODO: Make sure the paths are project-relative
        self.seed_file_paths = self.add_labeled_multifile_entry(parent=project_asset_bundling_options_frame,
                                                                label_text="Seed (Asset) File Paths",
                                                                default_value=default_seed_file_paths,
                                                                file_filters=(("All Files", "*.*")))

        default_level_names = self.export_config.get_value('default.level.names')
        self.level_names = self.add_labeled_multittext_entry(parent=project_asset_bundling_options_frame,
                                                             label_text="Level Names",
                                                             default_value=default_level_names)

        default_asset_bundling_path = self.export_config.get_value("asset.bundling.path")
        self.asset_bundling_path = self.add_labeled_text_entry(parent=project_asset_bundling_options_frame,
                                                               label_text="Asset Bundling Path",
                                                               default_value=default_asset_bundling_path,
                                                               entry_colspan=3)






if __name__ == '__main__':

    project_export_ui = MainWindow()

    project_export_ui.mainloop()





