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

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <udisks/udisks.h>
#include <gio/gio.h>

#include "asklabel.h"
#include "askpass.h"
#include "conf.h"
#include "die.h"
#include "dock.h"
#include "fsm.h"
#include "misc.h"
#include "wmudisks.h"

/* Variables */

static UDisksClient *udclient = NULL;
const char *wmudisks_version = NULL;

GPtrArray *volumes;
static GPtrArray *all_volumes;

static gboolean in_connect = FALSE;

struct callback_data {
    wmudisks_volume *v;
    wmudisks_volume *req_v;
    wmudisks_callback callback;
    int flag;
    gpointer data;
};

/* Forward declarations */

static void udisks_async_cb(GObject *, GAsyncResult *, gpointer);
static void udisks_manager_changed_handler(UDisksClient *, GParamSpec *, char *);
static void udisks_object_added_handler(GDBusObjectManager *, GDBusObject *, gpointer);
static void udisks_object_removed_handler(GDBusObjectManager *, GDBusObject *, gpointer);
static void udisks_object_property_changed_handler(GObject *, GParamSpec *, wmudisks_volume *);
static gboolean deactivate_volume(wmudisks_volume *);
static void update_volume(wmudisks_volume *);
static void wmudisks_unlock_cb(UDisksEncrypted *enc, GAsyncResult *res, struct callback_data *d);

/* Volume list management */

/** Allocates a new volume.
 * You probably want to call add_volume instead.
 */
static wmudisks_volume *alloc_volume(UDisksObject *obj) {
    const gchar *path = g_dbus_object_get_object_path(G_DBUS_OBJECT(obj));
    wmudisks_volume *v = g_slice_new0(wmudisks_volume);
    warn(DEBUG_DEBUG, "Allocating new volume %p for %s", v, path);
    v->refct = 0;
    v->path = g_strdup(path);
    v->obj = UDISKS_OBJECT(g_object_ref(obj));

    v->label = NULL;
    v->old_label = NULL;
    v->desc = NULL;
    v->uuid = NULL;
    v->dev = NULL;
    v->alldev = NULL;
    v->icon = NULL;
    v->unemblemed_icon = NULL;
    v->parent = NULL;
    v->mountpoint = NULL;
    v->allmountpoints = NULL;

    g_signal_connect(G_OBJECT(obj), "notify", G_CALLBACK(udisks_object_property_changed_handler), v);
    GObject *g;
    if ((g= G_OBJECT(udisks_object_peek_block(obj)))) {
        g_signal_connect(g, "notify", G_CALLBACK(udisks_object_property_changed_handler), v);
    }
    if ((g = G_OBJECT(udisks_object_peek_filesystem(obj)))) {
        g_signal_connect(g, "notify", G_CALLBACK(udisks_object_property_changed_handler), v);
    }
    if ((g = G_OBJECT(udisks_object_peek_encrypted(obj)))) {
        g_signal_connect(g, "notify", G_CALLBACK(udisks_object_property_changed_handler), v);
    }

    update_volume(v);

    return v;
}

/** Clears out the mutable data from a volume.
 */
static void free_volume_data(wmudisks_volume *v) {
    g_clear_pointer(&v->parent, wmudisks_unref);
    g_clear_pointer((gpointer*)&v->label, g_free);
    g_clear_pointer((gpointer*)&v->desc, g_free);
    g_clear_pointer((gpointer*)&v->uuid, g_free);
    g_clear_pointer((gpointer*)&v->dev, g_free);
    g_clear_pointer((gpointer*)&v->alldev, g_strfreev);
    g_clear_pointer(&v->icon, g_object_unref);
    g_clear_pointer(&v->unemblemed_icon, g_object_unref);
    g_clear_pointer((gpointer*)&v->mountpoint, g_free);
    g_clear_pointer((gpointer*)&v->allmountpoints, g_strfreev);
    g_clear_pointer((gpointer*)&v->crypto_device_uuid, g_free);
}

/** Deallocates a volume.
 * You probably want to call remove_volume instead.
 */
static void dealloc_volume(wmudisks_volume *v) {
    warn(DEBUG_DEBUG, "Deallocating volume %p for %s", v, v->path);

    g_signal_handlers_disconnect_by_func(v->obj, G_CALLBACK(udisks_object_property_changed_handler), v);
    GObject *g;
    if ((g = G_OBJECT(udisks_object_peek_block(v->obj)))) {
        g_signal_handlers_disconnect_by_func(g, G_CALLBACK(udisks_object_property_changed_handler), v);
    }
    if ((g = G_OBJECT(udisks_object_peek_filesystem(v->obj)))) {
        g_signal_handlers_disconnect_by_func(g, G_CALLBACK(udisks_object_property_changed_handler), v);
    }
    if ((g = G_OBJECT(udisks_object_peek_encrypted(v->obj)))) {
        g_signal_handlers_disconnect_by_func(g, G_CALLBACK(udisks_object_property_changed_handler), v);
    }

    if (v->update_scheduled) {
        g_source_remove(v->update_scheduled);
        v->update_scheduled = 0;
    }

    free_volume_data(v);
    g_clear_pointer((gpointer*)&v->path, g_free);
    g_clear_pointer((gpointer*)&v->old_label, g_free);
    g_clear_pointer(&v->obj, g_object_unref);
    g_slice_free(wmudisks_volume, v);
}


/** Allocates a volume and adds it to the all_volumes list.
 */
static wmudisks_volume *add_volume(UDisksObject *obj) {
    wmudisks_volume *v = alloc_volume(obj);
    warn(DEBUG_DEBUG, "Adding volume %s (%p)", v->path, v);
    wmudisks_ref(v);
    g_ptr_array_add(all_volumes, v);
    return v;
}

/** Removed a volume from the all_volumes list.
 * This will free it if there are no other references.
 */
