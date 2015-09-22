# Overview
A plugin for UE4 to provide smarter suggestions in the editing process of Blueprint graphs. 

# Setup 
Unfortunately to integrate this plugin properly into Unreal Engine 4 some modifications to the editor had to be made. These modifications can be found in the "UE4 Modifications" folder. To apply these modfications Unreal Engine has to be compiled from source with these files overwritten. 
1. Download 
2. Copy contents of "UE4 Modifcations" over a the original UE4 files (4.7.5 was used) (https://github.com/EpicGames/UnrealEngine)
3. Compile UE4
4. Copy folder "Plugins" to the root folder of your project.
5. Open your project with the newly compiled UE4. 
6. Select Window -> Plugins. Click on Installed and the plugin should appear in the Editor/Productivity category. Activate the plugin and restart the editor. 
7. The plugin should now be ready to use. 
