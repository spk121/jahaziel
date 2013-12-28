#include <gtk/gtk.h>
#include "application.h"
#include "ui.h"
#include "loop.h"
#include "proc.h"

int main(int argc, char **argv)
{
    //return g_application_run (G_APPLICATION (application_new ()), argc, argv);
#if 1
    g_log_set_always_fatal(G_LOG_LEVEL_CRITICAL);
    gtk_init(&argc, &argv);
    ui_init();
    // gtk_main();

    // Process manager initialization
    pm_init();

    // Register process manager as on_idle func
    loop_set_update_func(pm_iterate);

    // Go!
    loop();
    ui_fini();
    return 0;
#endif

}
