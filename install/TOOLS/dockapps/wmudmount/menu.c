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

#include <stdarg.h>
#include <gtk/gtk.h>

#include "die.h"
#include "dock.h"
#include "menu.h"
#include "conf.h"
#include "askpass.h"

static wmudisks_volume *menu_volume = NULL;
static GtkMenu *menu;

#define DEF(action,desc) static GtkMenuItem *action##button; \
static void menu_activate_##action (GtkMenuItem *i G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED){ \
    warn(DEBUG_DEBUG, "Menu actvated " #action); \
    if(menu_volume) wmudisks_##action (menu_volume, dock_action_callback, desc); \
}

DEF(mount,"Mount");
DEF(unmount,"Unmount");
DEF(relabel,"Relabel");
DEF(unlock,"Unlock");
DEF(lock,"Lock");
DEF(forget_passphrase,"Forget passphrase");
DEF(eject,"Eject");
DEF(detach,"Detach");

static void menu_activate_command(GtkMenuItem *i G_GNUC_UNUSED, gpointer data){
    struct user_command *u = (struct user_command *)data;
    warn(DEBUG_DEBUG, "Menu actvated command %s", u->title);
    exec_command(menu_volume, u);
}

#define ADD(action) gtk_menu_shell_append((GtkMenuShell *)menu, (GtkWidget *)(action##button = GTK_MENU_ITEM(gtk_menu_item_new()))); g_signal_connect(action##button, "activate", G_CALLBACK(menu_activate_##action), NULL)

void init_menu(){
    menu = GTK_MENU(gtk_menu_new());
    ADD(mount);
    ADD(unmount);
    ADD(relabel);
    ADD(unlock);
    ADD(lock);
    ADD(forget_passphrase);
    ADD(eject);
    ADD(detach);

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_widget_show(sep);
    gtk_menu_shell_append((GtkMenuShell *)menu, sep);

    for(struct user_command *u = menu_commands; u->title; u++){
        u->menu_item = GTK_MENU_ITEM(gtk_menu_item_new());
        gtk_menu_shell_append((GtkMenuShell *)menu, (GtkWidget *)u->menu_item);
        g_signal_connect(u->menu_item, "activate", G_CALLBACK(menu_activate_command), u);
    }
}

static void set_label(GtkMenuItem *i, gboolean show, char *fmt, ...){
    static char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int l = g_vsnprintf(buf, sizeof(buf), fmt, ap);
    while(buf[--l]==' ') buf[l]='\0';
    gtk_menu_item_set_label(i, buf);
    va_end(ap);
    gtk_widget_show((GtkWidget*)i);
    gtk_widget_set_sensitive((GtkWidget*)i, show);
}

gboolean update_menu(wmudisks_volume *v, gboolean do_name){
    if(!v) return FALSE;

    menu_volume = v;
    const char *name = do_name?wmudisks_get_display_name(v):"";
    set_label(mountbutton, !v->mountpoint && wmudisks_can_mount(v), "Mount %s", name);
    set_label(unmountbutton, !!v->mountpoint, "Unmount %s", name);
    set_label(unlockbutton, wmudisks_can_unlock(v), "Unlock %s", name);
    set_label(relabelbutton, wmudisks_can_relabel(v), "Change label %s %s", do_name?"of":"", name);
    set_label(lockbutton, wmudisks_can_lock(v), "Lock %s", name);
    set_label(forget_passphrasebutton, use_keyring && (wmudisks_can_lock(v) || wmudisks_can_unlock(v)), "Forget passphrase %s %s", do_name?"for":"", name);
    set_label(ejectbutton, wmudisks_can_eject(v), "Eject %s", name);
    set_label(detachbutton, wmudisks_can_detach(v), "Detach %s", name);

    for(struct user_command *u = menu_commands; u->title; u++){
        set_label(u->menu_item, user_command_enabled(v,u), "%s %s", u->title, name);
    }

    return TRUE;
}

void show_menu(GtkWidget *w, GdkEventButton *ev){
    GtkWidget *att = gtk_menu_get_attach_widget(menu);
    if (att != w) {
        if (att) {
            gtk_menu_detach(menu);
        }
        gtk_menu_attach_to_widget(menu, w, NULL);
    }
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, ev->button, ev->time);
}

void close_menu(){
    gtk_menu_popdown(menu);
}
