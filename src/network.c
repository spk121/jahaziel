/*
    network.c - network interface

    Copyright 2013 Michael L. Gran <spk121@yahoo.com>

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
  These are functions related to the network interface: ZeroMQ sockets that
  connect to a Jozabad-style broker.
*/
#include <gtk/gtk.h>
#include "joza_msg.h"
#include "xzmq.h"
#include "proc.h"
#include "statusbar.h"
#include "ui.h"
#include "xgtk.h"
#include "network.h"
#include "calllistview.h"

enum {UNBOUND, DISCONNECTED, CONNECTING, CONNECTED, DISCONNECTING} net_state = UNBOUND;

#define JAH_PORT 5555
#define JAH_TIMEOUT_CONNECT 1000

zctx_t *ctx = NULL;
void *sock = NULL;

struct proc_connect_data_tag
{
    gchar *server;
    gchar *username;
    gint state;
};

typedef struct proc_connect_data_tag proc_connect_data_t;

static gboolean broker_connect_func (gpointer data)
{
    proc_connect_data_t *wdata = (proc_connect_data_t *) data;
    gint *state = &(wdata ->state);
    int immed = TRUE;

    g_return_val_if_fail (net_state != UNBOUND, FALSE);

    if (g_atomic_int_get(state) == -1)
    {
        // waiting for something to complete
        return TRUE;
    }
    else if (g_atomic_int_get(state) == 0)
    {
        // State 0: A new connection request
        g_atomic_int_set(state, -1);

        // Wait if we're in the middle of something
        if (net_state == DISCONNECTING || net_state == CONNECTING)
            g_atomic_int_set(state, 0);
        else if (net_state == CONNECTED)
            g_atomic_int_set(state, 1);
        else if (net_state == DISCONNECTED)
            g_atomic_int_set(state, 2);
        else
            g_return_val_if_reached(FALSE);

        return TRUE;
    }
    else if (g_atomic_int_get(state) == 1) 
    {
        // State 1: Connect request received whilst already connected
        g_atomic_int_set(state, -1);
        net_state = DISCONNECTING;
        joza_msg_send_disconnect(sock);
        zsocket_destroy(ctx, sock);
        sock = NULL;
        net_state = DISCONNECTED;

        g_atomic_int_set(state, 2);
        return TRUE;
    }
    else if (g_atomic_int_get(state) == 2)
    {
        // State 2: connect a socket to a broker
        g_atomic_int_set(state, -1);
        net_state = CONNECTING;
        gchar *addr = g_strdup_printf("tcp://%s:%d", wdata->server, JAH_PORT);
        int ret;

        pm_queue_proc(proc_status_message_new("connect", addr));
        ret = zsocket_connect(sock, addr); 

        if (ret == -1)
        {
            // Connection failure
            // pop up warning dialog
            // give up
            
            g_atomic_int_set(state, 3);
            gchar *str = g_strdup_printf("Error connecting to '%s': %s",
                                         addr, zmq_strerror (errno));
            xgtk_message_dialog(GTK_WINDOW(ui->main_window), str);
            g_free (str);
        }
        else
        {
            // FIXME: handle directionality correctly
            pm_queue_proc(proc_status_message_new("connect", wdata->username));
            joza_msg_send_connect(sock, wdata->username, 0);
            bool pret = zsocket_poll (sock, JAH_TIMEOUT_CONNECT);
            if (pret == TRUE)
            {
                joza_msg_t *msg = joza_msg_recv (sock);
                if (joza_msg_id(msg) == JOZA_MSG_CONNECT_INDICATION)
                {
                    // success

                    // Enable
                    g_atomic_int_set(state, 4);
                }
                else 
                {
                    if (joza_msg_id(msg) == JOZA_MSG_DIAGNOSTIC)
                        // failure with info
                        // pop up warning dialog
                        ;
                    else if (joza_msg_id(msg) == JOZA_MSG_DISCONNECT_INDICATION)
                        // failure without info
                        // pop up warning dialog
                        ;
                    else
                        // unexpected message
                        // disconnect
                        ;
                    gchar *str = g_strdup_printf("Connection rejected by '%s'", addr);
                    xgtk_message_dialog(GTK_WINDOW(ui->main_window), str);
                    g_free (str);

                    g_atomic_int_set(state, 3);
                } 
            }
            else 
            {
                // timeout
                // pop up warning
                // disconnect
                gchar *str = g_strdup_printf("No response from '%s'", addr);
                xgtk_message_dialog(GTK_WINDOW(ui->main_window), str);
                g_free (str);

                g_atomic_int_set(state, 3);
            }
        }
        g_free(addr);

        return TRUE;
    }
    else if (g_atomic_int_get(state) == 3)
    {
        // Failed to connect
        g_atomic_int_set(state, -1);
        net_state = DISCONNECTED;

        // FIXME: remove this sock as a source of main loop data

        goto clean;
        g_return_val_if_reached (FALSE);
    }
    else if (g_atomic_int_get(state) == 4)
    {
        // Connected
        g_atomic_int_set(state, -1);
        net_state = CONNECTED;
        pm_queue_proc(proc_status_message_new("connect", "Connected"));

        pm_queue_proc(proc_zmq_handler_new());

    clean:
        g_free(wdata->server);
        wdata->server = NULL;
        g_free(wdata->username);
        wdata->username = NULL;
        g_free(wdata);
        wdata = NULL;

        return FALSE;
    }
    else
        g_return_val_if_reached(FALSE);
}

