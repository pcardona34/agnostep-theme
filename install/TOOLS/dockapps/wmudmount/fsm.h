#ifndef FSM_H
#define FSM_H

#include <glib.h>

int get_fs_usage(const char *path, guint64 *blocks, guint64 *bfree);

#endif
