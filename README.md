# agnostep-theme

GNUstep theme for the [AGNoStep Desktop Project](https://github.com/pcardona34/agnostep).

Although this theme was created with the AGNoStep Project in mind, it could be also used with any other GNUstep system already installed.
 
This theme was created mostly from the [Papirus Iconset](https://github.com/PapirusDevelopmentTeam/papirus-icon-theme.git).
New icons were created for some applications: Gorm, projectCenter...

The UI theme was inspired by the Sleek flat theme of GNUstep.

The color palette was inspired from the classic OPENSTEP background.

The default wallpapers were created on purpose:
- Cubes for the Conky flavour;
- Waves for the Classic flavour.

Of course, you may change it with a picture of your own using the Preferences of GWorkspace.

## Goodies

The theme is also providing some usefull desktop tools:
- Time, date, Wheather health, CPU, Network and Memory monitoring.
- Birthday Notification.
- Updater: checking wether new Debian packages are available...

## How to install AGNOSTEP theme

### Within AGNoStep Desktop

If you yet installed the whole AGNoStep Desktop, this theme has been already set.

## Installing without AGNoStep Desktop

If not, you might first set a compliant GNUstep Desktop.

**IMPORTANT NOTE**
As the dependencies are managed with the apt command, you must use a compliant Operating System like GNU/Linux Debian Trixie. (If not, you must adapt all the dependencies. Do not hesitate to create a PR if you did it).

1) Install the Window manager Window Maker;

2) Install a complete GNUstep system with the frameworks (see  [agnostep](https://github.com/pcardona34/agnostep) for a complete list).

3) Install the Workspace (GWorkspace) and the mostly used GNUstep applications: namely AddressManager, GNUMail, SimpleAgenda, TextEdit, Terminal, InnerSpace...  
For the Classic Flavour (C5C) you will need also: AClock, TimeMon.

4) Clone this repository:
````cd; mkdir SOURCES; cd SOURCES
    clone https://github.com/pcardona34/agnostep-theme
````
5) Enter in the subfolder `install` to run the installation of the theme.
````
    cd agnostep-theme
    cd install
    ./install_theme.sh
````
You will be asked for the Flavour:
- Conky: it is a theme with a Conky panel.
- Classic (C5C): it is a more classic organisation of the Workspace with a Deamon top bar (known as 'Fiend' within OpenStep) named 'Clip' and the WMDock within Window Maker. 

You will also be asked for the Weather Station next to you to set the Weather info.

That's done. Enjoy!

## Screenshots

![AGNOSTEP Classic French](./Screenshots/screenshot_2025-12-21_Agnostep_Classic.png)

Some other screenshots are provided in the folder [Screenshots](Screenshots).

## How to uninstall

````
    cd agnostep-theme
    cd install
    ./uninstall_theme.sh
````

You will revert to the Default GNUstep Theme.