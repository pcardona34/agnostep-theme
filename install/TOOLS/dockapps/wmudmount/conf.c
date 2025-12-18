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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <gdk/gdk.h>

#include "askpass.h"
#include "conf.h"
#include "die.h"
#include "dock.h"
#include "notify.h"

int sglclick_command_idx = -1;
int dblclick_command_idx = -1;
struct user_command *menu_commands = NULL;
int num_menu_commands = 0;

struct volume_inex *volume_inex = NULL;
int num_volume_inex = 0;

static int parse_command(char *k, char *v, const char *filename, int line){
    if(!v) die("%s requires a value at %s:%d", k, filename, line);
    num_menu_commands++;
    menu_commands = g_realloc(menu_commands, sizeof(struct user_command)*(num_menu_commands+1));
    menu_commands[num_menu_commands].title = NULL;
    struct user_command *u = menu_commands+num_menu_commands-1;

    u->filename = filename;
    u->line = line;
    u->need_mountpoint = 0;
    u->need_dev = 0;

    char *p = strchr(v, ';');
    if(!p) die("Missing semicolon in %s at %s:%d: A command spec is \"title; command [args]\"", k, filename, line);
    char *cmd = p+1; while(isspace(*cmd)) cmd++;
    *(p--)='\0';
    while(p>v && isspace(*p)) *(p--)='\0';
    u->title = g_strdup(v);
    if(p==v) die("Missing title in %s at %s:%d: A command spec is \"title; command [args]\"", k, filename, line);
    if(!*cmd) die("Missing command in %s at %s:%d: A command spec is \"title; command [args]\"", k, filename, line);

    char **argv;
    GError *err = NULL;
    if (!g_shell_parse_argv(cmd, &u->argc, &argv, &err)) {
        die("Failed to parse command in %s at %s:%d: %s", k, filename, line, err?err->message:"<unspecified error>");
    }
    g_clear_error(&err);

    u->argv=(const char**)argv;

    // Validate escapes and set "need" flags
    for(int i=0; i<u->argc; i++){
        const char *pp=u->argv[i];
        while(*pp){
            if(*pp == '%'){
                switch(pp[1]){
                  case 'p':
                  case '%':
                    break;
                  case 'd':
                    u->need_dev = 1;
                    break;
                  case 'm':
                    u->need_mountpoint = 1;
                    break;
                  default:
                    die("Invalid percent-escape in %s at %s:%d (use %%%% if you need an actual percent sign)", k, filename, line);
                    break;
                }
                pp+=2;
            } else {
                pp++;
            }
        }
    }

    return num_menu_commands-1;
}

static void parse_volume_inex(char *k, char *v, int location, int exclude, const char *filename, int line){
    if(!v) die("%s requires a value at %s:%d", k, filename, line);
    num_volume_inex++;
    volume_inex = g_realloc(volume_inex, sizeof(struct volume_inex)*(num_volume_inex+1));
    volume_inex[num_volume_inex].type = 0;
    volume_inex[num_volume_inex].location = 0;
    volume_inex[num_volume_inex].id = NULL;
    struct volume_inex *vie = volume_inex+num_volume_inex-1;

    if(!strncmp(v, "UUID=", 5)){
        vie->type=VOLUME_IN_EX_TYPE_UUID;
        vie->id=g_strdup(v+5);
    } else if(!strncmp(v, "LABEL=", 6)){
        vie->type=VOLUME_IN_EX_TYPE_LABEL;
        vie->id=g_strdup(v+6);
    } else {
        vie->type=VOLUME_IN_EX_TYPE_DEV;
        vie->id=g_strdup(v);
    }
    vie->location=location;
    vie->exclude=exclude;
    if(!vie->id || !vie->id[0] || vie->id[0]==' ')
        die("Invalid value for %s at %s:%d", k, filename, line);
}

