#ifndef DOCK_H
#define DOCK_H

#define BLINK_THRESHOLD 0.95

extern gint return_to_fsm_timeout;
extern gboolean fsm_skip_root_only_volumes;
extern gboolean pager_skip_root_only_volumes;
extern gboolean fsm_blink;
extern gboolean blink_full_filesystems;
extern gboolean nonwmaker;

#include "wmudisks.h"

void wmudisks_connection_changed(const char *wmudisks_version);
void volume_added(wmudisks_volume *v, gboolean connecting);
void volume_updated(wmudisks_volume *v);
void volume_removed(int idx, wmudisks_volume *v, gboolean disconnecting);

void create_dock_icon(void);
void redraw_dock(void);

void dock_action_callback(wmudisks_volume *v, gpointer ret, gpointer data, gboolean failed, GError *err);

void mount_button_click();
void eject_button_click();
void left_button_click();
void right_button_click();

struct notify_actions *dock_actions_for_volume(wmudisks_volume *v);
void dock_action_goto(wmudisks_volume *v);
void dock_action_mount(wmudisks_volume *v);
void dock_action_eject(wmudisks_volume *v);

#endif
