#include <gtk/gtk.h>
#include "statusbar.h"
#include "ui.h"
#include "proc.h"

/* Statusbar process
   1. Push a message onto the statusbar message stack.
   2. Wait five seconds
   3. Remove the message
*/

struct proc_status_message_data_tag
{
    double start_time;
    double stop_time;
    guint context_id;
    guint message_id;
    gchar *context_description;
    gchar *text;
    gboolean shown;
};

typedef struct proc_status_message_data_tag proc_status_message_data_t;

/* Wait for time to pass, then add the child func to the PM */
static gboolean proc_status_message_func (gpointer data)
{
    proc_status_message_data_t *s = (proc_status_message_data_t *) data;

    g_assert (data != NULL);

    if (s->shown == FALSE) 
    {
        s->context_id = gtk_statusbar_get_context_id (ui->statusbar, s->context_description);
        s->message_id = gtk_statusbar_push (ui->statusbar, s->context_id, s->text);
        s->shown = TRUE;
    }

    s->start_time += pm_get_delta_time();
    if (s->start_time > s->stop_time)
    {
        g_free(s->text);
        g_free(s->context_description);
        gtk_statusbar_remove(ui->statusbar, s->context_id, s->message_id);
        return FALSE;
    }
    else
        return TRUE;
}

GHook *
proc_status_message_new (const gchar *context_description, const gchar *text)
{
    GHook *hook = pm_proc_new();
    proc_status_message_data_t *status_data = g_new0 (proc_status_message_data_t, 1);
    status_data->start_time = 0.0;
    status_data->stop_time = 5.0;
    
    status_data->context_description = g_strdup(context_description);
    status_data->text = g_strdup(text);
    status_data->shown = FALSE;

    hook->data = status_data;
    hook->func = proc_status_message_func;

    return hook;
}
