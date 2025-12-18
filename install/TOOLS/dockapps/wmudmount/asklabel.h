#ifndef ASKLABEL_H
#define ASKLABEL_H

#include <gtk/gtk.h>
#include "wmudisks.h"

char *ask_for_label(wmudisks_volume *v, GtkWidget *transient_for);

#endif
