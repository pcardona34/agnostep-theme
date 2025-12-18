/* wmudmount
 * Copyright © 2010-2018  Brad Jorsch <anomie@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WMUDISKS_H
#define WMUDISKS_H

#include <gio/gio.h>

/* Structs */

typedef struct _wmudisks_volume {
    const char *path;
    const char *label;
    const char *old_label;
    const char *desc;
    const char *uuid;
    const char *dev;
    const char **alldev;
    GIcon *icon, *unemblemed_icon;
    int pending_actions;

    struct _wmudisks_volume *parent;

    const char *mountpoint;
    const char **allmountpoints;
    guint64 total_size, free_size;

    const char *crypto_device_type; // identifier, e.g. "LUKS"
    const char *crypto_device_uuid; // IdUuid only, unlike v->uuid

    int root_only:1;
    int readonly:1;
    int removable:1;

    int force_include;
    int force_exclude;
    int include:1;

    /* private */
    int can_unlock:1;
    int can_lock:1;
    int can_eject:1;
    int can_detach:1;
    int can_mount:1;
    int can_relabel:1;
    int is_block_dev:1;
    int removed:1;
    guint update_scheduled;
    int refct;
    struct _UDisksObject *obj;
} wmudisks_volume;

/* Variables */

extern const char *wmudisks_version;
extern GPtrArray *volumes;
extern gboolean fsm_blink;
extern gboolean blink_full_filesystems;

/* Functions */

#define volume_index(i) ((wmudisks_volume *)g_ptr_array_index(volumes, (i)))

typedef void (*wmudisks_callback)(wmudisks_volume *v, gpointer ret, gpointer data, gboolean failed, GError *err);

#define wmudisks_get_display_name(v) ((v)->label?(v)->label:(v)->old_label?(v)->old_label:(v)->dev)
#define wmudisks_display_name_should_right_justify(v) (!(v)->label && !(v)->old_label)

void wmudisks_ref(wmudisks_volume *v);
void wmudisks_unref(wmudisks_volume *v);

static inline gboolean wmudisks_can_unlock(wmudisks_volume *v){
    for(; v; v=v->parent){
        if(v->removed) return FALSE;
        if(v->can_unlock) return TRUE;
    }
    return FALSE;
}
static inline gboolean wmudisks_can_lock(wmudisks_volume *v){
    for(; v; v=v->parent){
        if(v->removed) return FALSE;
        if(v->mountpoint) return FALSE;
        if(v->can_lock) return TRUE;
    }
    return FALSE;
}
static inline gboolean wmudisks_can_eject(wmudisks_volume *v){
    for(; v; v=v->parent){
        if(v->removed) return FALSE;
        if(v->mountpoint) return FALSE;
        if(v->can_eject) return TRUE;
    }
    return FALSE;
}
static inline gboolean wmudisks_can_detach(wmudisks_volume *v){
    for(; v; v=v->parent){
        if(v->removed) return FALSE;
        if(v->mountpoint) return FALSE;
        if(v->can_detach) return TRUE;
    }
    return FALSE;
}
static inline gboolean wmudisks_can_mount(wmudisks_volume *v){
    if(v->removed) return FALSE;
    return v->can_mount?TRUE:FALSE;
}
static inline gboolean wmudisks_can_relabel(wmudisks_volume *v){
    if(v->removed) return FALSE;
    return v->can_relabel?TRUE:FALSE;
}

gboolean wmudisks_init();
gboolean wmudisks_mount(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_unmount(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_eject(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_detach(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_lock(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_unlock(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_forget_passphrase(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_relabel(wmudisks_volume *v, wmudisks_callback callback, gpointer data);
gboolean wmudisks_update_fs_usage(double *max_pct);

#endif
