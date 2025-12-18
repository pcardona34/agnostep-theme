#ifndef NOTIFY_H
#define NOTIFY_H

#include "wmudisks.h"
#include <gio/gio.h>

#define NOTIFY_INFO  3
#define NOTIFY_WARN  2
#define NOTIFY_ERROR 1
#define NOTIFY_NONE  0

extern int notify_level;
extern gboolean notify_body_is_used;
extern gboolean notify_body_can_markup;

struct notify_actions {
    wmudisks_volume *volume;
    const char *mount_text;
    const char *eject_text;
};

void init_notify(void);

void notify(int level, const char *category, gboolean transient, GIcon *icon, struct notify_actions *actions, const char *summary, const char *body, ...);

#endif
