#ifndef _logrotate_h
#define _logrotate_h

int add_log_to_rotate_list(const bstring fname, FILE *f);
int rotate_logs(void);

#endif
