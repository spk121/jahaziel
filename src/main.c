#include <gtk/gtk.h>
#include "ui.h"
#include "loop.h"
#include "proc.h"

int main(int argc, char **argv)
{
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
}
