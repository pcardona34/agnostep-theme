# agnostep-theme

GNUstep theme for the [AGNoStep Desktop Project](https://github.com/pcardona34/agnostep).

Although this theme was created with the AGNoStep Project in mind, it could be also used with a GNUstep system already installed.
 
This theme was created mostly from the [Papirus Iconset](https://github.com/PapirusDevelopmentTeam/papirus-icon-theme.git).
New icons were created for some applications: Gorm, projectCenter...
The UI theme was inspired by the Sleek flat theme of GNUstep.
The color palette was inspired from the classic OPENSTEP background.
The default wallpapers were created on purpose. Of course, you may change it with a picture of your own using the Preferences of GWorkspace. 

## With AGNoStep Desktop

If you installed the whole AGNoStep Desktop, this theme will be already set. You do not need it.

## Without AGNoStep Desktop

If not, you might first set a compliant GNUstep Desktop.

**IMPORTANT NOTE**
As the dependencies are managed with the apt command, you must use a compliant Operating System like GNU/Linux Debian Trixie.
If not, you must adapt all the dependencies.

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
- Classic (C5C): it is a more classic organisation of the Workspace with a Deamon top bar (known as 'Fiend' within OpenStep) named 'Clip' within Window Maker. 

You will also be asked for the Weather Station next to you to set the Weather info.

That's done. Enjoy!

## Screenshots

Some screenshots are provided in the folder [Screenshots](Screenshots).
