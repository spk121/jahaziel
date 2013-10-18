#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "proc.h"
#include "ui.h"

#define UI_FILE "jahaziel.glade"

JahUI      *ui;

void ui_init(void)
{
    GtkBuilder *builder;
    GError     *error = NULL;

    builder = gtk_builder_new();
    if(!gtk_builder_add_from_file(builder, UI_FILE, &error))
    {
        g_warning("%s", error->message);
        exit(1);
    }
    ui = g_malloc0_n(1, sizeof(JahUI));
    
    /* Get UI pointers from the Glade file */
    JAH_GET_WIDGET(builder, main_window, ui);
    JAH_GET_WIDGET(builder, connect_dialog, ui);
    JAH_GET_OBJECT(builder, connect_server_combobox_entry, GTK_ENTRY, ui);
    JAH_GET_OBJECT(builder, connect_username_combobox_entry, GTK_ENTRY, ui);

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));

    gtk_widget_show(ui->main_window);
}

void ui_fini()
{
    g_free(ui);
}

G_MODULE_EXPORT
void ui_cb_connect_button_clicked(GtkButton *button, gpointer user_data)
{
    // Clicking this button brings up a dialog that allows one to
    // choose a broker.
    g_warning("connect button is clicked");
    gtk_widget_show(ui->connect_dialog);
}

G_MODULE_EXPORT
void ui_cb_connect_connect_button_clicked(GtkButton *button, gpointer user_data)
{
    g_debug("Connect dialog's connect button is clicked");
    gtk_widget_hide(ui->connect_dialog);

    // Scrape the user-entered 
    gchar *server = gtk_editable_get_chars(GTK_EDITABLE(ui->connect_server_combobox_entry), 0, -1);
    gchar *username = gtk_editable_get_chars(GTK_EDITABLE(ui->connect_username_combobox_entry), 0, -1);

    // FIXME: need to scrub user-entered data here 
    pm_queue_proc(proc_connect_new(g_strdup(server), g_strdup(username)));

    g_free(server);
    g_free(username);
    // the given username: add a connection task to the idle process manager.
}

G_MODULE_EXPORT
void ui_cb_connect_cancel_button_clicked(GtkButton *button, gpointer user_data)
{
    // Clicking this button brings up a dialog that allows one to
    // choose a broker.
    g_warning("Connect dialog's cancel button is clicked");
    gtk_widget_hide(ui->connect_dialog);

    // Now, asynchronously try to cleanly disconnect from the given
    // host with the given username: add a disconnection task to the
    // idle process manager.
}

static void server_or_username_combobox_entry_changed (GtkEditable *editable, gpointer user_data)
{
    GtkButton *connect_button = user_data;

    gchar *server = gtk_editable_get_chars(GTK_EDITABLE(ui->connect_server_combobox_entry), 0, -1);
    gchar *username = gtk_editable_get_chars(GTK_EDITABLE(ui->connect_username_combobox_entry), 0, -1);

    if (strlen(server) > 0 && strlen(username) > 0)
    {
        if (gtk_widget_is_sensitive(GTK_WIDGET(connect_button)) == 0)
            gtk_widget_set_sensitive(GTK_WIDGET(connect_button), 1);
    }
    else if (gtk_widget_is_sensitive(GTK_WIDGET(connect_button)) == 1)
        gtk_widget_set_sensitive(GTK_WIDGET(connect_button), 0);
    g_free(server);
    g_free(username);
}


G_MODULE_EXPORT
void ui_cb_connect_server_combobox_entry_changed (GtkEditable *editable, gpointer user_data)
{
    server_or_username_combobox_entry_changed(editable, user_data);
}

void ui_cb_connect_username_combobox_entry_changed (GtkEditable *editable, gpointer user_data)
{
    server_or_username_combobox_entry_changed(editable, user_data);    
}
