# AGNOSTEP-Theme

## How to install AGNOSTEP theme

### Within AGNoStep Desktop Project

If you yet installed the whole [AGNoStep Desktop](https://github.com/pcardona34/agnostep-desktop), this theme has been already set.

## Installing without AGNoStep Desktop

If not, you might first set a compliant GNUstep Desktop.

> [!IMPORTANT]
> As the dependencies of the undelying operating system are managed with the apt command, you must use a compliant Operating System like GNU/Linux Debian Trixie. (If not, you must adapt all the dependencies. Do not hesitate to create a PR if you did it).

1. Install the Window manager Window Maker;

1. Install a complete GNUstep system with the frameworks (see  [agnostep-desktop](https://github.com/pcardona34/agnostep-desktop) for a complete list).

1. Install the Workspace (GWorkspace) and the mostly used GNUstep applications, namely:
   - AddressManager,
   - GNUMail,
   - Ink,
   - InnerSpace,
   - SimpleAgenda,
   - Terminal...  

1. For the *Classic* Flavour (C5C) you will need also:
   - AClock,
   - batmon (laptop only)
   - TimeMon.

1. Clone this repository:

    ```
    cd; mkdir SOURCES; cd SOURCES
    clone https://github.com/pcardona34/agnostep-theme
    ```

1. Enter in the subfolder `install` to run the installation of the theme.

      ```
      cd agnostep-theme
      cd install
      ./install_theme.sh
      ```

  - You will be asked for the Flavour (see above).
    - Conky: it is the theme with the Conky panel.
    - Classic (C5C): it is the theme with dockapps.
    - Also, you will choose a menu style: *Next* (vertical menu block) or *Mac* (horizontal menu bar)

  - You will also be asked for the Weather Station next to you to set the Weather info.

That's done. Enjoy!

## How to uninstall

```
    cd agnostep-theme
    cd install
    ./uninstall_theme.sh
```
You will revert to the Default GNUstep Theme. If you were using another theme, change it with `SystemPreferences`: then select the Theme tab.
