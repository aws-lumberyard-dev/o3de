# Kitbash Converter

Kitbash Converter is a tool for converting model assets downloaded from [Kitbash3d.com](https://kitbash3d.com/) into separated fbx files, complete with O3DE Standard PBR Material definitions. This makes it extremely easy to convert files for use immediately in the Engine.


## Usage

Using the Kitbash Conversion scripts is very simple. You need access to the two separate elements that come with each Kitbash3d Model set- the FBX file that contains the model geometry, and the complete set of textures that come with it. Follow these steps for a successful conversion:

1. Set the input directory/file for processing. If the "Directory" radio button is selected, each FBX contained on the specified directory will be processed. "Single File" will modify the UI and file selection dialog to filter FBX files instead of directories.
2. The relative path for the output directory is located to the right of the radio buttons- it is suggested that you keep this format, but you can make modifications to this for processing assets other than the "CityKit" assets being dealt with currently. Please note that the relative path designated here is located relative to the "Input Directory" or if running single file mode, to the parent directory of the chosen FBX file. The script will only run the conversion if the output is a subdirectory of this Input Directory location. If you want to make modifications to the relative output path, simply click on it and a dialog will pop up for assignment.
3. Once the paths are set, the script consistently checks if the paths exist, and if found for source and output the "Process Files" button becomes enabled.
4. By clicking the Process Files button, the script will begin to perform the conversion, and will display progress messages in the black information bar on the bottom of the window until the process is complete.


## Path Locations

The directory paths are currently set for the ideal location to run the script. The easiest way to run the script is to follow the locations of these paths. For instance, if a new Kitbash3d asset needed to be converted by the name of "RaceTrack", here are the locations best suited for the conversion:


Main directory containing the initial assets:
E:/LY/spectra_atom/dev/Gems/AtomContent/KB3D_RaceTrack


Contained in this directory are the gem.json file, the preview.png file, and a folder titled "Assets". Inside Assets would be the FBX file(s) to convert on the base level, with all of the accompanying textures containing in a directory called "KB3DTextures". If you follow this setup, and adjust the Input and relative output paths accordingly the conversion should run smoothly. So for this instance you would use the values below for this particular conversion:


Input directory:
E:/LY/spectra_atom/dev/Gems/AtomContent/KB3D_RaceTrack

Relative output path:
E:/LY/spectra_atom/dev/Gems/AtomContent/KB3D_RaceTrack/Objects/KB3D/RaceTrack


## Entry File

main.py


### File Under

Standalone, Maya