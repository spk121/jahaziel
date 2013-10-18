/*
    proc.h - Background process manager

    Copyright 2012, 2013 Michael L. Gran <spk121@yahoo.com>

    This file is part of Jahaziel.

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
#ifndef JAH_PROC_H
#define JAH_PROC_H
#include <glib.h>

void pm_init (void);
void pm_dequeue_proc (GHook *proc);
void pm_finalize (void);
void pm_queue_proc (GHook *proc);
GHook *pm_proc_new (void);
int pm_iterate (double delta_t);
double pm_get_delta_time (void);
void pm_set_delta_time (double dt);
GHook *proc_wait_new (double dtime, gboolean (*child_func) (gpointer data), gpointer child_data);


#endif
