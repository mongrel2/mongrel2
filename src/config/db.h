#ifndef _db_h
#define _db_h

int DB_init(const char *path);

void DB_close();

int DB_exec(const char *query, 
        int (*callback)(void*,int,char**,char**),
        void *param);

#endif
