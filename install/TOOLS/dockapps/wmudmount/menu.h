#ifndef MENU_H
#define MENU_H

#include "wmudisks.h"

void init_menu();
gboolean update_menu(wmudisks_volume *v, gboolean do_name);
void show_menu(GtkWidget *w, GdkEventButton *ev);
void close_menu();

#endif
