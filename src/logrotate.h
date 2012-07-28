#ifndef _log_h
#define _log_h

int add_log_to_rotate_list(const bstring fname, FILE *f);
void rotate_logs(void);

#endif
