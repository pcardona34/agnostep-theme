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
#include <stdarg.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "conf.h"
#include "die.h"
#include "dock.h"
#include "notify.h"
#include "fsm.h"
#include "chars.h"
#include "menu.h"
#include "misc.h"

#include "button_images.xpm"

/* Globals */
static GdkDisplay *display;
static GtkWidget *mainwin, *iconwin;
static GdkPixbuf *button_images_pixbuf;
static GtkIconTheme *theme;

#define LEFT_BUTTON 36
#define RIGHT_BUTTON 48
#define MOUNT_BUTTON 4
#define EJECT_BUTTON 16
static int button_down = 0;

#define BUTTON_IMG_LEFT    0
#define BUTTON_IMG_RIGHT  10
#define BUTTON_IMG_EJECT  20
#define BUTTON_IMG_DETACH 30
#define BUTTON_IMG_MOUNT  40
#define BUTTON_IMG_UMOUNT 50
#define BUTTON_IMG_UNLOCK 60
#define BUTTON_IMG_LOCK   70

static int cur_index = -1;
static wmudisks_volume *fsm_vols[7];
static wmudisks_volume *menu_volume = NULL;
static int hover_row = -1;

gint return_to_fsm_timeout = 60;
gboolean fsm_skip_root_only_volumes = TRUE;
gboolean pager_skip_root_only_volumes = TRUE;
gboolean fsm_blink = FALSE;
gboolean blink_full_filesystems = TRUE;
gboolean nonwmaker = FALSE;

cairo_pattern_t *highlight_gradients[2];
cairo_pattern_t *bar_gradients[2];
cairo_pattern_t *fsm_gradients[2];

/* Utility funcs */
static void make_gradients(){
    cairo_pattern_t *g;

    highlight_gradients[0] = g = cairo_pattern_create_radial(
        32, 23, 7,
        32, 23, 28
    );
    cairo_pattern_add_color_stop_rgba(g, 0, 0, 0, 1, 1);
    cairo_pattern_add_color_stop_rgba(g, 1, 0, 0, 1, 0);

    highlight_gradients[1] = g = cairo_pattern_create_radial(
        32, 23, 7,
        32, 23, 28
    );
    cairo_pattern_add_color_stop_rgba(g, 0, 1, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba(g, 1, 1, 0, 0, 0);

    bar_gradients[0] = g = cairo_pattern_create_linear(4, 0, 60, 0);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.04, 0.09, 0.27);
    cairo_pattern_add_color_stop_rgb(g, 1, 0.01, 0.91, 0.94);

    bar_gradients[1] = g = cairo_pattern_create_linear(4, 0, 60, 0);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.27, 0.04, 0.04);
    cairo_pattern_add_color_stop_rgb(g, 1, 1, 0, 0);

    fsm_gradients[0] = g = cairo_pattern_create_linear(30, 0, 60, 0);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.04, 0.09, 0.27);
    cairo_pattern_add_color_stop_rgb(g, 1, 0.01, 0.91, 0.94);

    fsm_gradients[1] = g = cairo_pattern_create_linear(30, 0, 60, 0);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.27, 0.04, 0.04);
    cairo_pattern_add_color_stop_rgb(g, 1, 1, 0, 0);
}

static void draw_window(cairo_t *cr, cairo_region_t *region, int x, int y, int w, int h) {
    cairo_rectangle_int_t rect = { x, y, w, h };
    cairo_region_union_rectangle(region, &rect);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);

    cairo_set_line_width(cr, 1);
    cairo_set_source_rgb(cr, 0.78, 0.78, 0.78);
    cairo_move_to(cr, x + w - .5, y + 1);
    cairo_line_to(cr, x + w - .5, y + h - .5);
    cairo_line_to(cr, x, y + h - .5);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, x + .5, y + h - 1);
    cairo_line_to(cr, x + .5, y + .5);
    cairo_line_to(cr, x + w, y + .5);
    cairo_stroke(cr);
}

static void draw_button(cairo_t *cr, int button, int img, gboolean enabled){
    int x = button, y = 48;

    gboolean pressed = button_down == button;

    cairo_move_to(cr, x + 11.5, y);
    cairo_line_to(cr, x + 11.5, y + 11.5);
    cairo_line_to(cr, x, y + 11.5);
    if (pressed) {
        cairo_set_source_rgb(cr, 1, 1, 1);
    } else {
        cairo_set_source_rgb(cr, 0, 0, 0);
    }
    cairo_stroke(cr);

    cairo_move_to(cr, x + .5, y + 11.5);
    cairo_line_to(cr, x + .5, y + .5);
    cairo_line_to(cr, x + 11.5, y + .5);
    if (pressed) {
        cairo_set_source_rgb(cr, 0, 0, 0);
    } else {
        cairo_set_source_rgb(cr, 1, 1, 1);
    }
    cairo_stroke(cr);

    cairo_save(cr);
    cairo_rectangle(cr, x + 1, y + 1, 10, 10);
    cairo_clip(cr);
    gdk_cairo_set_source_pixbuf(cr, button_images_pixbuf, x + 1 - img, y + 1);
    cairo_paint(cr);
    cairo_restore(cr);

    if (!enabled) {
        cairo_set_source_rgba(cr, 0.67, 0.67, 0.67, 0.67);
        cairo_rectangle(cr, x + 1, y + 1, 10, 10);
        cairo_fill(cr);
    }
}

