#include <gtk/gtk.h>
#include <stdlib.h>
#include "ui.h"

#define UI_FILE "../data/jahaziel.glade"

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
    // Clicking this button brings up a dialog that allows one to
    // choose a broker.
    g_warning("Connect dialog's connect button is clicked");
    gtk_widget_hide(ui->connect_dialog);

    // 
}

G_MODULE_EXPORT
void ui_cb_connect_cancel_button_clicked(GtkButton *button, gpointer user_data)
{
    // Clicking this button brings up a dialog that allows one to
    // choose a broker.
    g_warning("Connect dialog's cancel button is clicked");
    gtk_widget_hide(ui->connect_dialog);
}

