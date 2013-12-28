#include "application.h"
#include "main_window.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include "ui.h"

G_DEFINE_TYPE(MainWindow, main_window, GTK_TYPE_APPLICATION_WINDOW);

#define UI_FILE "jahaziel.glade"

static void
main_window_init (MainWindow *app)
{
    GtkBuilder *builder;
    GError     *error = NULL;
    GtkBox *main_window_outer_vbox;

    builder = gtk_builder_new();
    if(!gtk_builder_add_from_file(builder, UI_FILE, &error))
    {
        g_warning("%s", error->message);
        exit(1);
    }

    main_window_outer_vbox = GTK_BOX(gtk_builder_get_object(builder, "main_window_outer_vbox"));
    gtk_widget_reparent(GTK_WIDGET(main_window_outer_vbox), GTK_WIDGET(app));

    ui->connect_dialog = GTK_WIDGET(gtk_builder_get_object(builder,"connect_dialog"));
    ui->connect_server_combobox_entry = GTK_ENTRY(gtk_builder_get_object(builder,"connect_server_combobox_entry"));
    ui->connect_username_combobox_entry = GTK_ENTRY(gtk_builder_get_object(builder,"connect_username_combobox_entry"));
    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));

    gtk_widget_show(GTK_WIDGET(main_window_outer_vbox));
}

static void
main_window_class_init (MainWindowClass *class)
{
}

MainWindow *
main_window_new (Application *app)
{
    return g_object_new (MAIN_WINDOW_TYPE, "application", app, NULL);
}

void
main_window_open (MainWindow *win,
                  GFile            *file)
{
}