static void update_volume_includes(int location) {
    gboolean skip_root_only=FALSE;
    if (location & VOLUME_IN_EX_LOCATION_PAGER) {
        skip_root_only|=pager_skip_root_only_volumes;
    }
    if (location & VOLUME_IN_EX_LOCATION_FSM) {
        skip_root_only|=fsm_skip_root_only_volumes;
    }

    for (guint i = volumes->len; i-- > 0; ) {
        wmudisks_volume *v = volume_index(i);
        v->include = 1;
        if ((v->force_include & location) || (v->force_exclude & location)) {
            v->include = !(v->force_exclude & location);
            continue;
        }
        if (skip_root_only && v->root_only) {
            v->include = 0;
        }
    }
}

static void draw_fsm(cairo_t *cr) {
    update_volume_includes(VOLUME_IN_EX_LOCATION_FSM);
    wmudisks_volume *v;
    int n = 0;
    double pcts[7];
    for (int i = 0; i < 7; i++) {
        fsm_vols[i] = NULL;
        pcts[i] = -1;
    }
    for (guint i = volumes->len; i-- > 0; ) {
        v = volume_index(i);
        if (!v->mountpoint || v->total_size == 0 || !v->include) {
            continue;
        }
        double pct = (v->total_size - v->free_size) / (double)v->total_size;
        fsm_vols[n] = v;
        pcts[n] = pct;
        for (int j = n - 1; j >= 0; j--) {
            if (pct <= pcts[j]) {
                break;
            }
            fsm_vols[j+1] = fsm_vols[j];
            pcts[j+1] = pcts[j];
            fsm_vols[j] = v;
            pcts[j] = pct;
        }
        if (n < 6) n++;
    }
    if (!n) {
        return;
    }

    for (int i = 0; i < n; i++) {
        const char *name = fsm_vols[i]->label;
        if (!name) {
            name = fsm_vols[i]->mountpoint;
        }
        int ww = measure_string(name);
        if (ww < 25) {
            ww = 25;
        }
        draw_string(cr, name, 29 - ww, 6 + i * 6, (i == hover_row) ? 2 : 0);
        int thresh = (pcts[i] > BLINK_THRESHOLD) ? 1 : 0;
        if (fsm_vols[i]->readonly || !fsm_blink || !thresh) {
            cairo_set_source(cr, fsm_gradients[thresh]);
            cairo_rectangle(cr, 30, 6 + i * 6, round(30 * pcts[i]), 5);
            cairo_fill(cr);
        }
    }
}

static GdkPixbuf *get_icon(wmudisks_volume *v) {
    if (!v->icon) {
        return NULL;
    }

    GdkPixbuf *icon = NULL;
    GtkIconInfo *info = gtk_icon_theme_lookup_by_gicon(theme, v->icon, 24, GTK_ICON_LOOKUP_USE_BUILTIN);
    if (info) {
        GError *err = NULL;
        icon = gtk_icon_info_load_icon(info, &err);
        if (!icon) {
            char *c = g_icon_to_string(v->icon);
            warn(DEBUG_ERROR, "Couldn't load icon pixmap for %s: %s", c, err?err->message:"Unknown error");
            g_free(c);
        }
        g_clear_error(&err);
        g_object_unref(info);
    } else {
        char *c = g_icon_to_string(v->icon);
        warn(DEBUG_ERROR, "Couldn't load icon for %s", c);
        g_free(c);
    }
    return icon;
}

static wmudisks_volume *get_volume_at(int x G_GNUC_UNUSED, int y, GdkRectangle *rect) {
    wmudisks_volume *v;
    if(cur_index == -1){
        int i = y/6 - 1;
        if(i < 0 || i > 5) return NULL;
        v = fsm_vols[i];
        if(!v) return NULL;
        if(rect){
            rect->x = 4; rect->y = 6 + i*6; rect->width = 56; rect->height = 6;
        }
        return v;
    } else {
        v = volume_index(cur_index);
        if(rect){
            rect->x = 4; rect->y = 4; rect->width = 40; rect->height = 56;
        }
        return v;
    }
}

