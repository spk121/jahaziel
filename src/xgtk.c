#include <gtk/gtk.h>
#include "xgtk.h"

void xgtk_builder_object_unref(GtkBuilder **b)
{
    g_assert(b != 0);
    g_assert(GTK_IS_BUILDER(*b));
    g_object_unref(G_OBJECT(*b));
    *b = NULL;
}

void xgtk_message_dialog(GtkWindow *main_window, const gchar *str)
{
    GtkMessageDialog *dialog =
        GTK_MESSAGE_DIALOG(gtk_message_dialog_new (main_window,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   str));
                           
    /* Destroy the dialog when the user responds to it (e.g. clicks a button) */
    g_signal_connect_swapped (dialog, "response",
                              G_CALLBACK (gtk_widget_destroy),
                              dialog);
    gtk_widget_show(GTK_WIDGET(dialog));
}