static void remove_volume(wmudisks_volume *v) {
    deactivate_volume(v);
    warn(DEBUG_DEBUG, "Removing volume %s (%p)", v->path, v);
    v->removed = 1;
    for (guint i = all_volumes->len; i-- > 0; ) {
        wmudisks_volume *v2 = g_ptr_array_index(all_volumes, i);
        if (v2->parent == v) {
            if (!in_connect) {
                warn(DEBUG_WARN, "Device %s (%p) being removed before child device %s", v->path, v, v2->path);
            }
            remove_volume(v2);
            i = all_volumes->len; // Restart scan since array might have changed
        }
    }
    if (g_ptr_array_remove(all_volumes, v)) {
        wmudisks_unref(v);
    }
}

/** Find the volume for a UDisksObject
 */
static wmudisks_volume *find_volume(UDisksObject *obj) {
    const gchar *path = g_dbus_object_get_object_path(G_DBUS_OBJECT(obj));
    for (guint i = all_volumes->len; i-- > 0; ) {
        wmudisks_volume *v = g_ptr_array_index(all_volumes, i);
        if (!strcmp(path, v->path)) {
            return v;
        }
    }
    return NULL;
}

/** Add a volume to the active list
 */
static gboolean activate_volume(wmudisks_volume *v){
    for (guint i = volumes->len; i-- > 0; ) {
        if (g_ptr_array_index(volumes, i) == v) {
            return FALSE;
        }
    }

    warn(DEBUG_DEBUG, "Activating volume %s (%p)", v->path, v);
    g_ptr_array_add(volumes, v);
    wmudisks_ref(v);
    volume_added(v, in_connect);
    return TRUE;
}

/** Remove a volume from the active list
 */
static gboolean deactivate_volume(wmudisks_volume *v){
    for (guint i = volumes->len; i-- > 0; ) {
        if (g_ptr_array_index(volumes, i) == v) {
            warn(DEBUG_DEBUG, "Deactivating volume %s (%p) at index %d", v->path, v, i);
            g_ptr_array_remove_index(volumes, i);
            volume_removed(i, v, in_connect);
            wmudisks_unref(v);
            return TRUE;
        }
    }
    return FALSE;
}


/** Schedule a call to update_volume.
 * Useful if you expect a bunch of changes to all come in at once.
 */
static gboolean schedule_update_volume_cb(wmudisks_volume *v) {
    update_volume(v);
    return FALSE;
}
static void schedule_update_volume(wmudisks_volume *v) {
    if (!v->update_scheduled) {
        warn(DEBUG_DEBUG, "Scheduling update for volume %s (%p)", v->path, v);
        v->update_scheduled = g_idle_add_full(G_PRIORITY_HIGH_IDLE,
            (GSourceFunc)schedule_update_volume_cb,
            v, NULL
        );
    }
}

/** Wait for an update to the volume
 * When you expect a change and want to wait for it
 */
static void wait_for_update(wmudisks_volume *v) {
    wmudisks_ref(v);
    while (!v->update_scheduled && !v->removed) {
        gtk_main_iteration();
    }
    if (v->update_scheduled) {
        update_volume(v);
    }
    wmudisks_unref(v);
}

/** Get the object for an object-property
 */
static UDisksObject *object_for_path(const gchar *path) {
    if (!path || !strcmp(path, "/")) {
        return NULL;
    }

    return udisks_client_peek_object(udclient, path);
}

static char *dup_if_nonempty(const char *s) {
    return (s && *s) ? g_strdup(s) : NULL;
}

/** Update the volume information from the UDisksObject
 */
