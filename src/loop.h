#ifndef JAH_LOOP_H
#define JAH_LOOP_H

void loop (void);
void loop_quit (void);
void loop_set_full_speed_flag (void);
void loop_set_update_func (int idle (double delta_t));

#endif
