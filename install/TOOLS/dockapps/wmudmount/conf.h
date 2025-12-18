#ifndef CONF_H
#define CONF_H

#include "wmudisks.h"

struct user_command {
    const char *title;
    const char **argv;
    const char *filename;
    GtkMenuItem *menu_item;
    int argc;
    int line;

    int need_mountpoint:1;
    int need_dev:1;
};

extern struct user_command *menu_commands;
extern int sglclick_command_idx;
extern int dblclick_command_idx;

#define VOLUME_IN_EX_TYPE_DEV        1
#define VOLUME_IN_EX_TYPE_LABEL      2
#define VOLUME_IN_EX_TYPE_UUID       3
#define VOLUME_IN_EX_LOCATION_PAGER  1
#define VOLUME_IN_EX_LOCATION_FSM    2
#define VOLUME_IN_EX_LOCATION_ALL    3

struct volume_inex {
    int type;
    int location;
    const char *id;
    int exclude;
};
extern struct volume_inex *volume_inex;

void load_conf(int *argc, char **argv);

#define user_command_enabled(v,u) ( \
         (!(u)->need_dev || (v)->dev) && \
         (!(u)->need_mountpoint || (v)->mountpoint) \
        )


int exec_command(wmudisks_volume *v, struct user_command *cmd);

#endif
