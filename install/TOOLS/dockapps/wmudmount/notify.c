/* wmudmount
 * Copyright © 2010-2014  Brad Jorsch <anomie@users.sourceforge.net>
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
#include "notify.h"
#include "die.h"
#include "dock.h"

#include <glib.h>
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif
#include <stdarg.h>

int notify_level = NOTIFY_INFO;
gboolean notify_body_is_used = FALSE;
gboolean notify_body_can_markup = FALSE;

void init_notify(){
#ifdef HAVE_LIBNOTIFY
    warn(DEBUG_DEBUG, "Initializing libnotify");
    if(!notify_init(g_get_prgname()))
        die("Failed to initialize libnotify");

    GList *caps = notify_get_server_caps();
    for(GList *l=caps; l; l=g_list_next(l)){
        if(!strcmp(l->data, "body")) notify_body_is_used = TRUE;
        if(!strcmp(l->data, "body-markup")) notify_body_can_markup = TRUE;
    }
    warn(DEBUG_DEBUG, notify_body_is_used?"Notification bodies are supported":"Notification bodies are not supported");
    warn(DEBUG_DEBUG, notify_body_can_markup?"Notification body markup is supported":"Notification body markup is not supported");
    g_list_foreach(caps, (GFunc)g_free, NULL);
    g_list_free(caps);
#else
    warn(DEBUG_DEBUG, "Notification support was not compiled in");
#endif
}

#ifdef HAVE_LIBNOTIFY
static void notify_goto(NotifyNotification *n G_GNUC_UNUSED, char *action G_GNUC_UNUSED, gpointer data){
    dock_action_goto((wmudisks_volume *)data);
}

static void notify_mount(NotifyNotification *n G_GNUC_UNUSED, char *action G_GNUC_UNUSED, gpointer data){
    dock_action_mount((wmudisks_volume *)data);
}

static void notify_eject(NotifyNotification *n G_GNUC_UNUSED, char *action G_GNUC_UNUSED, gpointer data){
    dock_action_eject((wmudisks_volume *)data);
}

static void notify_closed_signaled(GObject *sender, gpointer data G_GNUC_UNUSED){
    g_object_unref(sender);
}

static void _notify(const char *category, GIcon *icon, struct notify_actions *actions, const char *summary, const char *body, gboolean transient){
    gchar *icon_name = NULL;
    if(G_IS_THEMED_ICON(icon)){
        const gchar * const *icon_names = g_themed_icon_get_names(G_THEMED_ICON(icon));
        if(icon_names) icon_name=g_strdup(icon_names[0]);
    } else if(G_IS_FILE_ICON(icon)){
        icon_name = g_icon_to_string(icon);
    }
    NotifyNotification *n = notify_notification_new(summary, body, icon_name);
    if(category) notify_notification_set_category(n, category);
    if(transient) notify_notification_set_hint_int32(n, "transient", 1);
    if(actions){
        wmudisks_ref(actions->volume);
        notify_notification_add_action(n, "default", "Go to", notify_goto, actions->volume, (GFreeFunc)wmudisks_unref);
        if(actions->mount_text){
            wmudisks_ref(actions->volume);
            notify_notification_add_action(n, "mount", actions->mount_text, notify_mount, actions->volume, (GFreeFunc)wmudisks_unref);
        }
        if(actions->eject_text){
            wmudisks_ref(actions->volume);
            notify_notification_add_action(n, "eject", actions->eject_text, notify_eject, actions->volume, (GFreeFunc)wmudisks_unref);
        }
    }
    GError *err = NULL;
    if(!notify_notification_show(n, &err)){
        warn(DEBUG_ERROR, "Failed to show notification \"%s\" \"%s\": %s", summary, body, err?err->message:"<unknown error>");
    }
    g_clear_error(&err);
    g_signal_connect(G_OBJECT(n), "closed", G_CALLBACK(notify_closed_signaled), NULL);
    g_free(icon_name);
} 
#endif

void notify(int level, const char *category, gboolean transient, GIcon *icon, struct notify_actions *actions, const char *summary, const char *body, ...){
#ifdef HAVE_LIBNOTIFY
    if(notify_level<level) return;

    char *b = NULL, *s;
    va_list ap1, ap2;
    va_start(ap1, body);
    if(body){
        va_copy(ap2, ap1);
        b = g_strdup_vprintf(body, ap2);
        va_end(ap2);
    }
    s = g_strdup_vprintf(summary, ap1);
    va_end(ap1);
    _notify(category, icon, actions, s, b, transient);
    g_free(b);
    g_free(s);
#endif
}
