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
#include "chars.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Normally, the colors in an xpm would be stored as string literals, which
 * may be stored in read-only memory. In order to change the colors at runtime,
 * we need to store them in writable memory instead. We pack them into one big
 * buffer here to save a few bytes and make the color-setting loop simpler. */
static char colorbuf[252] = " \tc #xxxxxx\0.\tc #xxxxxx\0+\tc #xxxxxx\0@\tc #xxxxxx\0#\tc #xxxxxx\0$\tc #xxxxxx\0%\tc #xxxxxx\0&\tc #xxxxxx\0*\tc #xxxxxx\0=\tc #xxxxxx\0-\tc #xxxxxx\0;\tc #xxxxxx\0>\tc #xxxxxx\0,\tc #xxxxxx\0'\tc #xxxxxx\0)\tc #xxxxxx\0!\tc #xxxxxx\0~\tc #xxxxxx\0{\tc #xxxxxx\0]\tc #xxxxxx\0^\tc #xxxxxx";

/* These are the mask values for each color. */
static guint8 mask[] = {0,255,109,88,253,238,94,126,191,219,150,234,167,249,174,103,227,244,141,69,124};

/* This xpm contains variable-width graphics for ASCII printablecharacters
 * plus a "replacement" character vaguely reminiscent of U+FFFD. */
/* XPM */
static char * chars_mask_xpm[] = {
"278 5 21 1",
colorbuf+0, /* Point colors to writable memory declared above */
colorbuf+12,
colorbuf+24,
colorbuf+36,
colorbuf+48,
colorbuf+60,
colorbuf+72,
colorbuf+84,
colorbuf+96,
colorbuf+108,
colorbuf+120,
colorbuf+132,
colorbuf+144,
colorbuf+156,
colorbuf+168,
colorbuf+180,
colorbuf+192,
colorbuf+204,
colorbuf+216,
colorbuf+228,
colorbuf+240,
"  .. . . .+..@. . .  . ..               . . .. .. .. . .... . ... .  .      .   .  .#. ... . ..  .... ...... .. . ....  .. ..  .  ..  ......  .. ..  ...... .. ..  .. .. .......  .. .    .    .       .     .   .  .  ..  ..                       .                     ...  . .....",
"  .. .....$.%   .. . ..  . .  .         .. . .   .  .. ..  .    .. .. .. . . ... .   ..  .. .. ..  . ..  .  .   . . .   .. ..  ...... .. .. ..  .. ..   . . .. ..  .. .. .  .. .   .. .    . ...   ..  . .& .  * .      .   . .. ..  . ..  .. .. ...... .. ..  .. .. .... ... . . .  .",
"  .    ..  =.- .  .   .  .......  ...  . . . .  .  . ..... ..   . .  ..   .       . ; . ....... .  . ... .. . ..... .   ... .  .>>.. ... ... .  ...  .  . . .. ..>>. .  .  . .  .  .        . ... .   ...&...., ... .  .. . ..>>.. .. .. .. ..  .>  . . .. ..  . .  .. >..   .    .. .",
"      ....'..).  . .  .  . .  .  .    .  . . . .    .  .  .. . . . .  .. . . ... .    .   . .. ..  . ..  .  .  .. . . . .. ..  .  ..  .. ..  . ... .  . . . .. ...... . . .  .   . .        . .. ..  . ..   .  !~. ... ...  ..  .. .. ...  ...   >. . . .. ..>>. .   ..>  ...     . ..",
"  .   . .  {] . ... .  .. . .   .    ..   . ........   ...  .  .  .  .  .   .   .   .  .... ...  .... ....   .. . .... . . .....  ..  .....   .... ...  . ... . .  .. . . .....  ...   ...   ....  .. .. .. . .~^. .. . . . ..  .. . . .    ..  ..  . ... .  .. . . . ... ...     ...."};