static void update_volume(wmudisks_volume *v) {
    if (v->update_scheduled) {
        g_source_remove(v->update_scheduled);
        v->update_scheduled = 0;
    }

    warn(DEBUG_DEBUG, " ... Updating volume %s (%p)", v->path, v);
    if (v->removed) warn(DEBUG_DEBUG, "      !!! Volume was removed!");

    // Clear old data
    free_volume_data(v);
    v->crypto_device_type = NULL;
    v->free_size = 0;
    v->total_size = 0;
    v->root_only=0;
    v->readonly=0;
    v->removable=0;
    v->can_unlock=0;
    v->can_lock=0;
    v->can_eject=0;
    v->can_detach=0;
    v->can_mount=0;
    v->can_relabel=0;

    UDisksObject *obj = v->obj;
    UDisksBlock *block = udisks_object_peek_block(obj);
    UDisksDrive *drive = udisks_object_peek_drive(obj);

    v->is_block_dev = !!block;

    // Find the parent device, if any
    UDisksObject *parent = NULL;
    if (block) {
        if (!parent) {
            parent = object_for_path(udisks_block_get_crypto_backing_device(block));
        }
        if (!parent) {
            parent = object_for_path(udisks_block_get_drive(block));
        }
    }
    if (parent) {
        wmudisks_volume *p = find_volume(parent);
        if (!p) {
            warn(DEBUG_DEBUG, "--- Adding parent volume");
            p = add_volume(parent);
            warn(DEBUG_DEBUG, "--- Done adding parent volume");
        }
        wmudisks_ref(p);
        v->parent = p;
        warn(DEBUG_DEBUG, "      ... Parent is %s", p->path);
    }

    // Load basic info
    UDisksObjectInfo *info = udisks_client_get_object_info(udclient, obj);

    v->desc = dup_if_nonempty(udisks_object_info_get_media_description(info));
    if (!v->desc) {
        v->desc = dup_if_nonempty(udisks_object_info_get_description(info));
    }
    warn(DEBUG_DEBUG, "      ... Description is %s", v->desc);

    GIcon *icon = NULL;
    const char *emblem = NULL;
    if (!icon) {
        icon = udisks_object_info_get_media_icon(info);
    }
    if (!icon) {
        icon = udisks_object_info_get_icon(info);
    }
    if (!icon) {
        icon = udisks_object_info_get_media_icon_symbolic(info);
    }
    if (!icon) {
        icon = udisks_object_info_get_icon_symbolic(info);
    }
    if (icon) {
        g_object_ref(icon);
    }
    warn(DEBUG_DEBUG, "      ... Icon is %s", icon ? g_icon_to_string( icon ) : "<none>");

    g_object_unref(info);

    // Information from the various sub-objects
    if (block) {
        v->dev = dup_if_nonempty(udisks_block_get_preferred_device(block));
        if (!v->dev) {
            v->dev = dup_if_nonempty(udisks_block_get_device(block));
        }
        warn(DEBUG_DEBUG, "      ... Preferred block device is %s", v->dev);

        const gchar * const *symlinks = udisks_block_get_symlinks(block);
        if (symlinks) {
            guint l = g_strv_length((gchar **)symlinks);
            v->alldev = g_new(const char *, l + 2);
            v->alldev[0] = udisks_block_dup_device(block);
            for (guint i = 0; i < l; i++) {
                v->alldev[i+1] = g_strdup(symlinks[i]);
            }
            v->alldev[l+1] = NULL;
        } else {
            v->alldev = g_new(const char *, 2);
            v->alldev[0] = udisks_block_dup_device(block);
            v->alldev[1] = NULL;
        }
        if (warn_level >= DEBUG_DEBUG) {
            for (const char **p = v->alldev; *p; p++) {
                warn(DEBUG_DEBUG, "      ... Block device is %s", *p);
            }
        }

        v->uuid = dup_if_nonempty(udisks_block_get_id_uuid(block));
        warn(DEBUG_DEBUG, "      ... Block UUID is %s", v->uuid);
        v->label = dup_if_nonempty(udisks_block_get_id_label(block));
        warn(DEBUG_DEBUG, "      ... Block Label is %s", v->label);
        v->root_only = udisks_block_get_hint_system(block);
        warn(DEBUG_DEBUG, "      ... Block device is %sroot only", v->root_only ? "" : "not ");
        v->readonly = udisks_block_get_read_only(block);
        warn(DEBUG_DEBUG, "      ... Block device is %sread only", v->readonly ? "" : "not ");

        v->can_lock = !!object_for_path(udisks_block_get_crypto_backing_device(block));
        if (v->can_lock) {
            emblem = "wmudmount-unlock";
        }
    } else if (drive) {
        UDisksBlock *dblock = udisks_client_get_block_for_drive(udclient, drive, FALSE);
        if (dblock) {
            v->dev = dup_if_nonempty(udisks_block_get_preferred_device(dblock));
            if (!v->dev) {
                v->dev = dup_if_nonempty(udisks_block_get_device(dblock));
            }
            warn(DEBUG_DEBUG, "      ... Preferred drive device is %s", v->dev);

            const gchar * const *symlinks = udisks_block_get_symlinks(dblock);
            if (symlinks) {
                guint l = g_strv_length((gchar **)symlinks);
                v->alldev = g_new(const char *, l + 2);
                v->alldev[0] = udisks_block_dup_device(dblock);
                for (guint i = 0; i < l; i++) {
                    v->alldev[i+1] = g_strdup(symlinks[i]);
                }
                v->alldev[l+1] = NULL;
            } else {
                v->alldev = g_new(const char *, 2);
                v->alldev[0] = udisks_block_dup_device(dblock);
                v->alldev[1] = NULL;
            }
            if (warn_level >= DEBUG_DEBUG) {
                for (const char **p = v->alldev; *p; p++) {
                    warn(DEBUG_DEBUG, "      ... Drive device is %s", *p);
                }
            }

            g_object_unref(dblock);
        }
    }

    UDisksPartition *partition = udisks_object_peek_partition(obj);
    if (partition) {
        if (!v->uuid) {
            v->uuid = dup_if_nonempty(udisks_partition_get_uuid(partition));
            warn(DEBUG_DEBUG, "      ... Partition UUID is %s", v->uuid);
        }
        if (!v->label) {
            v->label = dup_if_nonempty(udisks_partition_get_name(partition));
            warn(DEBUG_DEBUG, "      ... Partition name is %s", v->label);
        }
    }

    if (drive) {
        v->removable = udisks_drive_get_removable(drive);
        warn(DEBUG_DEBUG, "      ... Drive/media is %sremovable", v->removable ? "" : "not ");

        v->can_eject = udisks_drive_get_ejectable(drive) && udisks_drive_get_media_removable(drive);
        v->can_detach = udisks_drive_get_can_power_off(drive);
    }

    if (udisks_object_peek_encrypted(obj)) {
        emblem = "wmudmount-lock";

        v->crypto_device_uuid = udisks_block_dup_id_uuid(block);
        const gchar *type = udisks_block_get_id_type(block);
        if (!strncmp(type, "crypto_", 7)) {
            type += 7;
        }
        v->crypto_device_type = g_strdup(type);
        warn(DEBUG_DEBUG, "      ... Crypto type %s UUID %s", v->crypto_device_type, v->crypto_device_uuid);

        UDisksBlock *ctblock = udisks_client_get_cleartext_block(udclient, block);
        warn(DEBUG_DEBUG, "      ... Crypto device is %slocked", ctblock ? "un" : "");
        v->can_unlock = !ctblock;
        if (ctblock) {
            g_object_unref(ctblock);
        }
    }

    UDisksFilesystem *fs = udisks_object_peek_filesystem(obj);
    if (fs) {
        v->can_mount = 1;
        v->can_relabel = 1;

        v->allmountpoints = (const char **)udisks_filesystem_dup_mount_points(fs);
        if (v->allmountpoints && v->allmountpoints[0]) {
            v->mountpoint = g_strdup(v->allmountpoints[0]);
            if (warn_level >= DEBUG_DEBUG) {
                for (const char * const *p = v->allmountpoints; *p; p++) {
                    warn(DEBUG_DEBUG, "      ... Is mounted on %s", *p);
                }
            }
            emblem = "emblem-mounted";
            guint64 b, f;
            if (get_fs_usage(v->mountpoint, &b, &f) && b) {
                v->total_size = b;
                v->free_size = f;
                warn(DEBUG_DEBUG, "      ... Size %ld bytes, %ld free", b, f);
            }
        }
    }

    if (!icon) {
        for (wmudisks_volume *p = v->parent; p; p = p->parent) {
            if (p->unemblemed_icon) {
                icon = g_object_ref(p->unemblemed_icon);
                break;
            }
        }
        if (!icon) {
            icon = g_themed_icon_new_with_default_fallbacks(v->removable ? "drive-removable-media" : "drive-harddisk");
        }
    }
    if (icon) {
        v->unemblemed_icon = g_object_ref(icon);

        if (v->readonly) {
            GIcon *oldicon = icon;
            GIcon *ro_icon = g_themed_icon_new("emblem-readonly");
            GEmblem *emblem = g_emblem_new_with_origin(ro_icon, G_EMBLEM_ORIGIN_LIVEMETADATA);
            icon = g_emblemed_icon_new(oldicon, emblem);
            g_object_unref(emblem);
            g_object_unref(ro_icon);
            g_object_unref(oldicon);
        }
        if (emblem) {
            GIcon *oldicon = icon;
            GIcon *emblem_icon = g_themed_icon_new(emblem);
            GEmblem *emblem = g_emblem_new_with_origin(emblem_icon, G_EMBLEM_ORIGIN_DEVICE);
            icon = g_emblemed_icon_new(oldicon, emblem);
            g_object_unref(emblem);
            g_object_unref(emblem_icon);
            g_object_unref(oldicon);
        }
        v->icon = icon;
    }
    warn(DEBUG_DEBUG, "      ... Final icon is %s", icon ? g_icon_to_string(icon) : "<none>");

    if (v->can_lock) warn(DEBUG_DEBUG, "      !!! Can be locked");
    if (v->can_unlock) warn(DEBUG_DEBUG, "      !!! Can be unlocked");
    if (v->can_mount) warn(DEBUG_DEBUG, "      !!! Can be mounted");
    if (v->can_eject) warn(DEBUG_DEBUG, "      !!! Can be ejected");
    if (v->can_detach) warn(DEBUG_DEBUG, "      !!! Can be detached");

    v->force_include = 0;
    v->force_exclude = 0;
    if (volume_inex) {
        for (struct volume_inex *vie = volume_inex; vie->type; vie++) {
            const char *loc = "";
            switch (vie->location) {
              case VOLUME_IN_EX_LOCATION_PAGER:
                loc = "pager ";
                break;
              case VOLUME_IN_EX_LOCATION_FSM:
                loc = "FSM ";
                break;
            }
            switch (vie->type) {
              case VOLUME_IN_EX_TYPE_DEV:
                if (v->dev && !strcmp(vie->id, v->dev)) {
                    warn(DEBUG_DEBUG, "      ... Device file matches %s%s entry %s!", loc, vie->exclude?"exclude":"include", vie->id);
                    if (vie->exclude) {
                        v->force_include &= ~vie->location;
                        v->force_exclude |= vie->location;
                    } else {
                        v->force_include |= vie->location;
                        v->force_exclude &= ~vie->location;
                    }
                }
                if (v->mountpoint && !strcmp(vie->id, v->mountpoint)) {
                    warn(DEBUG_DEBUG, "      ... Device mountpoint matches %s%s entry %s!", loc, vie->exclude?"exclude":"include", vie->id);
                    if (vie->exclude) {
                        v->force_include &= ~vie->location;
                        v->force_exclude |= vie->location;
                    } else {
                        v->force_include |= vie->location;
                        v->force_exclude &= ~vie->location;
                    }
                }
                break;

              case VOLUME_IN_EX_TYPE_LABEL:
                if (v->label && !strcmp(vie->id, v->label)) {
                    warn(DEBUG_DEBUG, "      ... Device label matches %s%s entry LABEL=%s!", loc, vie->exclude?"exclude":"include", vie->id);
                    if (vie->exclude) {
                        v->force_include &= ~vie->location;
                        v->force_exclude |= vie->location;
                    } else {
                        v->force_include |= vie->location;
                        v->force_exclude &=~ vie->location;
                    }
                }
                break;

              case VOLUME_IN_EX_TYPE_UUID:
                if (v->uuid && !strcmp(vie->id, v->uuid)) {
                    warn(DEBUG_DEBUG, "      ... Device uuid matches %s%s entry UUID=%s!", loc, vie->exclude?"exclude":"include", vie->id);
                    if (vie->exclude) {
                        v->force_include &= ~vie->location;
                        v->force_exclude |= vie->location;
                    } else {
                        v->force_include |= vie->location;
                        v->force_exclude &= ~vie->location;
                    }
                }
                break;
            }
        }
    }


    // Now, activate or deactivate the device. A device is active if it's a
    // block device, it's mounted or has available actions, and doesn't have
    // active children.
    if (!v->is_block_dev) {
        deactivate_volume(v);
    } else {
        gboolean active = v->mountpoint || v->can_mount || v->can_lock || v->can_unlock || v->can_eject || v->can_detach;
        if ( active ) {
            for (guint i = volumes->len; i-- > 0; ) {
                wmudisks_volume *v2 = g_ptr_array_index(volumes, i);
                if (v2->parent == v) {
                    active = FALSE;
                    break;
                }
            }
        }

        gboolean changed = active ? activate_volume(v) : deactivate_volume(v);

        // If we changed, our parent device might need to be changed too.
        if (changed && v->parent) {
            schedule_update_volume(v->parent);
        }
    }

    volume_updated(v);
}


