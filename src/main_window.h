#ifndef JAH_MAIN_WINDOW
#define JAH_MAIN_WINDOW

#include <gtk/gtk.h>

struct _MainWindow
{
    GtkApplicationWindow parent;

    // Connect Dialog
    GtkWidget *connect_dialog;
    GtkEntry *connect_server_combobox_entry;
    GtkEntry *connect_username_combobox_entry;
};

struct _MainWindowClass
{
  GtkApplicationWindowClass parent_class;
};

typedef struct _MainWindow MainWindow;
typedef struct _MainWindowClass MainWindowClass;

GType main_window_get_type (void);
MainWindow *main_window_new (Application *app);
void main_window_open (MainWindow *win, GFile *file);

#define MAIN_WINDOW_TYPE (main_window_get_type ())
#define MAIN_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAIN_WINDOW_TYPE, MainWindow))

#endif
