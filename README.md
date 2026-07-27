
# JA2 v1.13

<br />
<br />
<br />

<p align="center">
  <img src="ja2v1.13.png" alt="JA2 v1.13">
</p>

<br />
<br />



### Preamble

Jagged Alliance 2 v1.13 is a modification for the Jagged Alliance 2 game.

Original development was done through SVN, this however ended abruptly in 2022, to keep the development going the step to Github has been made.

Feel free to participate in the development!
  
  
For more information you can visit the following locations:
- [The Bear's Pit Forum](https://thepit.ja-galaxy-forum.com) 
- [Jagged Alliance 2 v1.13 - Starter Documentation](https://github.com/1dot13/documentation)
- [How to get: latest 1.13, 7609, feature-descriptions and more](http://thepit.ja-galaxy-forum.com/index.php?t=msg&th=24648&start=0&)
- [JA2 v1.13 pbworks wiki (outdated)](http://ja2v113.pbworks.com/w/page/4218339/FrontPage)
- [The Bear's Pit Discord](https://discord.gg/GqrVZUM)


In case of any issues, look at [Reports](#Reports) or [Participation](#Participation)  

### Downloads

> **Note**
> All-in-one releases come for different languages and include
> JA2 v1.13, the Map Editor and JA2 Unfinished Business.

Visit the [releases page](https://github.com/1dot13/source/releases) to download the latest all-in-one.


### Installation

1. Install the original Jagged Alliance 2
2. Download the latest all-in-one release and copy its content to JA2 game directory. Overwrite when asked.
3. Modify ini settings if you like.
4. Play the game.

   Some additional information on can be found in folder "docs" inside download.  
     
   If you face issues with higher resolutions, alt+tab not working, blackscreen, etc.,  
   run the "cnc-ddraw-config.exe" in game-folder and adjust settings to your liking.  
   (those issues can occur due to the combination of old game and modern OS/hardware, cnc-ddraw helps to avoid those) 


### Visual Studio setup

1. Run `Visual Studio 2019` or newer.
2. Clone and open the location with the source code using one of these two options:
    * Click `Clone a repository`
        * Enter `git@github.com:1dot13/source.git` or `https://github.com/1dot13/source.git` in the Repository location field, select the path you want to clone the repository to and click `Clone`.
        * Double-click on `Folder View` in the `Solution Explorer`
    * Click `Open a local folder`
        * Use this option if you already cloned the repository yourself.
3. Visual Studio will automatically detect `CMakePresets.json` and run the CMake generation. Pick `1dot13 Debug`, `1dot13 Release` or `1dot13 RelWithDebInfo` in the configuration dropdown.
4. So the built executables land in your JA2 1.13 installation and can be debugged there, set the environment variable `JA2_GAMEDIR` to its path, e.g. `setx JA2_GAMEDIR C:/Games/JA2` in a terminal, then restart Visual Studio. Note that the path needs a working 1.13 installation, and that includes the 1.13 game data.
5. To change anything else — which applications to build, the compiler, an output directory per configuration — create a `CMakeUserPresets.json` next to `CMakePresets.json`. It is ignored by git and its presets can inherit from the `1dot13` ones:

    ```json
    {
      "version": 3,
      "configurePresets": [
        {
          "name": "my Debug",
          "inherits": "1dot13 Debug",
          "cacheVariables": {
            "Applications": "JA2;JA2MAPEDITOR",
            "CMAKE_RUNTIME_OUTPUT_DIRECTORY": "C:/Games/JA2"
          }
        }
      ]
    }
    ```

6. You can use `Build -> Build All` to build the executables you selected in the configuration.

> If you cloned before this was checked in, you have your own untracked `CMakePresets.json` in the source directory and git will refuse to update it. Rename it to `CMakeUserPresets.json` (renaming its presets so the names do not clash) or just delete it.


### Reports

For more information and reports, visit [Bug reports at Bear's Pit Forum](http://thepit.ja-galaxy-forum.com/index.php?t=thread&frm_id=216&) or join the [Bear's Pit Discord](https://discord.gg/GqrVZUM "Bear's Pit Discord")


### Participation 

Feel free to participate on GitHub. If you want to know how, or simply wanna share your thoughts on a topic join the [Bear's Pit Discord](https://discord.gg/GqrVZUM "Bear's Pit Discord")