static int return_to_fsm_counter = -1;
static gboolean timeout_cb(gpointer p G_GNUC_UNUSED) {
    if (blink_full_filesystems) {
        fsm_blink = !fsm_blink;
    }

    double max_pct;
    if (wmudisks_update_fs_usage(&max_pct) ||
        (blink_full_filesystems && max_pct >= BLINK_THRESHOLD)
    ) {
        if(cur_index == -1){
            redraw_dock();
        } else {
            wmudisks_volume *v = volume_index(cur_index);
            if (v->mountpoint && v->total_size!=0) {
                redraw_dock();
            }
        }
    }

    if (return_to_fsm_counter >= 0) {
        if (--return_to_fsm_counter < 0 && cur_index != -1) {
            cur_index = -1;
            redraw_dock();
        }
    }

    return TRUE;
}

/* All this mess is necessary to avoid firing the single-click command twice on
 * a double-click. Basically, schedule a timer for after the double-click
 * timeout, and have any button presses cancel the timer. */
static guint sgl_click_id = 0;
static wmudisks_volume *sgl_click_volume = NULL; // So we can wmudisks_unref it
static gboolean single_click_cb(gpointer p G_GNUC_UNUSED) {
    wmudisks_volume *v = sgl_click_volume;
    sgl_click_volume = NULL;
    sgl_click_id = 0;
    if (!v) {
        return FALSE;
    }
    if (sglclick_command_idx < 0) {
        if ( cur_index < 0 ) {
            dock_action_goto(v);
        }
    } else if (user_command_enabled(v,&menu_commands[sglclick_command_idx])) {
        warn(DEBUG_DEBUG, "Executing user command \"%s\"", menu_commands[sglclick_command_idx].title);
        exec_command(v, &menu_commands[sglclick_command_idx]);
    } else {
        warn(DEBUG_DEBUG, "User command \"%s\" preconditions not met", menu_commands[sglclick_command_idx].title);
    }
    wmudisks_unref(v);
    return FALSE;
}
static void cancel_sgl_click(){
    if(sgl_click_volume){
        wmudisks_unref(sgl_click_volume);
        sgl_click_volume = NULL;
    }
    if(sgl_click_id){
        g_source_remove(sgl_click_id);
        sgl_click_id = 0;
    }
}
static void schedule_sgl_click(wmudisks_volume *v){
    cancel_sgl_click();
    if (!v) {
        return;
    }

    wmudisks_ref(v);
    sgl_click_volume = v;
    if(dblclick_command_idx<0){
        single_click_cb(NULL);
    } else {
        guint timeout = 400;
        g_object_get(G_OBJECT(gtk_settings_get_default()), "gtk-double-click-time", &timeout, NULL);
        warn(DEBUG_DEBUG, "Setting timeout of %d ms for single-click command", timeout);
        sgl_click_id = g_timeout_add(timeout, single_click_cb, NULL);
    }
}

/* GTK event handlers */

