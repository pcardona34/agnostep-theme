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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "askpass.h"
#include "conf.h"
#include "die.h"
#include "notify.h"
#include "chars.h"
#include "wmudisks.h"
#include "menu.h"
#include "dock.h"

/* Signal handling */
static int sig_fd[2];

static void sighandler(int sig){
    if (write(sig_fd[1], &sig, sizeof(sig)) < 0) {
        // Nothing much we can do about it inside a signal handler, but gcc
        // complains if we don't test it.
    }
    signal(sig, sighandler);
}

static gboolean sig_caught(GIOChannel *source G_GNUC_UNUSED, GIOCondition cond G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED){
    int sig;
    while(read(sig_fd[0], &sig, sizeof(sig)) > 0){
        switch(sig){
          case SIGINT:
          case SIGQUIT:
          case SIGKILL: // Useless, but...
          case SIGPIPE:
          case SIGTERM:
          case SIGABRT:
            gtk_main_quit();
            break;
          case SIGHUP:
            // Reload something?
            break;
          case SIGUSR1:
            mount_button_click();
            redraw_dock();
            break;
          case SIGUSR2:
            eject_button_click();
            redraw_dock();
            break;
          default:
            if (sig == SIGRTMIN+0) {
                left_button_click();
                redraw_dock();
            } else if (sig == SIGRTMIN+1) {
                right_button_click();
                redraw_dock();
            }
            break;
        }
    }
    return TRUE;
}

/* Misc */

GQuark app_generic_error_quark(void) {
    return g_quark_from_static_string("app-generic-error-quark");
}

GQuark app_no_error_quark(void) {
    return g_quark_from_static_string("app-no-error-quark");
}

char *format_byte_size(guint64 size){
    static const char *byte_size_names[] = { "", "k", "M", "G", "T", "P", "E", "Z", "Y" };

    unsigned int i;
    double sz = size;
    for (i = 0; sz >= 1000 && i < sizeof(byte_size_names) - 1; i++, sz /= 1000.0);
    if (i == 0) {
        return g_strdup_printf("%ld", size);
    } else {
        return g_strdup_printf("%.2f %sB", sz, byte_size_names[i]);
    }
}

