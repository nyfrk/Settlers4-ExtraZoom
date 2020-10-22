# The Settlers 4: ExtraZoom Plugin

By default, you cannot zoom out very much thus it is unnecessary difficult to obtain a good overview of large settlements. This script enables you to customize the zoom limits.

This is a plugin for the game the settlers 4. 

There is a [German translation for this README](README_DE.md). Please note that it may be outdated.

![extrazoom](extrazoom.gif)



## Features

* Extend the zoom limit to allow for a much larger view of the map.
* You can customize the limit by using a configuration file.
* Compatibility: Works with the Gold Edition and the History Edition of The Settlers 4.
* Multiplayer interoperability: You can play multiplayer with participants that do not use this mod. No DESYNCS!
* Additional bug fixes:
  * Fixed a potential crash (caused by a non-terminating algorithm) related to zooming/scrolling.
  * Moving the camera at any zoom level will now allow you to move it to every possible position on the map.



## How to use

You need an ASI Loader to use this mod. I recommend [The Settlers 4: ASI Loader](https://github.com/nyfrk/Settlers4-ASI-Loader) as it works nicely with the Gold and History Edition of The Settlers 4 and does not require any configuration. 

1. [Download](https://github.com/nyfrk/Settlers4-ExtraZoom/releases) a release.
2. Unpack the files into your `plugins` directory.
3. Start the game. The mod will load automatically.

To uninstall the mod remove `ExtraZoom.asi` from your`plugins` directory. 

#### Customize

Open the ExtraZoom.ini to change the limit to any value you desire. Please note that the more you allow to zoom out, the more powerful your computer must be. Especially since there can be thousands of objects onto the screen when zooming very far out. This will challenge even modern computers, since the game lacks proper hardware support. When customizing, you can get a very far zoom levels like this:

![level6](level6.png)

(a map with the size of 1024x1024, viewed with the unlocked zoom level of 6)



## Known Problems

* If you zoom using the right mouse + left mouse combination, you may get a more sloppy zooming experience.
  I do not recommend to use this plugin if you use that method for zooming. I think nobody uses that zooming method anyway so I did not bother fixing it. If you happen to use this method, feel free to [open a ticket](https://github.com/nyfrk/Settlers4-ExtraZoom/issues) and I will work on a solution.



## Issues and Questions

The project uses the Github Issue tracker. Please open a ticket [here](https://github.com/nyfrk/Settlers4-ExtraZoom/issues). 



## Contribute

The official repository of this project is available at https://github.com/nyfrk/Settlers4-ExtraZoom. 

#### Compile it yourself

Download Visual Studio 2017 or 2019 with the C++ toolchain. The project is configured to build it with the Windows XP compatible **v141_xp** toolchain. However, you should be able to change the toolchain to whatever you like. No additional libraries are required so it should compile out of the box.

#### How does this plugin work?

This plugin fixes or circumvents several bugs that have been introduced by Blubyte.

Probably a missing 0x prefix at the zoom step multiplier may cause the GetClosestPointOutsideMap algorithm to not terminate (crash) when playing with a zoomed out view. This is caused by the faulty algorithm that breaks when the least significant word of the zoom level is non-zero. Instead of fixing the GetClosestPointOutsideMap algorithm, this plugin fixes the problem by enforcing that the least significant word of the zoom level is always 0. This will ensure multiplayer compatibility since for everything else the faulty algorithm is used and thus does not cause desyncs.

* Very close zoom levels will not cause the game to render animations sloppy anymore. 

* Some zoom levels prevented the player from positioning the camera to map positions close to the map border (especially the south border). This has been fixed.
* This fix is not needed for the map editor since Blubyte fixed the zoom level to carefully selected constants here.



## License

The project is licensed under the [MIT](LICENSE.md) License. 
