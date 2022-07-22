1. I unpacked the KB3D kit .zip file to here (source folder):

	C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\.src\KB3D_HighTechStreets
	
2. I copied the KB3DTextures to here (assets folder):

	C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\Assets\KB3D_HighTechStreets\KB3DTextures
	
3. I started Maya:
	I set the project workspace, Maya > File > Set Project ...
		C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\

4. worked on the source scene to repath textures (from source to assets)
	Maya > Windows > General Editors > Fiule path editor ...
	
	I repathed all textures, with relative paths to "../assets/KB3D_HighTechStreets/KB3DTextures"

5. I worked on scene cleanup
    - I set preferences: Z-up scene, Meter units
	- I imported the native (.ma) from:
		"C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\.src\KB3D_HighTechStreets\KB3D_HiTechStreets-Native.ma"
	- I placed all objects within a group, I set that group nodes pivot to the origin
	- I rotated the parent group, so all models were Z-up
	- I adjusted the parent groups pivot, to drop it to the bottom of the objects
	- I moved the parent group up, so the adjusted pivot was at 0,0,0
	- I froze transforms on the parent group
	- then I unparented all objects, so they are no longer under the parent group node
	- I deleted the parent group node
	
5. I saved the file (as a binary .mb) to:
	"C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\.src\KB3D_HighTechStreets\KB3D_HighTechStreets.mb"
	
6. I cleaned up all the materials, I removed the _blinn from the material node names, then resaved the file

7. I exported an SOURCE FBX:
	"C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\.src\KB3D_HighTechStreets\KB3D_HighTechStreets.fbx"
	
	also an ascii version (so I could inspect the paths): KB3D_HighTechStreets_ascii.fbx

8. then ran Ben's tool

	I actually used the source .mb, and not the .fbx
	input file: "C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\.src\KB3D_HighTechStreets\KB3D_HighTechStreets.fbx"
	
	output directory: "C:\depot\o3de-multiplayersample-assets\Gems\kb3d_mps\"
	
This mostly worked but I am seeing some other issues
	
... to be continued