static gboolean parse_opt(const gchar *option_name, const gchar *value, gpointer data G_GNUC_UNUSED, GError **err){
    if(!strcmp(option_name,"-V") || !strcmp(option_name,"--version")){
        printf("%s\n", VERSION);
        exit(0);
    } else if(!strcmp(option_name,"-v") || !strcmp(option_name,"--verbose")){
        warn_level++;
    } else if(!strcmp(option_name,"-q") || !strcmp(option_name,"--quiet")){
        warn_level--;
    } else if(!strcmp(option_name,"-n") || !strcmp(option_name,"--notify")){
        if(!strcmp(value,"none")){
            notify_level = NOTIFY_NONE;
        } else if(!strcmp(value,"err") || !strcmp(value,"error")){
            notify_level = NOTIFY_ERROR;
        } else if(!strcmp(value,"warn") || !strcmp(value,"warning")){
            notify_level = NOTIFY_WARN;
        } else if(!strcmp(value,"info") || !strcmp(value,"all")){
            notify_level = NOTIFY_INFO;
        } else {
            g_set_error(err, G_OPTION_ERROR, G_OPTION_ERROR_FAILED, "Notification level '%s' is not recognized", value);
            return FALSE;
        }
    } else if(!strcmp(option_name,"--exclude-system-volumes") || !strcmp(option_name,"--skip-system-filesystems")){
        fsm_skip_root_only_volumes = TRUE;
        pager_skip_root_only_volumes = TRUE;
    } else if(!strcmp(option_name,"--include-system-volumes") || !strcmp(option_name,"--include-system-filesystems")){
        fsm_skip_root_only_volumes = FALSE;
        pager_skip_root_only_volumes = FALSE;
    } else if(!strcmp(option_name,"--fsm-exclude-system-volumes")){
        fsm_skip_root_only_volumes = TRUE;
    } else if(!strcmp(option_name,"--fsm-include-system-volumes")){
        fsm_skip_root_only_volumes = FALSE;
    } else if(!strcmp(option_name,"--pager-exclude-system-volumes")){
        pager_skip_root_only_volumes = TRUE;
    } else if(!strcmp(option_name,"--pager-include-system-volumes")){
        pager_skip_root_only_volumes = FALSE;
    }
    return TRUE;
}
static GOptionEntry entries[] = {
    {"version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Print the version number and exit", NULL},
    {"no-default-config", 0, 0, G_OPTION_ARG_NONE, NULL, "Do not read the default configuration files", NULL},
    {"conf", 0, 0, G_OPTION_ARG_STRING, NULL, "Specify a configuration file to use in addition to the default", NULL},
    {"verbose", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Print more messages (can be repeated)", NULL},
    {"quiet", 'q', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Print fewer messages (can be repeated)", NULL},
#ifdef HAVE_LIBNOTIFY
    {"notify", 'n', 0, G_OPTION_ARG_CALLBACK, parse_opt, "Level of notifications to show: none, error, warn, info", NULL},
#endif
    {"return-to-fsm-timeout", 0, 0, G_OPTION_ARG_INT, &return_to_fsm_timeout, "Seconds before automatically returning to the FSM display (-1 to disable)", NULL},

    {"skip-system-filesystems", 0, G_OPTION_FLAG_HIDDEN|G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, NULL, NULL},
    {"include-system-filesystems", 0, G_OPTION_FLAG_HIDDEN|G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, NULL, NULL},
    {"exclude-system-volumes", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Do not include system volumes in the FSM or when paging", NULL},
    {"include-system-volumes", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Do not include system volumes in the FSM or when paging", NULL},
    {"fsm-exclude-system-volumes", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Do not include system volumes in the FSM", NULL},
    {"fsm-include-system-volumes", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Do not include system volumes in the FSM", NULL},
    {"pager-exclude-system-volumes", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Do not include system volumes when paging", NULL},
    {"pager-include-system-volumes", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, parse_opt, "Do not include system volumes when paging", NULL},

    {"allow-insecure-memory", 0, 0, G_OPTION_ARG_NONE, &allow_insecure_memory, "Allow passphrases to be temporarily stored in insecure memory", NULL},
    {"no-allow-insecure-memory", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &allow_insecure_memory, "Exit if secure memory cannot be allocated", NULL},
    {"allow-core-files", 0, 0, G_OPTION_ARG_NONE, &allow_core_files, "Allow execution even if core file dumping cannot be disabled", NULL},
    {"no-allow-core-files", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &allow_core_files, "Exit if core file dumping cannot be disabled", NULL},
    {"blink-full-filesystems", 0, 0, G_OPTION_ARG_NONE, &blink_full_filesystems, "Blink when a rw filesystem is over 95% full", NULL},
    {"no-blink-full-filesystems", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &blink_full_filesystems, "Do not blink when a rw filesystem is over 95% full", NULL},
    {"non-wmaker", 0, 0, G_OPTION_ARG_NONE, &nonwmaker, "Use in a non-Window Maker window manager", NULL},
    {"use-secret-service", 0, 0, G_OPTION_ARG_NONE, &use_keyring, "Use the Secrets service for encrypted filesystem unlock passphrase storage", NULL},
    {"no-secret-service", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &use_keyring, "Do not use the Secrets service for encrypted filesystem unlock passphrase storage", NULL},
    {NULL}
};

// This may not be defined anywhere, so define it now
int pipe2(int pipefd[2], int flags);

int main(int argc, char *argv[]){
    warn(DEBUG_INFO, "Parsing conf files");
    load_conf(&argc, argv);

    warn(DEBUG_INFO, "Initializing GTK");
    GOptionContext *context;
    context = g_option_context_new("");
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    g_option_context_add_main_entries(context, entries, NULL);
    GError *err = NULL;
    if(!g_option_context_parse(context, &argc, &argv, &err)){
        die("%s", err?err->message:"Failed to parse command line options");
    }
    g_clear_error(&err);
    g_option_context_free(context);

    init_notify();
    init_menu();
    create_dock_icon();

    warn(DEBUG_DEBUG, "Selecting and initializing Secrets service");
    init_keyring();

    warn(DEBUG_DEBUG, "Initializing udisks interface");
    if(!wmudisks_init()) die("Could not connect to the udisks daemon");

    warn(DEBUG_DEBUG, "Setting up signal handling");
    errno=0;
    if(!pipe2(sig_fd,O_NONBLOCK)){
        GIOChannel *channel = g_io_channel_unix_new(sig_fd[0]);
        g_io_add_watch(channel, G_IO_IN, sig_caught, NULL);
        g_io_channel_unref(channel);
        signal(SIGINT, sighandler);
        signal(SIGQUIT, sighandler);
        signal(SIGKILL, sighandler); // Useless, but...
        signal(SIGPIPE, sighandler);
        signal(SIGTERM, sighandler);
        signal(SIGABRT, sighandler);
        signal(SIGHUP, sighandler);
        signal(SIGUSR1, sighandler);
        signal(SIGUSR2, sighandler);
        signal(SIGRTMIN+0, sighandler);
        signal(SIGRTMIN+1, sighandler);
        warn(DEBUG_INFO, "SIGRTMIN+0 is %d", SIGRTMIN+0);
        warn(DEBUG_INFO, "SIGRTMIN+1 is %d", SIGRTMIN+1);
    } else {
        warn(DEBUG_WARN, "Signal handling initialization failed (%s), will probably die on any signal", strerror(errno));
    }

    warn(DEBUG_DEBUG, "Starting GTK main loop");
    gtk_main();
    warn(DEBUG_DEBUG, "GTK main loop exited");
    return 0;
}
