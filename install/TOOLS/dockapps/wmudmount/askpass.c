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
#include "misc.h"

#include <stdlib.h>

#ifdef HAVE_PRCTL
#include <sys/prctl.h>
#else
#include <sys/resources.h>
#endif

#include <glib.h>

#ifdef HAVE_GCR
#define GCR_API_SUBJECT_TO_CHANGE 1
#include <gcr/gcr.h>
#endif

#ifdef HAVE_SECRET
#include <libsecret/secret.h>

static SecretSchema schema = {
    "net.sourceforge.wmudmount", SECRET_SCHEMA_NONE,
    {
        { "device-type", SECRET_SCHEMA_ATTRIBUTE_STRING },
        { "device-uuid", SECRET_SCHEMA_ATTRIBUTE_STRING },
        { NULL, 0 }
    }
};
#endif

#include "die.h"
#include "askpass.h"

gboolean allow_core_files = FALSE;
gboolean allow_insecure_memory = FALSE;
gboolean use_keyring = TRUE;

/****************** Option parsing ******************/

void init_keyring() {
    warn(DEBUG_DEBUG, "Disabling core files...");
    errno=0;
#ifdef HAVE_PRCTL
    if (prctl(PR_SET_DUMPABLE, 0)) {
#else
    struct rlimit r;
    r.rlim_cur=r.rlim_max=0;
    if (setrlimit(RLIMIT_CORE, &r)) {
#endif
        if (allow_core_files) {
            die("Could not disable core files: %s", strerror(errno));
        } else {
            warn(DEBUG_WARN, "Could not disable core files: %s", strerror(errno));
        }
    } else {
        warn(DEBUG_INFO, "Core files disabled");
    }

    if (!allow_insecure_memory) {
        warn(DEBUG_DEBUG, "Checking whether secure memory can be allocated...");
#ifdef HAVE_GCR
        gpointer p = gcr_secure_memory_try_realloc(NULL, 1024);
        if (!p) {
            die("Could not allocate 1024 bytes of secure memory (as a test)");
        }
        warn(DEBUG_INFO, "Allocated at %p", p);
        gcr_secure_memory_free(p);
        warn(DEBUG_INFO, "Secure memory can be allocated");
#else
        die("Support for secure memory via gcr was not compiled in. Specify\n--allow-insecure-memory on the command line to proceed.");
#endif
    }

    if (use_keyring) {
#ifndef HAVE_SECRET
        warn(DEBUG_INFO, "Secret service support was not compiled in, crypto device passwords will NOT be saved");
        use_keyring = FALSE;
#else
        warn(DEBUG_INFO, "Secret service will be used for crypto device passwords");
#endif
    } else {
        warn(DEBUG_INFO, "Crypto device passwords will NOT be saved");
    }
}

#ifdef HAVE_SECRET
static void save_pass_to_keyring_cb(GObject *obj G_GNUC_UNUSED, GAsyncResult *res, gpointer name) {
    GError *err = NULL;

    secret_password_store_finish(res, &err);
    if (err) {
        warn(DEBUG_ERROR, "Could not save %s: %s", (char *)name, err->message);
    }
    g_clear_error(&err);
    g_free(name);
}

static void save_pass_to_keyring(wmudisks_volume *v, gboolean session, const char *pass) {
    char *name = g_strdup_printf("%s crypto device passphrase for UUID %s", v->crypto_device_type, v->crypto_device_uuid);
    if (use_keyring) {
        secret_password_store(
            &schema, session ? SECRET_COLLECTION_SESSION : SECRET_COLLECTION_DEFAULT, name, pass, NULL, save_pass_to_keyring_cb, name,
            "device-type", v->crypto_device_type,
            "device-uuid", v->crypto_device_uuid,
            NULL
        );
    } else {
        warn(DEBUG_ERROR, "Could not save %s: Secrets service is not available", (char *)name);
    }
}

static void forget_pass_from_keyring_cb(GObject *obj G_GNUC_UNUSED, GAsyncResult *res, gpointer data) {
    wmudisks_volume *v = (wmudisks_volume *)data;
    GError *err = NULL;

    secret_password_clear_finish(res, &err);
    if (err) {
        warn(DEBUG_ERROR, "Could not delete %s crypto device passphrase for UUID %s: %s", v->crypto_device_type, v->crypto_device_uuid, err->message);
    }
    g_clear_error(&err);
    wmudisks_unref(v);
}

void forget_pass_from_keyring(wmudisks_volume *v) {
    if (use_keyring) {
        wmudisks_ref(v);
        secret_password_clear(&schema, NULL, forget_pass_from_keyring_cb, v,
            "device-type", v->crypto_device_type,
            "device-uuid", v->crypto_device_uuid,
            NULL
        );
    }
}

struct load_pass_from_keyring_data {
    wmudisks_volume *v;
    char *pass;
    int done;
};

static void load_pass_from_keyring_cb(GObject *obj G_GNUC_UNUSED, GAsyncResult *res, gpointer d) {
    struct load_pass_from_keyring_data *data = (struct load_pass_from_keyring_data *)d;
    GError *err = NULL;
    data->pass = secret_password_lookup_nonpageable_finish(res, &err);
    if (err) {
        warn(DEBUG_ERROR, "Could not load %s crypto device passphrase for UUID %s: %s", data->v->crypto_device_type, data->v->crypto_device_uuid, err->message);
    }
    g_clear_error(&err);
    data->done = 1;
}

char *load_pass_from_keyring(wmudisks_volume *v) {
    struct load_pass_from_keyring_data data = { v, NULL, 0 };

    if (use_keyring) {
        secret_password_lookup(&schema, NULL, load_pass_from_keyring_cb, &data,
            "device-type", v->crypto_device_type,
            "device-uuid", v->crypto_device_uuid,
            NULL
        );

        wmudisks_ref(v);
        while (!data.done) {
            gtk_main_iteration();
        }
        wmudisks_unref(v);
    }

    return data.pass;
}

void free_pass(char *pass) {
    secret_password_free(pass);
}

#else
static void save_pass_to_keyring(wmudisks_volume *v, gboolean session, const char *pass) {
    char *name = g_strdup_printf("%s crypto device passphrase for UUID %s", v->crypto_device_type, v->crypto_device_uuid);
    warn(DEBUG_ERROR, "Could not save %s: Secrets service is not available", (char *)name);
}

void forget_pass_from_keyring(wmudisks_volume *v) {
}

char *load_pass_from_keyring(wmudisks_volume *v) {
    return NULL;
}

void free_pass(char *pass) {
    free(pass);
}
#endif

char *ask_for_pass(wmudisks_volume *v, GtkWidget *transient_for, GError **err) {
    g_warn_if_fail(err == NULL || *err == NULL);

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Unlock device",
        GTK_WINDOW(transient_for),
        GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Unlock", GTK_RESPONSE_ACCEPT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );
    gtk_dialog_set_default_response((GtkDialog*)dialog, GTK_RESPONSE_ACCEPT);
    gtk_window_set_icon_name((GtkWindow*)dialog, "dialog-password");

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

    label = gtk_label_new("This device is password protected. To make it available for use, enter the passphrase here.");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign((GtkLabel*)label, 0.0);
    gtk_label_set_yalign((GtkLabel*)label, 0.5);
    gtk_label_set_line_wrap((GtkLabel*)label, TRUE);
    gtk_box_pack_start(content, label, FALSE, FALSE, 0);

    hbox = (GtkBox*)gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(content, (GtkWidget*)hbox, FALSE, FALSE, 0);
    label = gtk_label_new_with_mnemonic("_Passphrase:");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
    gtk_label_set_xalign((GtkLabel*)label, 1.0);
    gtk_label_set_yalign((GtkLabel*)label, 0.5);
    gtk_box_pack_start(hbox, label, FALSE, FALSE, 0);
    GtkEntryBuffer *entrybuf;
#ifdef HAVE_GCR
    entrybuf = gcr_secure_entry_buffer_new();
#else
    entrybuf = gtk_entry_buffer_new(NULL, -1);
#endif
    GtkWidget *entry = gtk_entry_new_with_buffer(entrybuf);
    gtk_entry_set_visibility((GtkEntry*)entry, FALSE);
    gtk_entry_set_activates_default((GtkEntry*)entry, TRUE);
    gtk_box_pack_start(hbox, entry, TRUE, TRUE, 0);
    gtk_label_set_mnemonic_widget((GtkLabel*)label, entry);

    GtkWidget *rb_forget = gtk_radio_button_new_with_mnemonic(NULL, "_Forget passphrase immediately");
    GtkWidget *rb_remember = gtk_radio_button_new_with_mnemonic_from_widget((GtkRadioButton*)rb_forget, "_Remember passphrase");
    GtkWidget *rb_session = gtk_radio_button_new_with_mnemonic_from_widget((GtkRadioButton*)rb_forget, "Remember passphrase until _logout");

    gtk_toggle_button_set_active((GtkToggleButton*)rb_forget,TRUE);
    gtk_widget_set_sensitive(rb_remember, use_keyring);
    gtk_widget_set_sensitive(rb_session, use_keyring);

    gtk_box_pack_start(content, rb_forget, FALSE, FALSE, 0);
    gtk_box_pack_start(content, rb_remember, FALSE, FALSE, 0);
    gtk_box_pack_start(content, rb_session, FALSE, FALSE, 0);

    gtk_widget_show_all((GtkWidget*)content);
    int response = gtk_dialog_run((GtkDialog*)dialog);
    char *pass = NULL;
    switch (response) {
      case GTK_RESPONSE_ACCEPT:
        pass = (char *)gtk_entry_get_text((GtkEntry*)entry);
        if (gtk_toggle_button_get_active((GtkToggleButton*)rb_remember)) {
            save_pass_to_keyring(v, FALSE, pass);
        } else if (gtk_toggle_button_get_active((GtkToggleButton*)rb_session)) {
            save_pass_to_keyring(v, TRUE, pass);
        } else {
            forget_pass_from_keyring(v);
        }
        break;
      case GTK_RESPONSE_CANCEL:
        g_set_error(err, APP_GENERIC_ERROR, 0, "Passphrase request cancelled by user");
        break;
      default:
        g_set_error(err, APP_GENERIC_ERROR, 0, "Passphrase request failed");
        break;
    }
    gtk_widget_destroy(dialog);
    return pass;
}