static void parse_conf(const char *filename, gboolean require_exists){
    FILE *fp = fopen(filename, "r");
    if(!fp){
        if(require_exists) die("Conf file '%s' could not be opened: %s", filename, strerror(errno));
        return;
    }
    warn(DEBUG_DEBUG, "Parsing conf file '%s'", filename);

    int buflen = 1024, o=0;
    char *buf=g_malloc(buflen+2); // +2 so we can prefix the key with "--"
    char *b=buf+2, *k, *v, *p;
    int line=0;
    while(fgets(b+o, buflen-o, fp)){
        o=strlen(b);

        // Ran out of room in the buffer? Realloc and read more
        if(b[o-1]!='\n'){
            buflen*=2;
            buf=g_realloc(buf, buflen+2);
            b=buf+2;
            continue;
        }

        line++;
        while(o>0 && isspace(b[o-1])) b[--o]='\0';
        for(k=b; isspace(*k); k++);
        if(!*k || *k=='#'){ // blank line or comment
            o=0;
            continue;
        }
        p=strchr(k, '=');
        if(!p){
            v = NULL;
            p = b+o-2;
        } else {
            for(v=p+1; isspace(*v); v++);
            *(p--)='\0';
        }
        while(p>k && isspace(*p)) *(p--)='\0';
        if(p<k) die("Syntax error at %s:%d", filename, line);

        warn(DEBUG_DEBUG, "Found directive \"%s\" at %s:%d", k, filename, line);

        k[-1]='-'; k[-2]='-'; // Since we read starting at buf[2], this is safe
        if(!strcmp(k,"verbose")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            warn_level++;
        } else if(!strcmp(k,"quiet")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            warn_level--;
        } else if(!strcmp(k,"non-wmaker")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            nonwmaker=TRUE;
        } else if(!strcmp(k,"warn")){
            if(!v) die("%s requires a value at %s:%d", k, filename, line);
            if(!strcmp(v,"err") || !strcmp(v,"error")){
                warn_level = DEBUG_ERROR;
            } else if(!strcmp(v,"warn") || !strcmp(v,"warning")){
                warn_level = DEBUG_WARN;
            } else if(!strcmp(v,"info")){
                warn_level = DEBUG_INFO;
            } else if(!strcmp(v,"debug") || !strcmp(v,"all")){
                warn_level = DEBUG_DEBUG;
            } else {
                die("Warning level '%s' is not recognized", v);
            }
        } else if(!strcmp(k,"notify")){
            if(!v) die("%s requires a value at %s:%d", k, filename, line);
#ifdef HAVE_LIBNOTIFY
            if(!strcmp(v,"none")){
                notify_level = NOTIFY_NONE;
            } else if(!strcmp(v,"err") || !strcmp(v,"error")){
                notify_level = NOTIFY_ERROR;
            } else if(!strcmp(v,"warn") || !strcmp(v,"warning")){
                notify_level = NOTIFY_WARN;
            } else if(!strcmp(v,"info") || !strcmp(v,"all")){
                notify_level = NOTIFY_INFO;
            } else {
                die("Notification level '%s' is not recognized", v);
            }
#else
            warn(DEBUG_WARN, "Notification support was not compiled in");
#endif
        } else if(!strcmp(k,"exclude-system-volumes") || !strcmp(k,"skip-system-filesystems")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            fsm_skip_root_only_volumes = TRUE;
            pager_skip_root_only_volumes = TRUE;
        } else if(!strcmp(k,"include-system-volumes") || !strcmp(k,"include-system-filesystems")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            fsm_skip_root_only_volumes = FALSE;
            pager_skip_root_only_volumes = FALSE;
        } else if(!strcmp(k,"fsm-exclude-system-volumes")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            fsm_skip_root_only_volumes = TRUE;
        } else if(!strcmp(k,"fsm-include-system-volumes")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            fsm_skip_root_only_volumes = FALSE;
        } else if(!strcmp(k,"pager-exclude-system-volumes")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            pager_skip_root_only_volumes = TRUE;
        } else if(!strcmp(k,"pager-include-system-volumes")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            pager_skip_root_only_volumes = FALSE;
        } else if(!strcmp(k, "return-to-fsm-timeout")){
            if(!v) die("%s requires a value at %s:%d", k, filename, line);
            long i = strtol(v, &p, 0);
            if(i<0) i=-1;
            return_to_fsm_timeout = (gint)i;
            if(!*v || *p || i!=(long)return_to_fsm_timeout)
                die("Invalid value for %s in %s:%d", k, filename, line);
        } else if(!strcmp(k, "allow-insecure-memory")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            allow_insecure_memory = TRUE;
        } else if(!strcmp(k, "no-allow-insecure-memory")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            allow_insecure_memory = FALSE;
        } else if(!strcmp(k, "use-secret-service")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            use_keyring = TRUE;
        } else if(!strcmp(k, "no-secret-service")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            use_keyring = FALSE;
        } else if(!strcmp(k, "allow-core-files")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            allow_core_files = TRUE;
        } else if(!strcmp(k, "no-allow-core-files")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            allow_core_files = FALSE;
        } else if(!strcmp(k, "blink-full-filesystems")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            blink_full_filesystems = TRUE;
        } else if(!strcmp(k, "no-blink-full-filesystems")){
            if(v) die("%s cannot accept a value at %s:%d", k, filename, line);
            blink_full_filesystems = FALSE;
        } else if(!strcmp(k, "click-command")){
            if(sglclick_command_idx>=0) warn(DEBUG_ERROR, "Replacing click command defined at %s:%d with command defined at %s:%d", menu_commands[sglclick_command_idx].filename, menu_commands[sglclick_command_idx].line, filename, line);
            sglclick_command_idx = parse_command(k, v, filename, line);
        } else if(!strcmp(k, "double-click-command")){
            if(dblclick_command_idx>=0) warn(DEBUG_ERROR, "Replacing double-click command defined at %s:%d with command defined at %s:%d", menu_commands[dblclick_command_idx].filename, menu_commands[dblclick_command_idx].line, filename, line);
            dblclick_command_idx = parse_command(k, v, filename, line);
        } else if(!strcmp(k, "command")){
            parse_command(k, v, filename, line);
        } else if(!strcmp(k, "include-volume")){
            parse_volume_inex(k, v, VOLUME_IN_EX_LOCATION_ALL, 0, filename, line);
        } else if(!strcmp(k, "exclude-volume")){
            parse_volume_inex(k, v, VOLUME_IN_EX_LOCATION_ALL, 1, filename, line);
        } else if(!strcmp(k, "fsm-include-volume")){
            parse_volume_inex(k, v, VOLUME_IN_EX_LOCATION_FSM, 0, filename, line);
        } else if(!strcmp(k, "fsm-exclude-volume")){
            parse_volume_inex(k, v, VOLUME_IN_EX_LOCATION_FSM, 1, filename, line);
        } else if(!strcmp(k, "pager-include-volume")){
            parse_volume_inex(k, v, VOLUME_IN_EX_LOCATION_PAGER, 0, filename, line);
        } else if(!strcmp(k, "pager-exclude-volume")){
            parse_volume_inex(k, v, VOLUME_IN_EX_LOCATION_PAGER, 1, filename, line);
        } else {
            warn(DEBUG_WARN, "Ignoring unrecognized option \"%s\" in %s:%d", k, filename, line);
        }
        o=0;
    }

    fclose(fp);
}

