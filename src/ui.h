#ifndef JAHAZIEL_UI_H
#define JAHAZIEL_UI_H

#include <gtk/gtk.h>


/* Convenience macros for obtaining objects from UI file */
#define JAH_GET_OBJECT(builder, name, type, data)                   \
    data->name = type(gtk_builder_get_object(builder, #name))
#define JAH_GET_WIDGET(builder, name, data) \
    JAH_GET_OBJECT(builder, name, GTK_WIDGET, data)

typedef struct
{
    GtkWidget *main_window;
    GtkStatusbar *statusbar;

    // Connect dialog
    GtkWidget *connect_dialog;
    GtkEntry *connect_server_combobox_entry;
    GtkEntry *connect_username_combobox_entry;
    
} JahUI;

void ui_init(void);
void ui_fini(void);

G_MODULE_EXPORT void ui_cb_connect_button_clicked(GtkButton *button, gpointer user_data);

extern JahUI      *ui;


#endif /* JAHAZIEL_UI_H */