/* Other functions */

/** Try to connect to connect to the daemon, i.e. get a UDisksClient.
 */
static gboolean try_to_connect(gpointer d G_GNUC_UNUSED) {
    udisks_client_new(NULL, udisks_async_cb, NULL);
    return FALSE;
}

/** Try to unlock a device, either using the saved passphrase or by asking the
 * user for the passphrase.
 */
static gboolean wmudisks_unlock_internal(wmudisks_volume *v, wmudisks_callback callback, gpointer data, gboolean did_saved, GError **err){
    g_warn_if_fail(err == NULL || *err == NULL);

    struct callback_data *d = g_slice_new(struct callback_data);
    d->v = v;
    d->req_v = v;
    d->callback = callback;
    d->data = data;

    char *pass = NULL;
    if(!did_saved) {
        pass = load_pass_from_keyring(v);
    }
    if (pass) {
        warn(DEBUG_INFO, "Unlocking %s with saved passphrase", v->path);
        d->flag = 0;
    } else {
        warn(DEBUG_DEBUG, "Requesting passphrase for %s", v->path);
        pass = ask_for_pass(v, NULL, err);
        if (!pass) {
            g_slice_free(struct callback_data, d);
            return FALSE;
        }
        warn(DEBUG_INFO, "Unlocking %s with requested passphrase", v->path);
        d->flag = 1;
    }

    wmudisks_ref(v);
    v->pending_actions++;

    udisks_encrypted_call_unlock(
        udisks_object_peek_encrypted(v->obj),
        pass,
        g_variant_new("a{sv}", NULL),
        NULL,
        (GAsyncReadyCallback)wmudisks_unlock_cb,
        d
    );
    return TRUE;
}