void load_conf(int *argc, char **argv){
    menu_commands = g_malloc(sizeof(struct user_command));
    menu_commands[0].title = NULL;

    volume_inex = g_malloc(sizeof(struct volume_inex));
    volume_inex[0].type = 0;
    volume_inex[0].location = 0;
    volume_inex[0].id = NULL;

    int skip_default=0;
    for(int i=0; i<*argc; i++){
        if(!g_strcmp0(argv[i], "--no-default-config")) skip_default=1;
    }

    if(!skip_default){
        parse_conf("/etc/wmudmount.conf", FALSE);
        parse_conf("/usr/local/etc/wmudmount.conf", FALSE);
        const char *homedir = g_getenv("HOME");
        if(!homedir) homedir = g_get_home_dir();
        if(homedir){
            char *f = g_strjoin("/", homedir, ".wmudmountrc", NULL);
            parse_conf(f, FALSE);
        }
    }

    int i=1, j=1;
    while(i < *argc){
        if(!g_strcmp0(argv[i], "--no-default-config")){
            i++;
        } else if(!g_strcmp0(argv[i], "--conf")){
            if(i+1 >= *argc) die("--conf must have a value following");
            parse_conf(argv[i+1], TRUE);
            i+=2;
        } else if(!strncmp(argv[i], "--conf=", 7)){
            if(!argv[i][7]) die("--conf must have a value following");
            parse_conf(argv[i]+7, TRUE);
            i++;
        } else {
            argv[j++] = argv[i++];
        }
    }
    if(j != i) argv[j] = NULL;
    *argc = j;
}

static const char *str_for_escape(wmudisks_volume *v, const char *p){
    switch(p[1]){
      case 'p':
        return v->path;
      case 'd':
        return v->dev;
      case 'm':
        return v->mountpoint;
      case '%':
        return "%";
      default:
        die("Invalid percent-escape");
    }
}

static const char *free_str_for_escape(wmudisks_volume *v G_GNUC_UNUSED, const char *p, const char *str G_GNUC_UNUSED){
    switch (p[1]) {
      case 'p':
      case 'd':
      case 'm':
      case '%':
        return p+2;
      default:
        die("Invalid percent-escape");
    }
}

int exec_command(wmudisks_volume *v, struct user_command *cmd){
    char **argv = g_malloc(sizeof(char *)*(cmd->argc+1));
    for(int i=0; i<cmd->argc; i++){
        // First, determine the needed string length
        int len=0;
        const char *p=cmd->argv[i];
        while(*p){
            if(*p == '%'){
                const char *c=str_for_escape(v,p);
                len+=strlen(c?c:"(null)");
                p=free_str_for_escape(v,p,c);
            } else {
                len++; p++;
            }
        }
        // Then malloc the memory and fill it
        char *q;
        argv[i] = q = g_malloc(len+1);
        p=cmd->argv[i];
        while(*p){
            if(*p == '%'){
                const char *c=str_for_escape(v,p);
                int l = strlen(c?c:"(null)");
                memcpy(q, c?c:"(null)", l);
                p=free_str_for_escape(v,p,c);
                q+=l;
            } else {
                *(q++) = *(p++);
            }
        }
        *q='\0';
    }
    argv[cmd->argc] = NULL;

    GError *err = NULL;
    int ret = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);
    if (!ret) {
        warn(DEBUG_ERROR, "Could not execute command %s: %s", argv[0], err?err->message:"<unknown error>");
    }
    g_clear_error(&err);

    for(int i=0; i<cmd->argc; i++) g_free(argv[i]);
    g_free(argv);

    return ret;
}
