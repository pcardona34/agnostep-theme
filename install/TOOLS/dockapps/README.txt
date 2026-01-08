With AGNoStep C5C, you will need some dockapps and some docked GNUstep apps to run within the Deamon (Clip) of WindowMaker at the top side:

Description from left to right:

- Default Clip icon

- Date and Weather heat are provided with 'wmtext' and the plugin 'meteo4wmtext'
Command: 
	wmtext -i 3600 -b '#8686bE -f 'white' /usr/local/bin/meteo4wmtext
Localized: yes (auto)

- Clock
Command:
	$GNUSTEP_SYSTEM_TOOLS/openapp AClock

- Uptime, Memory free are provided by 'wmtext' with the plugin 'memory4wmtext'
Command:
	wmtext -i 1 -b '#8686bE -f 'white' /usr/local/bin/memory4wmtext
Localized: done

- CPU monitoring is provided by Timon (an app ported from OPENSTEP to GNUstep)
	$GNUSTEP_SYSTEM_TOOLS/openapp TimeMon

- Root FileSytem (/) Disk Storage and removable diskq are monitored by wmudmount
Command:
	wmudmount

- Network activity is monitored by wmnd
Command:
	wmnd

- Debian System Updater notification is provided by wmtext with the script updater4wmtext
Command:
	wmtext -i 3600 -b '#8686be' -f 'white' -a \"New|Nouveaux\" crimson white /usr/local/bin/updater4wmtext
Localized: English, French

- Birthday/Feast Memo notification is provided by wmtext he script birthday4wmtext
Command:
	wmtext -i 3600 -b \"forest green\" -a Anniveraire|Birthday crimson white /usr/local/bin/birthday4wmtext
Localized: English, French

- Mounting/Unmouting removable media is provided by the interactive wmudmount dockapp
Command:
	wmudmount: see above.

INSTALL: see 'install_dockapps.sh' in the current folder.