static char *concat_printf(char *buf, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
static char *concat_printf(char *buf, char *format, ...){
    va_list ap;
    va_start(ap, format);

    char *ret;
    char *tmp = g_markup_vprintf_escaped(format, ap);
    if(!buf){
        ret = tmp;
    } else {
        ret = g_strjoin("\n", buf, tmp, NULL);
        g_free(buf);
        g_free(tmp);
    }

    va_end(ap);
    return ret;
}

static gboolean query_tooltip_handler(GtkWidget *widget G_GNUC_UNUSED, gint x, gint y, gboolean keyboard_mode G_GNUC_UNUSED, GtkTooltip *tt, gpointer data G_GNUC_UNUSED) {
    GdkRectangle rect;
    wmudisks_volume *v = get_volume_at(x,y,&rect);
    if(!v) return FALSE;
    GdkPixbuf *icon = get_icon(v);
    gtk_tooltip_set_icon(tt, icon);
    g_object_unref(icon);

    if(y >= 4 && y <= 44 && x >= 4 && x <= 60){
        gtk_tooltip_set_tip_area(tt, &rect);
        char *c = NULL;
        if(v->dev) c = concat_printf(c, "<b>Device:</b> %s", v->dev);
        if(v->label) c = concat_printf(c, "<b>Label:</b> %s", v->label);
        if(v->desc) c = concat_printf(c, "<b>Description:</b> %s", v->desc);
        if(v->uuid) c = concat_printf(c, "<b>UUID:</b> %s", v->uuid);
        if (v->allmountpoints) {
            for (const char **p = v->allmountpoints; *p; p++) {
                c = concat_printf(c, "<b>Mountpoint:</b> %s", *p);
            }
        }
        if(v->total_size!=0){
            double pct = (v->total_size - v->free_size) / (double)v->total_size;
            char *t = format_byte_size(v->total_size);
            char *u = format_byte_size(v->total_size - v->free_size);
            char *f = format_byte_size(v->free_size);
            if(pct >= BLINK_THRESHOLD){
                c = concat_printf(c, "<span foreground=\"red\"><b>Space Used:</b> %s of %s (%.1f%%), %s free</span>", u, t, 100.0*pct, f);
            } else {
                c = concat_printf(c, "<b>Space Used:</b> %s of %s (%.1f%%), %s free", u, t, 100.0*pct, f);
            }
            g_free(t);
            g_free(u);
            g_free(f);
        }
        if(!c) return FALSE;
        if (wmudisks_version) {
            c = concat_printf(c, "<b>UDisks version:</b> %s", wmudisks_version);
        }
        gtk_tooltip_set_markup(tt, c);
        g_free(c);
        return TRUE;
    } else if(y >= 48 && y <= 59){
        rect.y = 48; rect.width=12; rect.height=12;
        if(x >= 4 && x <= 15){
            rect.x = 4; gtk_tooltip_set_tip_area(tt, &rect);
            if (v->mountpoint) {
                gtk_tooltip_set_text(tt, "Unmount the filesystem");
                return TRUE;
            } else if (wmudisks_can_mount(v)) {
                gtk_tooltip_set_text(tt, "Mount the filesystem");
                return TRUE;
            } else if(wmudisks_can_unlock(v)) {
                gtk_tooltip_set_text(tt, "Unlock the device");
                return TRUE;
            }
        } else if(x >= 16 && x <= 25){
            rect.x = 16; gtk_tooltip_set_tip_area(tt, &rect);
            if(wmudisks_can_lock(v)){
                gtk_tooltip_set_text(tt, "Lock the device");
                return TRUE;
            } else if(wmudisks_can_eject(v)){
                gtk_tooltip_set_text(tt, "Eject the media from the drive");
                return TRUE;
            } else if(wmudisks_can_detach(v)){
                gtk_tooltip_set_text(tt, "Detach the drive");
                return TRUE;
            }
        }
    }

    return FALSE;
}

static gboolean draw_handler(GtkWidget *widget, cairo_t *cr, gpointer d G_GNUC_UNUSED) {
    GdkWindow *w = gtk_widget_get_window(widget);

    if (warn_level >= DEBUG_DEBUG) {
        GdkRectangle rect;
        gdk_cairo_get_clip_rectangle(cr, &rect);
        warn(DEBUG_DEBUG, "Redrawing %p (%d,%d+%d+%d)", w, rect.x, rect.y, rect.width, rect.height);
    }

    cairo_region_t *region = cairo_region_create();
    draw_window(cr, region, 3, 3, 58, 42);
    draw_window(cr, region, 3, 47, 26, 14);
    draw_window(cr, region, 35, 47, 26, 14);
    gtk_widget_shape_combine_region(widget, region);
    cairo_region_destroy(region);

    if (!wmudisks_version) {
        int ww = measure_string("Udisks daemon");
        draw_string(cr, "Udisks daemon", 32-ww/2, 24-char_height-1, 3);
        ww = measure_string("is unavailable");
        draw_string(cr, "is unavailable", 32-ww/2, 25, 3);
        draw_button(cr, LEFT_BUTTON, BUTTON_IMG_LEFT, FALSE);
        draw_button(cr, RIGHT_BUTTON, BUTTON_IMG_RIGHT, FALSE);
        draw_button(cr, MOUNT_BUTTON, BUTTON_IMG_MOUNT, FALSE);
        draw_button(cr, EJECT_BUTTON, BUTTON_IMG_EJECT, FALSE);
        return FALSE;
    }

    gboolean show_buttons = FALSE;
    update_volume_includes(VOLUME_IN_EX_LOCATION_PAGER);
    for (guint i = volumes->len; i-- > 0; ) {
        if (volume_index(i)->include) {
            show_buttons = TRUE;
            break;
        }
    }
    draw_button(cr, LEFT_BUTTON, BUTTON_IMG_LEFT, show_buttons);
    draw_button(cr, RIGHT_BUTTON, BUTTON_IMG_RIGHT, show_buttons);

    wmudisks_volume *v = NULL;
    int highlight = 0;
    const char *bottom_text = NULL;
    int bottom_color = 0;
    int bar_width = 0;
    int bar_color = 0;

    if (cur_index == -1) {
        draw_button(cr, MOUNT_BUTTON, BUTTON_IMG_MOUNT, FALSE);
        draw_button(cr, EJECT_BUTTON, BUTTON_IMG_EJECT, FALSE);
    } else {
        v = volume_index(cur_index);
        if (v->mountpoint) {
            highlight = 1;
            bottom_text = v->mountpoint;
            bottom_color = 0;
            double pct = (v->total_size - v->free_size) / (double)v->total_size;
            bar_color = (pct > BLINK_THRESHOLD)?1:0;
            bar_width = (!v->readonly && fsm_blink && bar_color)?0:(int)round(56 * pct);
            draw_button(cr, MOUNT_BUTTON, BUTTON_IMG_UMOUNT, TRUE);
        } else if (wmudisks_can_mount(v)) {
            highlight = 0;
            bottom_text = "<unmounted>";
            bottom_color = 1;
            bar_width = 0;
            draw_button(cr, MOUNT_BUTTON, BUTTON_IMG_MOUNT, TRUE);
        } else if (wmudisks_can_unlock(v)) {
            bottom_text = "<encrypted>";
            draw_button(cr, MOUNT_BUTTON, BUTTON_IMG_UNLOCK, TRUE);
        } else {
            draw_button(cr, MOUNT_BUTTON, BUTTON_IMG_MOUNT, FALSE);
        }
        if (v->pending_actions) {
            highlight = 2;
        }
        if (wmudisks_can_lock(v)) {
            draw_button(cr, EJECT_BUTTON, BUTTON_IMG_LOCK, TRUE);
        } else if(wmudisks_can_eject(v)){
            draw_button(cr, EJECT_BUTTON, BUTTON_IMG_EJECT, TRUE);
        } else if(wmudisks_can_detach(v)){
            draw_button(cr, EJECT_BUTTON, BUTTON_IMG_DETACH, TRUE);
        } else {
            draw_button(cr, EJECT_BUTTON, BUTTON_IMG_EJECT, FALSE);
        }
    }

    cairo_rectangle(cr, 4, 4, 56, 40);
    cairo_clip(cr);
    if (gdk_cairo_get_clip_rectangle(cr, NULL)) {
        if (cur_index == -1) {
            draw_fsm(cr);
        } else {
            if (highlight) {
                cairo_set_source(cr, highlight_gradients[highlight-1]);
                cairo_rectangle(cr, 4, 11, 56, 24);
                cairo_fill(cr);
            }

            const char *s = wmudisks_get_display_name(v);
            int x = 4;
            int rj = wmudisks_display_name_should_right_justify(v);
            if (rj) {
                x = 60-measure_string(s);
                if(x > 4) x = 4;
            }
            draw_string(cr, s, x, 5, rj?1:0);

            if (bar_width) {
                cairo_set_source(cr, bar_gradients[bar_color]);
                cairo_rectangle(cr, 4, 35, bar_width, 2);
                cairo_fill(cr);
            }
            if (bottom_text) {
                draw_string(cr, bottom_text, 4, 38, bottom_color);
            }

            GdkPixbuf *icon = get_icon(v);
            if(icon){
                cairo_rectangle(cr, 20, 11, 24, 24);
                cairo_clip(cr);
                gdk_cairo_set_source_pixbuf(cr, icon, 20, 11);
                cairo_paint(cr);
                g_object_unref(icon);
            }
        }
    }

    return FALSE;
}

void dock_action_callback(wmudisks_volume *v, gpointer ret G_GNUC_UNUSED, gpointer data, gboolean failed, GError *err) {
    struct notify_actions *actions = dock_actions_for_volume(v);
    if (failed) {
        notify(DEBUG_ERROR, "device.error", FALSE, v->icon, actions, "%s failed", "%s of %s failed: %s", data, wmudisks_get_display_name(v), err?err->message:"<unknown error>");
    } else if (err) {
        int ct = 0;
        for (char *p = err->message; *p; p++, ct++) {
            if (*p=='%') {
                ct++;
            }
        }
        char buf[ct];
        for (char *p = err->message, *q = buf; *p; *q++ = *p++) {
            if (*p=='%') {
                *q++='%';
            }
        }
        notify(DEBUG_INFO, "device", TRUE, v->icon, actions, "%s succeeded", err->message, data);
    } else {
        notify(DEBUG_INFO, "device", TRUE, v->icon, actions, "%s succeeded", "%s of %s succeeded", data, wmudisks_get_display_name(v));
    }
    g_slice_free(struct notify_actions, actions);
    redraw_dock();
}

void dock_lock_callback(wmudisks_volume *v, gpointer ret, gpointer data, gboolean failed, GError *err) {
    if (!failed) {
        update_volume_includes(VOLUME_IN_EX_LOCATION_PAGER);
        for (guint i = volumes->len; i-- > 0; ) {
            wmudisks_volume *v = volume_index(i);
            if (v == (wmudisks_volume *)ret && v->include) {
                cur_index = i;
            }
        }
    }
    dock_action_callback(v, ret, data, failed, err);
}

static gboolean click_handler(GtkWidget *w, GdkEventButton *ev, gpointer d G_GNUC_UNUSED) {
    int x=ev->x, y=ev->y;
    wmudisks_volume *v;

    if(ev->button == 2){
        if(ev->type==GDK_BUTTON_PRESS && cur_index!=-1){
            cur_index = -1;
            redraw_dock();
        }
    } else if(ev->button == 3){
        v = get_volume_at(x, y, NULL);
        if(update_menu(v, cur_index==-1)){
            menu_volume = v;
            show_menu(w, ev);
        }
    }
    if(ev->button != 1) return FALSE;

    int button = 0;
    if(y >= 48 && y <= 59){
        if(x >= 4 && x <= 15) button = MOUNT_BUTTON;
        if(x >= 16 && x <= 25) button = EJECT_BUTTON;
        if(x >= 36 && x <= 47) button = LEFT_BUTTON;
        if(x >= 48 && x <= 59) button = RIGHT_BUTTON;
    }

    switch(ev->type){
      case GDK_BUTTON_PRESS:
        warn(DEBUG_DEBUG, "Button %d press at %d,%d", ev->button, x, y);
        if(button_down != button) redraw_dock();
        button_down = button;
        cancel_sgl_click();
        if(y >= 4 && y <= 44 && x >= 4 && x <= 60){
            schedule_sgl_click(get_volume_at(x, y, NULL));
        }
        break;

      case GDK_BUTTON_RELEASE:
        warn(DEBUG_DEBUG, "Button %d release at %d,%d", ev->button, x, y);
        if(button_down == button) switch(button){
          case MOUNT_BUTTON:
            mount_button_click();
            break;
          case EJECT_BUTTON:
            eject_button_click();
            break;
          case LEFT_BUTTON:
            left_button_click();
            break;
          case RIGHT_BUTTON:
            right_button_click();
            break;
        }
        if(button_down) redraw_dock();
        button_down = 0;
        break;

      case GDK_2BUTTON_PRESS:
        warn(DEBUG_DEBUG, "Button %d double-click at %d,%d", ev->button, x, y);
        cancel_sgl_click();
        if(dblclick_command_idx>=0 && y >= 4 && y <= 44 && x >= 4 && x <= 60){
            v = get_volume_at(x, y, NULL);
            if(!v) break;
            if(user_command_enabled(v,&menu_commands[dblclick_command_idx])){
                warn(DEBUG_DEBUG, "Executing user command \"%s\"", menu_commands[dblclick_command_idx].title);
                exec_command(v, &menu_commands[dblclick_command_idx]);
            } else {
                warn(DEBUG_DEBUG, "User command \"%s\" preconditions not met", menu_commands[dblclick_command_idx].title);
            }
        }
        break;

      default:
        break;
    }

    return FALSE;
}

void mount_button_click() {
    if (cur_index < 0) return;
    dock_action_mount(volume_index(cur_index));
}

void dock_action_mount(wmudisks_volume *v) {
    if (v->mountpoint) {
        wmudisks_unmount(v, dock_action_callback, "Unmount");
    } else if (wmudisks_can_mount(v)) {
        wmudisks_mount(v, dock_action_callback, "Mount");
    } else if (wmudisks_can_unlock(v)) {
        wmudisks_unlock(v, dock_lock_callback, "Unlock");
    }
}

void eject_button_click(){
    if(cur_index < 0) return;
    dock_action_eject(volume_index(cur_index));
}

void dock_action_eject(wmudisks_volume *v){
    if(wmudisks_can_lock(v)){
        wmudisks_lock(v, dock_lock_callback, "Lock");
    } else if(wmudisks_can_eject(v)){
        wmudisks_eject(v, dock_action_callback, "Eject");
    } else if(wmudisks_can_detach(v)){
        wmudisks_detach(v, dock_action_callback, "Detach");
    }
}

void left_button_click(){
    close_menu();
    update_volume_includes(VOLUME_IN_EX_LOCATION_PAGER);
    do {
        if(++cur_index >= (int)volumes->len) cur_index=-1;
    } while(cur_index>=0 && !volume_index(cur_index)->include);
}

void right_button_click(){
    close_menu();
    if(cur_index < 0) cur_index=volumes->len;
    update_volume_includes(VOLUME_IN_EX_LOCATION_PAGER);
    do {
        --cur_index;
    } while(cur_index>=0 && !volume_index(cur_index)->include);
}

void dock_action_goto(wmudisks_volume *v){
    update_volume_includes(VOLUME_IN_EX_LOCATION_PAGER);
    for (guint i = volumes->len; i-- > 0; ) {
        wmudisks_volume *v2 = volume_index(i);
        if(v2 == v && v2->include && cur_index != (int)i){
            cur_index = i;
            redraw_dock();
            return;
        }
    }
}

struct notify_actions *dock_actions_for_volume(wmudisks_volume *v){
    struct notify_actions *ret = g_slice_new(struct notify_actions);
    ret->volume = v;
    ret->mount_text = NULL;
    if (v->mountpoint) {
        ret->mount_text = "Unmount";
    } else if (wmudisks_can_mount(v)) {
        ret->mount_text = "Mount";
    } else if (wmudisks_can_unlock(v)) {
        ret->mount_text = "Unlock";
    }
    ret->eject_text = NULL;
    if(wmudisks_can_lock(v)){
        ret->eject_text = "Lock";
    } else if(wmudisks_can_eject(v)){
        ret->eject_text = "Eject";
    } else if(wmudisks_can_detach(v)){
        ret->eject_text = "Detach";
    }
    return ret;
}

static gboolean scroll_handler(GtkWidget *w G_GNUC_UNUSED, GdkEventScroll *ev, gpointer d G_GNUC_UNUSED) {
    int x = ev->x, y = ev->y;

    if (ev->type != GDK_SCROLL) {
        return FALSE;
    }
    warn(DEBUG_DEBUG, "Scroll %d at %d,%d", ev->direction, x, y);

    int delta = 0;
    switch (ev->direction) {
      case GDK_SCROLL_UP:
      case GDK_SCROLL_LEFT:
        delta = 1;
        break;
      case GDK_SCROLL_DOWN:
      case GDK_SCROLL_RIGHT:
        delta = -1;
        break;

      case GDK_SCROLL_SMOOTH:
        // Can't get here, no GDK_SMOOTH_SCROLL_MASK
        break;
    }
    if ( delta ) {
        close_menu();
        if (delta < 0 && cur_index < 0) {
            cur_index = volumes->len;
        }
        update_volume_includes(VOLUME_IN_EX_LOCATION_PAGER);
        do {
            cur_index += delta;
            if(cur_index >= (int)volumes->len) cur_index=-1;
        } while(cur_index>=0 && !volume_index(cur_index)->include);
        redraw_dock();
    }

    return FALSE;
}

static gboolean motion_handler(GtkWidget *w G_GNUC_UNUSED, GdkEventMotion *ev, gpointer d G_GNUC_UNUSED) {
    int old_hover_row = hover_row;
    hover_row = ((int)ev->y)/6 - 1;
    if (cur_index == -1 && old_hover_row != hover_row) {
        redraw_dock();
    }
    return FALSE;
}

static gboolean enter_handler(GtkWidget *w G_GNUC_UNUSED, GdkEventCrossing *ev, gpointer d G_GNUC_UNUSED) {
    return_to_fsm_counter = -1;
    int old_hover_row = hover_row;
    hover_row = ((int)ev->y)/6 - 1;
    if (cur_index == -1 && old_hover_row != hover_row) {
        redraw_dock();
    }
    return FALSE;
}

static gboolean leave_handler(GtkWidget *w G_GNUC_UNUSED, GdkEventCrossing *ev G_GNUC_UNUSED, gpointer d G_GNUC_UNUSED) {
    return_to_fsm_counter = return_to_fsm_timeout;
    int old_hover_row = hover_row;
    hover_row = -1;
    if (cur_index == -1 && old_hover_row != hover_row) {
        redraw_dock();
    }
    return FALSE;
}


/* Functions */

void redraw_dock(void) {
    gdk_window_invalidate_rect(gtk_widget_get_window(iconwin), NULL, FALSE);
    gdk_window_invalidate_rect(gtk_widget_get_window(mainwin), NULL, FALSE);
    gtk_tooltip_trigger_tooltip_query(display);
}

static void init_window(GtkWidget *win) {
    gtk_window_set_wmclass(GTK_WINDOW(win), g_get_prgname(), "DockApp");
    gtk_window_set_role(GTK_WINDOW(win), "DockApp");
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    gtk_widget_set_size_request(win, 64, 64);
    gtk_widget_set_app_paintable(win, TRUE);
    gtk_widget_add_events(win, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_EXPOSURE_MASK | GDK_SCROLL_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_HINT_MASK);
    g_object_set(win, "has-tooltip", TRUE, NULL);
    g_signal_connect(G_OBJECT(win), "draw", G_CALLBACK(draw_handler), NULL);
    g_signal_connect(G_OBJECT(win), "button-press-event", G_CALLBACK(click_handler), NULL);
    g_signal_connect(G_OBJECT(win), "button-release-event", G_CALLBACK(click_handler), NULL);
    g_signal_connect(G_OBJECT(win), "scroll-event", G_CALLBACK(scroll_handler), NULL);
    g_signal_connect(G_OBJECT(win), "enter-notify-event", G_CALLBACK(enter_handler), NULL);
    g_signal_connect(G_OBJECT(win), "leave-notify-event", G_CALLBACK(leave_handler), NULL);
    g_signal_connect(G_OBJECT(win), "motion-notify-event", G_CALLBACK(motion_handler), NULL);
    g_signal_connect(G_OBJECT(win), "query-tooltip", G_CALLBACK(query_tooltip_handler), NULL);
    g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
}

void create_dock_icon(void) {
    warn(DEBUG_DEBUG, "Loading theme");
    theme = gtk_icon_theme_get_default();
    display = gdk_display_get_default();

    warn(DEBUG_DEBUG, "Loading pixbufs");
    button_images_pixbuf = gdk_pixbuf_new_from_xpm_data((const char **)button_images_xpm);
    if (!button_images_pixbuf) {
        die("Could not load button image pixbuf");
    }
    if (!init_chars(4,
            0,0,0, 0x15,0x8F,0xBC,
            0,0,0, 0x75,0x75,0x75,
            0,0,0, 0x1c,0xc2,0xff,
            0,0,0, 0xff,0x00,0x00
    )) {
        die("Could not load character pixmaps");
    }

    warn(DEBUG_INFO, "Creating gradients");
    make_gradients();

    warn(DEBUG_INFO, "Creating window");
    mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (nonwmaker) {
        iconwin = mainwin;
    } else {
        iconwin = gtk_window_new(GTK_WINDOW_POPUP);
    }
    if (mainwin == NULL || iconwin == NULL) {
        die("Couldn't create windows");
    }

    init_window(mainwin);
    if (!nonwmaker) {
        init_window(iconwin);
    }

    warn(DEBUG_DEBUG, "Realizing window");
    gtk_widget_realize(mainwin);
    gtk_widget_realize(iconwin);

    // GTK3 removed gdk_window_set_icon, sigh. 
    // https://bugzilla.gnome.org/show_bug.cgi?id=723437
    // So we need to set the icon window manually. But if we do it before
    // gtk_widget_show(), GTK will override it. So, return of the GTK hack:
    // reparent it to an unmapped window, show it, fix the hints, and then
    // reparent back to root. Sigh.
    /* HACK */ {
        Display *d = GDK_DISPLAY_XDISPLAY(display);
        Window mw = GDK_WINDOW_XID(gtk_widget_get_window(mainwin));
        Window iw = GDK_WINDOW_XID(gtk_widget_get_window(iconwin));
        Window p, dummy1, *dummy2;
        unsigned int dummy3;
        XQueryTree(d, mw, &dummy1, &p, &dummy2, &dummy3);
        if (dummy2) XFree(dummy2);
        Window w = XCreateSimpleWindow(d, p, 0, 0, 1, 1, 0, 0, 0);
        XReparentWindow(d, mw, w, 0, 0);
        gtk_widget_show(mainwin);
        gtk_widget_show(iconwin);
        XWMHints *wmHints = XGetWMHints(d, mw);
        if (!wmHints) {
            wmHints = XAllocWMHints();
        }
        wmHints->flags |= IconWindowHint;
        wmHints->icon_window = iw;
        XSetWMHints(d, mw, wmHints);
        XFree(wmHints);
        XReparentWindow(d, mw, p, 0, 0);
        XDestroyWindow(d, w);
    }

    warn(DEBUG_DEBUG, "Created window mainwin=%p (0x%lx) iconwin=%p (0x%lx)", gtk_widget_get_window(mainwin), GDK_WINDOW_XID(gtk_widget_get_window(mainwin)), gtk_widget_get_window(iconwin), GDK_WINDOW_XID(gtk_widget_get_window(iconwin)));

    g_timeout_add_seconds(1, timeout_cb, NULL);
}

/* Device notifications */

void volume_added(wmudisks_volume *v, gboolean connecting) {
    if (!connecting) {
        struct notify_actions *actions = dock_actions_for_volume(v);
        notify(NOTIFY_INFO, "device.added", TRUE, v->icon, actions, "Device %s added", NULL, wmudisks_get_display_name(v));
        g_slice_free(struct notify_actions, actions);
    }
    if (cur_index == -1) {
        redraw_dock();
    }
}

void volume_updated(wmudisks_volume *v) {
    if(cur_index >= 0 && volume_index(cur_index) == v) {
        update_volume_includes(VOLUME_IN_EX_LOCATION_PAGER);
        if (!v->include) {
            cur_index = -1;
            if (menu_volume == v) {
                close_menu();
            }
        }
        redraw_dock();
    } else if(cur_index == -1) {
        redraw_dock();
    }
    if (menu_volume == v) {
        update_menu(menu_volume, cur_index == -1);
    }
}

void volume_removed(int idx, wmudisks_volume *v, gboolean disconnecting) {
    if(!disconnecting){
        notify(NOTIFY_INFO, "device.removed", TRUE, v->icon, NULL, "Device %s removed", NULL, wmudisks_get_display_name(v));
    }
    if (cur_index == idx || cur_index == -1) {
        cur_index = -1;
        if (menu_volume == v) {
            close_menu();
        }
        redraw_dock();
    } else if (cur_index > idx) {
        cur_index--;
    }
}

void wmudisks_connection_changed(const char *wmudisks_version G_GNUC_UNUSED) {
    redraw_dock();
}
