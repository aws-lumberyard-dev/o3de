#!/usr/bin/env python3

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import pathlib
import re
from typing import (Dict, List)
from xml.dom import minidom
from xml.etree import ElementTree

CMAKE_FILE_NAME: str = "CMakeLists.txt"
HEADER_FILE_PATH: pathlib.Path
RELATIVE_INCLUDE_FILE_PATH: pathlib.Path
OUTPUT_FOLDER_PATH: pathlib.Path
OUTPUT_FILE_NAME: str = ""

RAW_HEADER_FILE_CONTENT: List[str] = []
FUNCTION_SIGNATURE_PATTERN: str = "([a-zA-Z0-9_:]*)([\n\r\s]+)([a-zA-Z0-9_:]*)\((.*)\)"
PARSED_HEADER_FILE_CONTENT: Dict[str, List[str]] = {}

def collect_arguments_info() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--header-file',
        required=True
    )
    
    args = parser.parse_args()
    header_file_processed: List[str] = args.header_file.split('.')
    if not header_file_processed or not header_file_processed[-1] == 'h':
        raise Exception(f"Expecting cpp header file, but get: {args.header_file}")
    
    global HEADER_FILE_PATH, OUTPUT_FOLDER_PATH, OUTPUT_FILE_NAME
    HEADER_FILE_PATH = pathlib.Path(args.header_file)
    if not HEADER_FILE_PATH.is_file():
        raise Exception(f"Given file doesn't exist or not a file: {HEADER_FILE_PATH}")
    get_include_file_path_based_on_first_cmake()
    
    OUTPUT_FOLDER_PATH = HEADER_FILE_PATH.parent
    
    OUTPUT_FILE_NAME = f"{HEADER_FILE_PATH.name.split('.')[0]}.ScriptCanvasFunction.xml"
    
def get_include_file_path_based_on_first_cmake() -> None:
    global RELATIVE_INCLUDE_FILE_PATH
    RELATIVE_INCLUDE_FILE_PATH = HEADER_FILE_PATH
    parent_folder: pathlib.Path = HEADER_FILE_PATH.parent
    while parent_folder.is_dir():
        tmp: pathlib.Path = parent_folder.joinpath(CMAKE_FILE_NAME)
        if tmp.is_file():
            RELATIVE_INCLUDE_FILE_PATH = HEADER_FILE_PATH.relative_to(parent_folder)
            return
        if parent_folder.parent == parent_folder:
            return
        parent_folder = parent_folder.parent

def read_raw_header_file() -> None:
    with open(HEADER_FILE_PATH.resolve(), 'r') as file:
        global RAW_HEADER_FILE_CONTENT
        RAW_HEADER_FILE_CONTENT = file.readlines()

def format_namespace(namespace: List[str]) -> str:
    return '::'.join(namespace)

def format_function_name(raw_function: str) -> str:
    function_name: str = raw_function.split(' ')[1]
    left_parenthesis: int = function_name.find('(')
    if left_parenthesis > 0:
        return function_name[:left_parenthesis].strip()
    else:
        return function_name.strip()

def process_raw_header_file() -> None:
    content = []
    block_count = 0
    namespace_stack = []
    current_block = []
    
    for index, line in enumerate(RAW_HEADER_FILE_CONTENT):
        clean_line: str = line.strip()
        if clean_line and not clean_line.startswith(('#', '//', '/*', '*', '*/')):
            if clean_line.startswith('namespace'):
                if len(namespace_stack) - block_count > 1:
                    namespace_stack = namespace_stack[:-1]
                namespace_stack.append(clean_line.split(' ')[1])
                
            if '{' in clean_line:
                block_count += 1
                
            if '}' in clean_line:
                block_count -= 1
                
            if re.match(FUNCTION_SIGNATURE_PATTERN, clean_line):
                current_block.append(format_function_name(clean_line))
            
            if len(namespace_stack) > block_count and current_block:
                global PARSED_HEADER_FILE_CONTENT
                PARSED_HEADER_FILE_CONTENT[format_namespace(namespace_stack)] = current_block
                current_block = []
                namespace_stack = namespace_stack[:-1]
               
def create_xml_file() -> None:
    # create the file structure
    scriptcanvas_root: ElementTree.Element = ElementTree.Element('ScriptCanvas')
    for key, value in PARSED_HEADER_FILE_CONTENT.items():
        generate_scriptcanvas_library_element(scriptcanvas_root, key, value)
    
    # create XML file with the results
    with open(OUTPUT_FOLDER_PATH.joinpath(OUTPUT_FILE_NAME).resolve(), "wb") as file:
        file.write(prettify_xml_content(scriptcanvas_root))

def generate_scriptcanvas_library_element(root: ElementTree.Element, namespace: str, functions: List[str]) -> None:
    library: ElementTree.Element = ElementTree.SubElement(root, 'Library')
    if RELATIVE_INCLUDE_FILE_PATH.is_absolute():
        library.set('Include', f"Please update it to relative path {RELATIVE_INCLUDE_FILE_PATH.resolve()}")
    else:
        library.set('Include', str(RELATIVE_INCLUDE_FILE_PATH))
    processed_namespace: List[str] = namespace.split('::')
    if len(processed_namespace) == 1:
        library.set('Name', namespace)
    else:
        library.set('Name', processed_namespace[-1])
    library.set('Namespace', namespace)
    
    for function in functions:
        sub_element: ElementTree.Element = ElementTree.SubElement(library, 'Function')
        sub_element.set('Name', function)

def prettify_xml_content(root: ElementTree.Element) -> bytes:
    rough_string: str = ElementTree.tostring(root, 'utf-8')
    reparsed = minidom.parseString(rough_string)
    return reparsed.toprettyxml(indent="    ", encoding='utf-8')

if __name__ == '__main__':
    try:
        collect_arguments_info()
        
        read_raw_header_file()
        
        process_raw_header_file()
        
        create_xml_file()

        exit(0)
    except Exception as e:
        print(e)
        exit(1)