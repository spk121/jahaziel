#ifndef JAH_APPLICATION_H
#define JAH_APPLICATION_H

#include <gtk/gtk.h>

struct _Application
{
    GtkApplication parent;
};

struct _ApplicationClass
{
    GtkApplicationClass parent_class;
};

typedef struct _Application Application;
typedef struct _ApplicationClass ApplicationClass;

Application *application_new (void);

#define APPLICATION_TYPE (application_get_type ())
#define APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), APPLICATION_TYPE, Application))

#endif
