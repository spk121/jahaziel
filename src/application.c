#include <gtk/gtk.h>
#include "application.h"
#include "main_window.h"

G_DEFINE_TYPE(Application, application, GTK_TYPE_APPLICATION);

static void
application_init (Application *app)
{
    // Process manager initialization
    pm_init();

}

static void
application_activate (GApplication *app)
{
    MainWindow *win;
    
    win = main_window_new (APPLICATION (app));
    gtk_window_present (GTK_WINDOW (win));
}

static void
application_open (GApplication  *app,
              GFile        **files,
              gint           n_files,
              const gchar   *hint)
{
    GList *windows;
    MainWindow *win;
    int i;
    
    windows = gtk_application_get_windows (GTK_APPLICATION (app));
    if (windows)
        win = MAIN_WINDOW (windows->data);
    else
        win = main_window_new (APPLICATION (app));

    for (i = 0; i < n_files; i++)
        main_window_open (win, files[i]);
    
    gtk_window_present (GTK_WINDOW (win));
}

static void
application_class_init (ApplicationClass *class)
{
    G_APPLICATION_CLASS (class)->activate = application_activate;
    G_APPLICATION_CLASS (class)->open = application_open;
}

Application *
application_new (void)
{
    return g_object_new (APPLICATION_TYPE,
                         "application-id", "com.lonelycactus.jahaziel",
                         "flags", G_APPLICATION_HANDLES_OPEN,
                         NULL);
}