/* Width of each character. */
static int widths[] = {2,1,3,4,4,3,4,1,2,2,3,3,2,3,1,3,3,3,3,3,3,3,3,3,3,3,1,2,3,3,3,3,4,3,3,3,3,3,3,4,3,3,3,3,3,4,4,3,3,4,3,3,3,3,3,4,3,3,3,2,3,2,3,3,2,3,3,3,3,3,3,3,3,1,3,3,2,4,3,3,3,3,3,3,3,3,3,4,3,3,3,2,1,2,4,4};

/* Starting X position of each character. */
static int starts[] = {0,2,3,6,10,14,17,21,22,24,26,29,32,34,37,38,41,44,47,50,53,56,59,62,65,68,71,72,74,77,80,83,86,90,93,96,99,102,105,108,112,115,118,121,124,127,131,135,138,141,145,148,151,154,157,160,164,167,170,173,175,178,180,183,186,188,191,194,197,200,203,206,209,212,213,216,219,221,225,228,231,234,237,240,243,246,249,252,256,259,262,265,267,268,270,274};

/* Height of all characters. */
const int char_height = 5;

static GdkPixbuf **charpix = NULL;

gboolean init_chars(int numcolors, ...) {
    gboolean ret = TRUE;
    va_list ap;

    charpix = g_malloc(numcolors * sizeof(GdkPixbuf *));
    va_start(ap, numcolors);
    for (int i=0; i<numcolors; i++) {
        /* Standard color mixing algorithm. We just sprintf the RGB spec into
         * the right positions in the color buffer. */
        int r1 = va_arg(ap,int), g1 = va_arg(ap,int), b1 = va_arg(ap,int);
        int r2 = va_arg(ap,int) - r1, g2 = va_arg(ap,int) - g1, b2 = va_arg(ap,int) - b1;
        for (size_t j = 0, o = 5; j < sizeof(mask); j++, o += 12) {
            sprintf(colorbuf + o, "%02x%02x%02x",
                (guint8)(r1 + r2 * mask[j] / 255),
                (guint8)(g1 + g2 * mask[j] / 255),
                (guint8)(b1 + b2 * mask[j] / 255)
            );
        }
        charpix[i] = gdk_pixbuf_new_from_xpm_data((const char **)chars_mask_xpm);
        if(charpix[i] == NULL) ret = FALSE;
    }
    va_end(ap);
    return ret;
}

gboolean chars_change_color(int color, int bg_r, int bg_g, int bg_b, int fg_r, int fg_g, int fg_b){
    g_object_unref(charpix[color]);
    int r2 = fg_r-bg_r, g2 = fg_g-bg_g, b2 = fg_b-bg_b;
    for (size_t j = 0, o = 5; j < sizeof(mask); j++, o += 12) {
        sprintf(colorbuf + o, "%02x%02x%02x",
            (guint8)(bg_r + r2 * mask[j] / 255),
            (guint8)(bg_g + g2 * mask[j] / 255),
            (guint8)(bg_b + b2 * mask[j] / 255)
        );
    }
    charpix[color] = gdk_pixbuf_new_from_xpm_data((const char **)chars_mask_xpm);
    return charpix[color] != NULL;
}

int draw_char(cairo_t *cr, char c, int x, int y, int color) {
    if (c < 0x20 || c > 0x7e) {
        c=0x7f;
    }
    c -= 0x20;
    int i = c;
    int w = widths[i];

    cairo_save(cr);
    cairo_rectangle(cr, x, y, w, 5);
    cairo_clip(cr);
    gdk_cairo_set_source_pixbuf(cr, charpix[color], x - starts[i], y);
    cairo_paint(cr);
    cairo_restore(cr);
    return w;
}

int draw_string(cairo_t *cr, const char *s, int x, int y, int color) {
    if (!*s) {
        return 0;
    }
    int w = 0;
    for (; *s; s++) {
        w += draw_char(cr, *s, x+w, y, color) + 1;
    }
    return w-1;
}

int measure_string(const char *s) {
    if (!*s) {
        return 0;
    }
    int w = 0;
    for (; *s; s++) {
        char c = *s;
        if (c < 0x20 || c > 0x7e) {
            c=0x7f;
        }
        w += widths[(int)c - 0x20] + 1;
    }
    return w-1;
}
