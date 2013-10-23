#include <gtk/gtk.h>
#include "calllistview.h"

enum {
    COLUMN_ADDRESS = 0,
    COLUMN_BUSY,
    N_COLUMNS
};

 
static gboolean
row_visible (GtkTreeModel *model,
             GtkTreeIter *iter,
             Store *store)
{
    gboolean busy;
 
    gtk_tree_model_get (model, iter,
                        COLUMN_BUSY, &busy,
                        -1);
    
    if (store->show_busy)
        return TRUE;
    return !busy;
}
 
Store *
create_store (void)
{
    Store *store;
    GtkTreeIter iter;
 
    store = g_new0 (Store, 1);
    store->show_busy = TRUE;
    store->workers = gtk_list_store_new (N_COLUMNS,
                                          G_TYPE_STRING,
                                          G_TYPE_BOOLEAN);
    /* Add some items */
    gtk_list_store_append (store->workers, &iter);
    gtk_list_store_set (store->workers, &iter, COLUMN_ADDRESS, "Spam", COLUMN_BUSY, TRUE, -1);
 
    gtk_list_store_append (store->workers, &iter);
    gtk_list_store_set (store->workers, &iter, COLUMN_ADDRESS, "Beer", COLUMN_BUSY, FALSE, -1);
 
    gtk_list_store_append (store->workers, &iter);
    gtk_list_store_set (store->workers, &iter, COLUMN_ADDRESS, "Chewing Gum", COLUMN_BUSY, TRUE, -1);
 
    store->filtered = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (store->workers), NULL));
    store->sorted = GTK_TREE_MODEL_SORT (gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store->filtered)));
 
    gtk_tree_model_filter_set_visible_func (store->filtered,
                                            (GtkTreeModelFilterVisibleFunc) row_visible,
                                            store, NULL);
 
    return store;
}

// When we get a double-click, we go here.  This is the same a selecting a row and hitting
// the connect button. 
static void
on_row_activated (GtkTreeView *view,
                  GtkTreePath *path,
                  GtkTreeViewColumn *col,
                  Store *store)
{
    GtkTreeIter iter;
    GtkTreePath *filtered_path;
    GtkTreePath *child_path;
 
    filtered_path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (store->sorted),
                                                                    path);
 
    child_path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (store->filtered),
                                                                   filtered_path);
 
    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (store->workers), &iter, child_path)) {
        gchar *address;
        gboolean busy;
 
        gtk_tree_model_get (GTK_TREE_MODEL (store->workers), &iter,
                            COLUMN_ADDRESS, &address,
                            COLUMN_BUSY, &busy,
                            -1);
 
        g_free (address);

        // FIXME: add some call logic here.
    }
}

void 
create_view (Store *store, GtkTreeView *view)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *address_column;
    GtkTreeViewColumn *busy_column;
 
    gtk_tree_view_set_model(view, GTK_TREE_MODEL(store->sorted));
    gtk_widget_set_vexpand (GTK_WIDGET (view), TRUE);
 
    renderer = gtk_cell_renderer_text_new ();
 
    address_column = gtk_tree_view_column_new_with_attributes ("Address", renderer,
                                                               "text", COLUMN_ADDRESS,
                                                               NULL);
 
    gtk_tree_view_column_set_sort_column_id (address_column, COLUMN_ADDRESS);
    gtk_tree_view_append_column (view, address_column);
 
    busy_column = gtk_tree_view_column_new_with_attributes ("Busy", renderer,
                                                             "text", COLUMN_BUSY,
                                                             NULL);
 
    gtk_tree_view_column_set_sort_column_id (busy_column, COLUMN_BUSY);
    gtk_tree_view_append_column (view, busy_column);
 
    g_signal_connect (view, "row-activated",
                      G_CALLBACK (on_row_activated), store);
 
}
 
static void
on_show_busy_changed (GtkAdjustment *adjustment,
                      Store *store)
{
    store->show_busy = gtk_adjustment_get_value (adjustment);
    gtk_tree_model_filter_refilter (store->filtered);
}
 
#if 0
gint
main (gint argc,
      gchar *argv[])
{
    GtkWidget *window;
    GtkWidget *view;
    GtkWidget *box;
    GtkWidget *spinbutton;
    GtkAdjustment *show_busy;
    Store *store;
 
    gtk_init (&argc, &argv);
 
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
 
    store = create_store ();
    view = GTK_WIDGET (create_view (store));
 
    show_busy = gtk_adjustment_new (10.99, 0.01, 1024.0, 0.01, 1.0, 0.0);
    spinbutton = gtk_spin_button_new (show_busy, 1.0, 2);
 
    gtk_container_add (GTK_CONTAINER (window), box);
    gtk_container_add (GTK_CONTAINER (box), view);
    gtk_container_add (GTK_CONTAINER (box), spinbutton);
 
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
 
    g_signal_connect (G_OBJECT (window), "delete-event",
                      G_CALLBACK (gtk_main_quit), NULL);
 
    g_signal_connect (G_OBJECT (show_busy), "value-changed",
                      G_CALLBACK (on_show_busy_changed), store);
 
    gtk_widget_show_all (window);
    gtk_main ();
 
    return 0;
}
#endif