/* Signal handlers */

/** Called when the UDisksManager changes
 * Generally this is on connection. If the manager exists and has a version
 * number, this will start listening for device change events (and will
 * simulate one to populate the list).
 */
static void udisks_manager_changed_handler(UDisksClient *client, GParamSpec *s G_GNUC_UNUSED, char *what) {
    if (client != udclient) {
        // WTF?
        g_signal_handlers_disconnect_by_func(G_OBJECT(client), G_CALLBACK(udisks_manager_changed_handler), NULL);
        return;
    }

    GDBusObjectManager *objman = udisks_client_get_object_manager(udclient);

    gboolean old_in_connect = in_connect;
    if (!wmudisks_version) {
        in_connect = TRUE;
    }

    UDisksManager *m = udisks_client_get_manager(udclient);
    if (!m) {
        warn(DEBUG_ERROR, "Connection to UDisks daemon %s", what);
        goto fail;
    }

    gchar *ver = udisks_manager_dup_version(m);
    if (!ver) {
        warn(DEBUG_ERROR, "Failed to fetch version from UDisks daemon");
        goto fail;
    }

    g_free((char *)wmudisks_version);
    wmudisks_version = ver;
    warn(DEBUG_INFO, "Connected to UDisks daemon version %s", ver);
    wmudisks_connection_changed(ver);

    g_signal_connect(G_OBJECT(objman), "object-added", G_CALLBACK(udisks_object_added_handler), NULL);
    g_signal_connect(G_OBJECT(objman), "object-removed", G_CALLBACK(udisks_object_removed_handler), NULL);

    // Simulate "added" messages for all objects currently existing
    GList *objects = g_dbus_object_manager_get_objects(objman);
    for (GList *cur = objects; cur; cur = cur->next) {
        udisks_object_added_handler(objman, cur->data, NULL);
    }
    g_list_free_full(objects, g_object_unref);

    goto out;

fail:
    in_connect = TRUE;
    g_signal_handlers_disconnect_by_func(G_OBJECT(objman), G_CALLBACK(udisks_object_added_handler), NULL);
    g_signal_handlers_disconnect_by_func(G_OBJECT(objman), G_CALLBACK(udisks_object_removed_handler), NULL);
    while (all_volumes->len > 0) {
        remove_volume(g_ptr_array_index(all_volumes, all_volumes->len - 1));
    }
    if(wmudisks_version){
        g_clear_pointer((gpointer*)&wmudisks_version, g_free);
        wmudisks_connection_changed(NULL);
    }

out:
    in_connect = old_in_connect;
}

/** Called when a UDiskObject is added
 * This will add it to the list if it doesn't already exist
 */
static void udisks_object_added_handler(GDBusObjectManager *objman G_GNUC_UNUSED, GDBusObject *dobj, gpointer d G_GNUC_UNUSED) {
    UDisksObject *obj = UDISKS_OBJECT(dobj);
    if (!obj) {
        return;
    }
    if (!udisks_object_peek_block(obj)) {
        // Only handle block devices here
        return;
    }

    if (!find_volume(obj)) {
        add_volume(obj);
    }
}

/** Called when a UDiskObject is removed
 * This will remove it from the list
 */
static void udisks_object_removed_handler(GDBusObjectManager *objman G_GNUC_UNUSED, GDBusObject *dobj, gpointer d G_GNUC_UNUSED) {
    UDisksObject *obj = UDISKS_OBJECT(dobj);
    if (!obj) {
        return;
    }

    wmudisks_volume *v;
    while ((v = find_volume(obj))) {
        remove_volume(v);
    }
}

/** Called when a property changes on a UDisksObject
 */
static void udisks_object_property_changed_handler(GObject *o G_GNUC_UNUSED, GParamSpec *s G_GNUC_UNUSED, wmudisks_volume *v) {
    // Make sure we care before doing a bunch of work.
    for (guint i = all_volumes->len; i-- > 0; ) {
        wmudisks_volume *v2 = g_ptr_array_index(all_volumes, i);
        if (v == v2) {
            schedule_update_volume(v);
            return;
        }
    }
}


/* Async callbacks */

/** Callback for udisks_client_new()
 * If the creation was successful, checks the manager to see if it's actually
 * connected.
 * If not, schedules a retry for 60 seconds.
 */
