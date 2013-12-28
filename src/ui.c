#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "proc.h"
#include "ui.h"
#include "network.h"
#include "calllistview.h"
#include "raii.h"
#include "xglib.h"
#include "xgtk.h"

#define UI_FILE "jahaziel.glade"

JahUI *ui;
Store *store;

void ui_init(void)
{
    RAII_VARIABLE(GtkBuilder *, builder, gtk_builder_new(), xgtk_builder_object_unref);
    GError     *error = NULL;

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
    JAH_GET_OBJECT(builder, statusbar, GTK_STATUSBAR, ui);
    JAH_GET_OBJECT(builder, worker_list_store, GTK_LIST_STORE, ui);
    JAH_GET_OBJECT(builder, call_client_list_view, GTK_TREE_VIEW, ui);

    gtk_builder_connect_signals(builder, NULL);

    gtk_widget_show(ui->main_window);
    
    store = create_store();
    create_view(store, ui->call_client_list_view);
    gtk_widget_show(GTK_WIDGET(ui->call_client_list_view));
}

void ui_fini()
{
    g_free(ui);
}

G_MODULE_EXPORT
void ui_cb_connect_button_clicked(GtkButton *button, gpointer connect_dialog_box)
{
    // Clicking this button brings up a dialog that allows one to
    // choose a broker.
    g_debug("connect button is clicked");
    gtk_widget_show(GTK_WIDGET(connect_dialog_box));
}

G_MODULE_EXPORT
void ui_cb_connect_connect_button_clicked(GtkButton *button, gpointer connect_dialog_box)
{
    RAII_VARIABLE(gchar *, server,
                  gtk_editable_get_chars(GTK_EDITABLE(ui->connect_server_combobox_entry), 0, -1),
                  xg_free_char_pointer);
    RAII_VARIABLE(gchar *, username,
                  gtk_editable_get_chars(GTK_EDITABLE(ui->connect_username_combobox_entry), 0, -1),
                  xg_free_char_pointer);

    g_debug("Connect dialog's connect button is clicked");
    gtk_widget_hide(GTK_WIDGET(connect_dialog_box));

    // FIXME: need to scrub user-entered data here 
    pm_queue_proc(proc_connect_new(g_strdup(server), g_strdup(username)));
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

G_MODULE_EXPORT
void ui_cb_call_button_clicked(GtkButton *button, gpointer call_dialog_box)
{
    // Clicking this button brings up a dialog that allows one to
    // choose a broker.
    g_warning("call button is clicked");

    pm_queue_proc(proc_directory_new());
    gtk_widget_show(GTK_WIDGET(call_dialog_box));
}

G_MODULE_EXPORT
void ui_cb_call_cancel_button_clicked(GtkButton *button, gpointer call_dialog_box)
{
    g_warning("Call dialog's cancel button is clicked");
    gtk_widget_hide(GTK_WIDGET(call_dialog_box));
}

G_MODULE_EXPORT
void ui_cb_call_call_button_clicked(GtkButton *button, gpointer call_dialog_box)
{
    g_warning("Call dialog's call button is clicked");
    gtk_widget_hide(GTK_WIDGET(call_dialog_box));

    // Scrape the user-entered 
    // gchar *server = gtk_editable_get_chars(GTK_EDITABLE(ui->connect_server_combobox_entry), 0, -1);
    // gchar *username = gtk_editable_get_chars(GTK_EDITABLE(ui->connect_username_combobox_entry), 0, -1);

    // FIXME: need to scrub user-entered data here 
    pm_queue_proc(proc_call_request_new("ECHO"));

    // g_free(server);
    // g_free(username);
}
