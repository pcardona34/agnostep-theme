#ifndef MISC_H
#define MISC_H

#include <glib.h>

#define APP_GENERIC_ERROR app_generic_error_quark()
#define APP_NO_ERROR app_no_error_quark()

GQuark app_generic_error_quark(void) G_GNUC_CONST;
GQuark app_no_error_quark(void) G_GNUC_CONST;

char *format_byte_size(guint64 size);

#endif
