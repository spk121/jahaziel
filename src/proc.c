/*
    proc.c - Background process manager

    Copyright 2012, 2013 Michael L. Gran <spk121@yahoo.com>

    This file is part of Jahaziel

    Jahaziel is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jahaziel is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jahaziel.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
  The Process Manager (PM) is a list of hook functions and associated
  data.  Each function in the list is called on its associated data
  during each on_idle.  If a function returns FALSE, it is removed
  from the PM.
*/

#include "xglib.h"
#include "proc.h"


static GHookList *pm;
double delta_time = 0.0;

void pm_set_delta_time (double dt)
{
    delta_time = dt;
}

double pm_get_delta_time (void)
{
	return delta_time;
}

/* Initializes the process manager */
void
pm_init ()
{
    pm = g_new0 (GHookList, 1);
    xg_hook_list_init (pm);
}

GHook *pm_proc_new (void)
{
    return xg_hook_alloc (pm);
}

void
pm_finalize ()
{
    g_free (pm);
}

void
pm_queue_proc (GHook *proc)
{
    g_hook_append (pm, proc);
}

void
pm_dequeue_proc (GHook *proc)
{
    g_hook_unref (pm, proc);
}

/* Run each process being managed by the process manager.  DELTA_T is
   the time since this was last called, according to the timer in the
   main loop.  */
int
pm_iterate (double delta_t)
{
    gint proc_count = 0;
    GHook *proc;

    pm_set_delta_time (delta_t);

    /* Call all the processes being managed by the process manager,
       skipping any slow processes that aren't complete from the last
       iteration.  (Any process that returns FALSE is culled.) */
    g_hook_list_invoke_check (pm, FALSE);
    proc = g_hook_first_valid (pm, TRUE);

	/* Count the number of processes being managed by the process
	   manager. */
    while (proc != NULL)
    {
        proc_count ++;
        proc = g_hook_next_valid (pm, proc, TRUE);
    }
    return proc_count;
}

/****************************************************************/
/* Wait process -- waits for time to pass, then hooks a child process
   to the PM. */
struct proc_wait_data_tag
{
    double start_time;
    double stop_time;
    gboolean (*child_func) (gpointer data);
    gpointer child_data;
};

typedef struct proc_wait_data_tag proc_wait_data_t;

/* Wait for time to pass, then add the child func to the PM */
static gboolean proc_wait_func (gpointer data)
{
    proc_wait_data_t *wdata = (proc_wait_data_t *) data;

    g_assert (data != NULL);

    wdata->start_time += delta_time;
    if (wdata->start_time > wdata->stop_time)
    {
        if (wdata->child_func == NULL)
            g_error ("wait process has no child func");
        if (wdata->child_data == NULL)
            g_error ("wait process has no child data");

        GHook *h = g_hook_alloc (pm);
        g_assert (h != NULL);

        h->data = wdata->child_data;
        h->func = wdata->child_func;
        g_hook_append (pm, h);
        wdata->child_func = NULL;

        return FALSE;
    }
    else
        return TRUE;
}

GHook *
proc_wait_new (double dtime, gboolean (*child_func) (gpointer data), gpointer child_data)
{
    g_assert (dtime > 0.0);
    g_assert (child_func != NULL);
    g_assert (child_data != NULL);

    GHook *hook = xg_hook_alloc (pm);
    proc_wait_data_t *wait_data = g_new0 (proc_wait_data_t, 1);
    wait_data->start_time = 0.0;
    wait_data->stop_time = dtime;
    wait_data->child_func = child_func;
    wait_data->child_data = child_data;
    hook->data = wait_data;
    hook->func = proc_wait_func;
    return hook;
}
