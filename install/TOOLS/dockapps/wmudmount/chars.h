#ifndef CHARS_H
#define CHARS_H

#include <gtk/gtk.h>

extern const int char_height;

/* Returns TRUE if the init succeeded, or FALSE otherwise. */
gboolean init_chars(int num_colors, ...);

/* Draws the specified character at (x,y) to the specified cairo context.
 * Returns the width of the character drawn. */
int draw_char(cairo_t *cr, char c, int x, int y, int color);

/* Draws the specified string at (x,y) to the specified cairo context. Returns
 * the width of the string drawn. */
int draw_string(cairo_t *cr, const char *s, int x, int y, int color);

/* Returns the width of the string that would be drawn. */
int measure_string(const char *s);

#endif
