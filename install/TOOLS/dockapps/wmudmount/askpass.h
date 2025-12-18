#ifndef ASKPASS_H
#define ASKPASS_H

#include <gtk/gtk.h>
#include "wmudisks.h"

extern gboolean allow_core_files;
extern gboolean allow_insecure_memory;
extern gboolean use_keyring;

void init_keyring();

char *load_pass_from_keyring(wmudisks_volume *v);
void forget_pass_from_keyring(wmudisks_volume *v);
char *ask_for_pass(wmudisks_volume *v, GtkWidget *transient_for, GError **error);
void free_pass(char *pass);

#endif
