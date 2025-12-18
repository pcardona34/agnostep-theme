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

#include "die.h"
#include "asklabel.h"

char *ask_for_label(wmudisks_volume *v, GtkWidget *transient_for) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Relabel device",
        GTK_WINDOW(transient_for),
        GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Relabel", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL
   );
    gtk_dialog_set_default_response((GtkDialog*)dialog, GTK_RESPONSE_ACCEPT);

    GtkBox *content = (GtkBox*)gtk_dialog_get_content_area((GtkDialog*)dialog);

    GtkBox *hbox = (GtkBox*)gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(content, (GtkWidget*)hbox, FALSE, FALSE, 10);
    GtkWidget *image = gtk_image_new_from_gicon(v->icon, GTK_ICON_SIZE_DIALOG);
    gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(image, GTK_ALIGN_START);
    gtk_box_pack_start(hbox, image, FALSE, FALSE, 0);
    GtkWidget *label = gtk_label_new(NULL);
    char *txt = g_markup_printf_escaped("<b><big>%s</big></b>", v->desc);
    gtk_label_set_markup((GtkLabel*)label, txt);
    g_free(txt);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign((GtkLabel*)label, 0.0);
    gtk_label_set_yalign((GtkLabel*)label, 0.5);
    gtk_label_set_line_wrap((GtkLabel*)label, TRUE);
    gtk_box_pack_start(hbox, label, FALSE, FALSE, 0);

    txt = g_strdup_printf(
        "To relabel %s, enter the label here.",
        v->desc
    );
    label = gtk_label_new(txt);
    g_free(txt);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign((GtkLabel*)label, 0.0);
    gtk_label_set_yalign((GtkLabel*)label, 0.5);
    gtk_label_set_line_wrap((GtkLabel*)label, TRUE);
    gtk_box_pack_start(content, label, FALSE, FALSE, 0);

    hbox = (GtkBox*)gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(content, (GtkWidget*)hbox, FALSE, FALSE, 0);
    label = gtk_label_new_with_mnemonic("_Label:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign((GtkLabel*)label, 1.0);
    gtk_label_set_yalign((GtkLabel*)label, 0.5);
    gtk_box_pack_start(hbox, label, FALSE, FALSE, 0);
    GtkWidget *entry = gtk_entry_new();
    if (v->label) {
        gtk_entry_set_text((GtkEntry*)entry, v->label);
        gtk_editable_select_region((GtkEditable*)entry, 0, -1);
    }
    gtk_entry_set_activates_default((GtkEntry*)entry, TRUE);
    gtk_box_pack_start(hbox, entry, TRUE, TRUE, 0);
    gtk_label_set_mnemonic_widget((GtkLabel*)label, entry);

    gtk_widget_show_all((GtkWidget*)content);
    int response = gtk_dialog_run((GtkDialog*)dialog);
    char *newlabel = NULL;
    switch (response) {
      case GTK_RESPONSE_ACCEPT:
        newlabel = g_strdup(gtk_entry_get_text((GtkEntry*)entry));
        break;
    }
    gtk_widget_destroy(dialog);
    return newlabel;
}
