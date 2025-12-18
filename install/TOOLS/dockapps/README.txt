With AGNoStep C5C, you will need some dockapps and some docked GNUstep apps to run within the Deamon (Clip) of WindowMaker at the top side:

Description from left to right:

- Default Clip icon

- Date and Weather heat are provided with 'wminfo' and the plugin 'meteo.wmi'
Command: 
	wminfo -p meteo.wmi -u 3600 -k -b#2f2f2f -f#c0c0c0
Localized: yes (auto)

- Clock
Command:
	/System/Tools/openapp AClock

- Uptime, Memory free are provided by 'wminfo' with the plugin 'memory.wmi
Command:
	wminfo -p memory.wmi -u 1 -k -b#2f2f2f -f#c0c0c0
Localized: to be done

- CPU monitoring is provided by Timon (an app ported from OPENSTEP to GNUstep)
	/System/Tools/openapp TimeMon

- Root FileSytem (/) Disk Storage is monitored by wmdiskmon
Command: you must adapt this to your context
	wmdiskmon -p /dev/mmcblk0p2 -s

- Network activity is monitored by wmnd
Command:
	wmnd

- Debian System Updater notification is provided by wmtext with the script updater4wmtext
Command:
	wmtext -i 3600 -b \"forest green\" -a \"New|Nouveaux\" crimson white /usr/local/bin/updater4wmtext
Localized: English, French

- Birthday/Feast Memo notification is provided by wmtext he script birthday4wmtext
Command:
	wmtext -i 3600 -b \"forest green\" -a Anniveraire|Birthday crimson white /usr/local/bin/birthday4wmtext
Localized: English, French

- Mounting/Unmouting removable media is provided by the interactive wmudmount dockapp
Command:
	wmudmount

INSTALL: see 'install_dockapps.sh' in the current folder.
