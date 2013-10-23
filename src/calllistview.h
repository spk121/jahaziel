#pragma once

typedef struct
{
    GtkListStore *workers;
    GtkTreeModelSort *sorted;
    GtkTreeModelFilter *filtered;
    gboolean show_busy;
} Store;
 
Store *
create_store (void);
void 
create_view (Store *store, GtkTreeView *view);