static void udisks_async_cb(GObject *o G_GNUC_UNUSED, GAsyncResult *res, gpointer d G_GNUC_UNUSED) {
    GError *err = NULL;

    udclient = udisks_client_new_finish(res, &err);
    if (!udclient) {
        warn(DEBUG_ERROR, "Failed to initialize connection to UDisks daemon: %s", err ? err->message : "<unknown error>");
        g_clear_error(&err);
        // Retry connection in a little bit
        g_timeout_add_seconds(60, try_to_connect, NULL);
        return;
    }
    g_clear_error(&err);

    g_signal_connect(G_OBJECT(udclient), "notify::manager", G_CALLBACK(udisks_manager_changed_handler), "lost");
    udisks_manager_changed_handler(udclient, NULL, "failed");
}

static void wmudisks_mount_cb(UDisksFilesystem *fs, GAsyncResult *res, struct callback_data *d) {
    GError *err = NULL;

    gchar *mount_path = NULL;
    gboolean ok = udisks_filesystem_call_mount_finish(fs, &mount_path, res, &err);
    if (err) {
        warn(DEBUG_INFO, "Mount %s: %s", ok ? "succeeded" : "failed", err->message);
    } else {
        warn(DEBUG_INFO, "Mount %s", ok ? "succeeded" : "failed");
    }

    if (ok) {
        g_clear_error(&err);
        g_set_error(&err, APP_NO_ERROR, 0, "%s mounted on %s", wmudisks_get_display_name(d->req_v), mount_path);

        warn(DEBUG_DEBUG, "Waiting for mount on %s", d->req_v->path);
        while (!d->req_v->mountpoint) {
            wait_for_update(d->req_v);
        }
        warn(DEBUG_DEBUG, "Got mount on %s", d->req_v->path);
    }

    d->req_v->pending_actions--;
    if (d->callback) {
        d->callback(d->req_v, NULL, d->data, !ok, err);
    }
    g_clear_pointer((gpointer*)&d->req_v->old_label, g_free);
    wmudisks_unref(d->req_v);
    g_slice_free(struct callback_data, d);
    g_clear_error(&err);
}

static void wmudisks_unmount_cb(UDisksFilesystem *fs, GAsyncResult *res, struct callback_data *d) {
    GError *err = NULL;

    gboolean ok = udisks_filesystem_call_unmount_finish(fs, res, &err);
    if (err) {
        warn(DEBUG_INFO, "Unmount %s: %s", ok ? "succeeded" : "failed", err->message);
    } else {
        warn(DEBUG_INFO, "Unmount %s", ok ? "succeeded" : "failed");
    }

    if (ok) {
        warn(DEBUG_DEBUG, "Waiting for unmount on %s", d->req_v->path);
        while (!d->req_v->removed && d->req_v->mountpoint) {
            wait_for_update(d->req_v);
        }
        warn(DEBUG_DEBUG, "Got mount on %s", d->req_v->path);
    }

    d->req_v->pending_actions--;
    if (d->callback) {
        d->callback(d->req_v, NULL, d->data, !ok, err);
    }
    g_clear_pointer((gpointer*)&d->req_v->old_label, g_free);
    wmudisks_unref(d->req_v);
    g_slice_free(struct callback_data, d);
    g_clear_error(&err);
}

static void wmudisks_detach_cb(UDisksDrive *drive, GAsyncResult *res, struct callback_data *d) {
    GError *err = NULL;

    gboolean ok = udisks_drive_call_power_off_finish(drive, res, &err);
    if (err) {
        warn(DEBUG_INFO, "Detach %s: %s", ok ? "succeeded" : "failed", err->message);
    } else {
        warn(DEBUG_INFO, "Detach %s", ok ? "succeeded" : "failed");
    }

    if (ok) {
        warn(DEBUG_DEBUG, "Waiting for detach on %s", d->req_v->path);
        while (wmudisks_can_detach(d->req_v)) {
            wait_for_update(d->req_v);
        }
        warn(DEBUG_DEBUG, "Got detach on %s", d->req_v->path);
    }

    d->v->pending_actions--;
    d->req_v->pending_actions--;
    if (d->callback) {
        d->callback(d->req_v, d->v, d->data, !ok, err);
    }
    g_clear_pointer((gpointer*)&d->v->old_label, g_free);
    wmudisks_unref(d->v);
    wmudisks_unref(d->req_v);
    g_slice_free(struct callback_data, d);
    g_clear_error(&err);
}

static void wmudisks_eject_cb(UDisksDrive *drive, GAsyncResult *res, struct callback_data *d) {
    GError *err = NULL;

    gboolean ok = udisks_drive_call_eject_finish(drive, res, &err);
    if (err) {
        warn(DEBUG_INFO, "Eject %s: %s", ok ? "succeeded" : "failed", err->message);
    } else {
        warn(DEBUG_INFO, "Eject %s", ok ? "succeeded" : "failed");
    }

    if (ok) {
        warn(DEBUG_DEBUG, "Waiting for eject on %s", d->req_v->path);
        while (wmudisks_can_eject(d->req_v)) {
            wait_for_update(d->req_v);
        }
        warn(DEBUG_DEBUG, "Got eject on %s", d->v->path);
    }

    d->v->pending_actions--;
    d->req_v->pending_actions--;
    if (d->callback) {
        d->callback(d->req_v, d->v, d->data, !ok, err);
    }
    g_clear_pointer((gpointer*)&d->v->old_label, g_free);
    wmudisks_unref(d->v);
    wmudisks_unref(d->req_v);
    g_slice_free(struct callback_data, d);
    g_clear_error(&err);
}