static gboolean proc_connect_func (gpointer data)
{
    proc_connect_data_t *wdata = (proc_connect_data_t *) data;

    g_assert (data != NULL);

    if (net_state == UNBOUND)
    {
        ctx = xzctx_new ();
        net_state = DISCONNECTED;
    }

    if (sock == NULL)
        sock = xzsocket_new(ctx, ZMQ_DEALER);
    
    g_assert(sock != NULL);

    gboolean ret = broker_connect_func(data);
    return ret;
}

GHook *
proc_connect_new (gchar *server, gchar *username)
{
    GHook *hook = pm_proc_new();
    proc_connect_data_t *connect_data = g_new0 (proc_connect_data_t, 1);
    connect_data->server = g_strdup(server);
    connect_data->username = g_strdup(username);
    hook->data = connect_data;
    hook->func = proc_connect_func;
    g_free (server);
    g_free(username);
    return hook;
}


/****************************************************************/

struct proc_directory_data_tag
{
    gchar *server;
    gchar *username;
    gint state;
};

typedef struct proc_directory_data_tag proc_directory_data_t;

static gboolean proc_directory_func (gpointer data)
{
    proc_directory_data_t *wdata = (proc_directory_data_t *) data;

    g_assert (data != NULL);

    if (sock == NULL || net_state != CONNECTED)
    {
        if (sock == NULL)
            pm_queue_proc(proc_status_message_new("directory", "No socket available"));
        else if (net_state != CONNECTED)
            pm_queue_proc(proc_status_message_new("directory", "Not connected"));
    }
    else
    {
        joza_msg_send_directory_request(sock);
    }

    g_free(wdata);
    return FALSE;
}

GHook *
proc_directory_new ()
{
    GHook *hook = pm_proc_new();
    proc_directory_data_t *dir_data = g_new0 (proc_directory_data_t, 1);
    hook->data = dir_data;
    hook->func = proc_directory_func;
    return hook;
}

/****************************************************************/

struct proc_zmq_handler_data_tag
{
    gint state;
};

typedef struct proc_zmq_handler_data_tag proc_zmq_handler_data_t;

static gboolean proc_zmq_handler_func (gpointer data)
{
    proc_zmq_handler_data_t *wdata = (proc_zmq_handler_data_t *) data;
    gint *state = &(wdata ->state);

    if (g_atomic_int_get(state) == -1)
    {
        return TRUE;
    }
    else
    {
        g_atomic_int_set(state, -1);

        if (sock == NULL || net_state != CONNECTED)
        {
            g_free (wdata);
            if (sock == NULL)
                pm_queue_proc(proc_status_message_new("zmq_handler", "No socket available"));
            else if (net_state != CONNECTED)
                pm_queue_proc(proc_status_message_new("zmq_handler", "Not connected"));
            
            return FALSE;
        }

        zmq_pollitem_t items [] = { { sock, 0, ZMQ_POLLIN, 0 } };
        int rc = zmq_poll (items, 1, 1);
        if (rc == -1)
        {
            g_free(wdata);
            pm_queue_proc(proc_status_message_new("zmq_handler", "Disconnecting"));
            return FALSE;
        }
        else if (rc == 0)
        {
            g_atomic_int_set(state, 0);
            return TRUE;
        }

        joza_msg_t *msg = joza_msg_recv (sock);
        pm_queue_proc(proc_status_message_new("zmq_handler", joza_msg_const_command(msg)));
        //msg_apply(msg);
        if (joza_msg_id (msg) == JOZA_MSG_DIRECTORY) 
        {
            zhash_t *workers = joza_msg_workers (msg);
            zlist_t *wlist = zhash_keys(workers);
            gchar *first = zlist_first(wlist);
            gchar *cur = first;

            extern Store* store;
            GtkTreeIter iter;
            GtkTreePath *path ;

            gtk_list_store_clear(store->workers);

            do {
                if (cur)
                {
                    gint i;
                    // Add one worker name to the Call dialog box's user view
                    gtk_list_store_append(store->workers, &iter);
                    gtk_list_store_set(store->workers, &iter, 0, cur, 1, TRUE, -1);
                }
                cur = zlist_next(wlist);
            } while (cur && cur != first);
        }
        joza_msg_destroy(&msg);
        g_atomic_int_set(state, 0);
        return TRUE;
    }
}

GHook *
proc_zmq_handler_new ()
{
    GHook *hook = pm_proc_new();
    proc_zmq_handler_data_t *wdata = g_new0 (proc_zmq_handler_data_t, 1);
    gint *state = &(wdata ->state);
    g_atomic_int_set(state, 0);

    hook->data = wdata;
    hook->func = proc_zmq_handler_func;
    return hook;
}


