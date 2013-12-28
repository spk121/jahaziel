#pragma once

GHook *proc_connect_new (gchar *server, gchar *username);
GHook *proc_directory_new (void);
GHook *proc_zmq_handler_new (void);
GHook *proc_call_request_new (const char *name);