static void wmudisks_lock_cb(UDisksEncrypted *enc, GAsyncResult *res, struct callback_data *d) {
    GError *err = NULL;

    gboolean ok = udisks_encrypted_call_lock_finish(enc, res, &err);
    if (err) {
        warn(DEBUG_INFO, "Lock %s: %s", ok ? "succeeded" : "failed", err->message);
    } else {
        warn(DEBUG_INFO, "Lock %s", ok ? "succeeded" : "failed");
    }

    if (ok) {
        warn(DEBUG_DEBUG, "Waiting for lock on %s", d->v->path);
        while (wmudisks_can_lock(d->v)) {
            wait_for_update(d->v);
        }
        warn(DEBUG_DEBUG, "Got lock on %s", d->v->path);
    }

    d->v->pending_actions--;
    d->req_v->pending_actions--;
    if (d->callback) {
        d->callback(d->req_v, d->v, d->data, !ok, err);
    }
    g_clear_pointer((gpointer*)&d->v->old_label, g_free);
    wmudisks_unref(d->v);
    wmudisks_unref(d->req_v);
    g_slice_free(struct callback_data, d);
    g_clear_error(&err);
}

static void wmudisks_unlock_cb(UDisksEncrypted *enc, GAsyncResult *res, struct callback_data *d) {
    GError *err = NULL;

    wmudisks_volume *new_v = NULL;
    gchar *cleartext_path = NULL;
    gboolean ok = udisks_encrypted_call_unlock_finish(enc, &cleartext_path, res, &err);
    if (err) {
        warn(DEBUG_INFO, "Unlock %s: %s", ok ? "succeeded" : "failed", err->message);
    } else {
        warn(DEBUG_INFO, "Unlock %s", ok ? "succeeded" : "failed");
    }
    if (!ok) {
        if (!d->flag && wmudisks_unlock_internal(d->req_v, d->callback, d->data, TRUE, NULL)) {
            d->callback = NULL;
        }
        goto cleanup;
    }

    g_clear_error(&err);
    UDisksObject *cleartext_obj = object_for_path(cleartext_path);
    if (!cleartext_obj) {
        g_set_error(&err, APP_GENERIC_ERROR, 0, "Supposedly unlocked as %s, but that path is not found", cleartext_path);
        goto cleanup;
    }

    new_v = find_volume(cleartext_obj);
    if (!new_v) {
        new_v = add_volume(cleartext_obj);
    }
    g_set_error(&err, APP_NO_ERROR, 0, "%s unlocked as %s", wmudisks_get_display_name(d->req_v), wmudisks_get_display_name(new_v));

cleanup:
    d->req_v->pending_actions--;
    if (d->callback) {
        d->callback(d->req_v, new_v, d->data, !ok, err);
    }
    wmudisks_unref(d->req_v);
    g_slice_free(struct callback_data, d);
    g_clear_error(&err);
}

static void wmudisks_relabel_cb(UDisksFilesystem *fs, GAsyncResult *res, struct callback_data *d) {
    GError *err = NULL;

    gboolean ok = udisks_filesystem_call_set_label_finish(fs, res, &err);
    if (err) {
        warn(DEBUG_INFO, "Relabel %s: %s", ok ? "succeeded" : "failed", err->message);
    } else {
        warn(DEBUG_INFO, "Relabel %s", ok ? "succeeded" : "failed");
    }

    d->req_v->pending_actions--;
    if (d->callback) {
        d->callback(d->req_v, NULL, d->data, !ok, err);
    }
    g_clear_pointer((gpointer*)&d->req_v->old_label, g_free);
    wmudisks_unref(d->req_v);
    g_slice_free(struct callback_data, d);
    g_clear_error(&err);
}


/* Public functions */

gboolean wmudisks_update_fs_usage(double *max_pct) {
    gboolean update = FALSE;
    if (max_pct != NULL) {
        *max_pct = -1;
    }
    for (guint i = volumes->len; i-- > 0; ) {
        wmudisks_volume *v = g_ptr_array_index(volumes, i);
        if (v->mountpoint) {
            guint64 old_b = v->total_size;
            guint64 old_f = v->free_size;
            guint64 b, f;
            if (get_fs_usage(v->mountpoint, &b, &f) && b) {
                v->total_size = b;
                v->free_size = f;
                if (max_pct) {
                    double pct = (b - f) / (double)b;
                    if (pct >  *max_pct) {
                        *max_pct = pct;
                    }
                }
            } else {
                v->total_size = 0;
                v->free_size = 0;
            }
            if (b != old_b || f != old_f) {
                update = TRUE;
            }
        } else {
            v->total_size = 0;
            v->free_size = 0;
        }
    }

    return update;
}

void wmudisks_ref(wmudisks_volume *v) {
    g_warn_if_fail(v->refct >= 0);
    v->refct++;
}

void wmudisks_unref(wmudisks_volume *v) {
    g_warn_if_fail(v->refct > 0);
    if (--v->refct <= 0) {
        dealloc_volume(v);
    }
}

gboolean wmudisks_init() {
    volumes = g_ptr_array_new();
    all_volumes = g_ptr_array_new();

    try_to_connect(NULL);

    return TRUE;
}

gboolean wmudisks_mount(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    UDisksBlock *block = udisks_object_peek_block(v->obj);
    const gchar *type = udisks_block_get_id_type(block);
    if (!type || !*type) {
        type = "auto";
    }

    warn(DEBUG_INFO, "Mounting %s, fstype=%s", v->path, type);
    g_free((void *)v->old_label);
    v->old_label = g_strdup(v->label);

    struct callback_data *d = g_slice_new(struct callback_data);
    d->v = v;
    d->req_v = v;
    d->callback = callback;
    d->data = data;
    wmudisks_ref(v);
    v->pending_actions++;

    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&b, "{sv}", "fstype", g_variant_new_string(type));

    udisks_filesystem_call_mount( 
        udisks_object_peek_filesystem(v->obj),
        g_variant_builder_end(&b),
        NULL,
        (GAsyncReadyCallback)wmudisks_mount_cb,
        d
    );

    return TRUE;
}

gboolean wmudisks_unmount(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    warn(DEBUG_INFO, "Unmounting %s", v->path);
    g_free((void *)v->old_label);
    v->old_label = g_strdup(v->label);

    struct callback_data *d = g_slice_new(struct callback_data);
    d->v = v;
    d->req_v = v;
    d->callback = callback;
    d->data = data;
    wmudisks_ref(v);
    v->pending_actions++;

    udisks_filesystem_call_unmount( 
        udisks_object_peek_filesystem(v->obj),
        g_variant_new("a{sv}", NULL),
        NULL,
        (GAsyncReadyCallback)wmudisks_unmount_cb,
        d
    );
    return TRUE;
}

gboolean wmudisks_detach(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    wmudisks_volume *req_v = v;
    UDisksDrive *drive = NULL;
    while (TRUE) {
        drive = udisks_object_peek_drive(v->obj);
        if (drive && udisks_drive_get_can_power_off(drive)) {
            break;
        }
        v = v->parent;
        if (!v) {
            warn(DEBUG_WARN, "Cannot detach %s", req_v->path);
            return FALSE;
        }
    }

    warn(DEBUG_INFO, "Detaching %s", v->path);
    g_free((void *)req_v->old_label);
    req_v->old_label = g_strdup(req_v->label);

    struct callback_data *d = g_slice_new(struct callback_data);
    d->v = v;
    d->req_v = req_v;
    d->callback = callback;
    d->data = data;
    wmudisks_ref(v);
    wmudisks_ref(req_v);
    v->pending_actions++;
    req_v->pending_actions++;

    udisks_drive_call_power_off( 
        drive,
        g_variant_new("a{sv}", NULL),
        NULL,
        (GAsyncReadyCallback)wmudisks_detach_cb,
        d
    );
    return TRUE;
}

gboolean wmudisks_eject(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    wmudisks_volume *req_v = v;
    UDisksDrive *drive = NULL;
    while (TRUE) {
        drive = udisks_object_peek_drive(v->obj);
        if (drive && udisks_drive_get_ejectable(drive)) {
            break;
        }
        v = v->parent;
        if (!v) {
            warn(DEBUG_WARN, "Cannot eject %s", req_v->path);
            return FALSE;
        }
    }

    warn(DEBUG_INFO, "Ejecting %s", v->path);
    g_free((void *)req_v->old_label);
    req_v->old_label = g_strdup(req_v->label);

    struct callback_data *d = g_slice_new(struct callback_data);
    d->v = v;
    d->req_v = req_v;
    d->callback = callback;
    d->data = data;
    wmudisks_ref(v);
    wmudisks_ref(req_v);
    v->pending_actions++;
    req_v->pending_actions++;

    udisks_drive_call_eject( 
        drive,
        g_variant_new("a{sv}", NULL),
        NULL,
        (GAsyncReadyCallback)wmudisks_eject_cb,
        d
    );
    return TRUE;
}

gboolean wmudisks_lock(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    wmudisks_volume *req_v = v;
    UDisksEncrypted *enc = NULL;
    while (TRUE) {
        enc = udisks_object_peek_encrypted(v->obj);
        if (enc) {
            break;
        }
        v = v->parent;
        if (!v) {
            warn(DEBUG_WARN, "Cannot lock %s", req_v->path);
            return FALSE;
        }
    }

    warn(DEBUG_INFO, "Locking %s", v->path);
    g_free((void *)req_v->old_label);
    req_v->old_label = g_strdup(req_v->label);

    struct callback_data *d = g_slice_new(struct callback_data);
    d->v = v;
    d->req_v = req_v;
    d->callback = callback;
    d->data = data;
    wmudisks_ref(v);
    wmudisks_ref(req_v);
    v->pending_actions++;
    req_v->pending_actions++;

    udisks_encrypted_call_lock(
        enc,
        g_variant_new("a{sv}", NULL),
        NULL,
        (GAsyncReadyCallback)wmudisks_lock_cb,
        d
    );
    return TRUE;
}

gboolean wmudisks_unlock(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    if(!v->can_unlock){
        warn(DEBUG_WARN, "Cannot unlock %s", v->path);
        return FALSE;
    }

    gboolean ok = wmudisks_unlock_internal(v, callback, data, FALSE, NULL);
    return ok;
}

gboolean wmudisks_forget_passphrase(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    wmudisks_volume *req_v = v;
    for (; v; v = v->parent) {
        if (udisks_object_peek_encrypted(v->obj)) {
            warn(DEBUG_INFO, "Forgetting passphrase for %s (on %s)", req_v->path, v->path);
            forget_pass_from_keyring(v);
            if(callback) callback(req_v, v, data, !TRUE, NULL);
            return TRUE;
        }
    }
    warn(DEBUG_WARN, "Cannot find a crypto device for %s to forget passphrase", req_v->path);
    return FALSE;
}

gboolean wmudisks_relabel(wmudisks_volume *v, wmudisks_callback callback, gpointer data) {
    if(!v->can_relabel){
        warn(DEBUG_WARN, "Cannot relabel %s", v->path);
        return FALSE;
    }

    char *label = ask_for_label(v, NULL);
    if(!label) return FALSE;

    warn(DEBUG_INFO, "Relabeling %s as %s", v->path, label);
    struct callback_data *d = g_slice_new(struct callback_data);
    d->v = v;
    d->req_v = v;
    d->callback = callback;
    d->data = data;
    wmudisks_ref(v);
    v->pending_actions++;

    udisks_filesystem_call_set_label( 
        udisks_object_peek_filesystem(v->obj),
        label,
        g_variant_new("a{sv}", NULL),
        NULL,
        (GAsyncReadyCallback)wmudisks_relabel_cb,
        d
    );
    return TRUE;
}
