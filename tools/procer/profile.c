#include "procer.h"
#include <dbg.h>

bstring Profile_read_setting(bstring path, const char *file)
{
    bstring target = bformat("%s/%s", bdata(path), file);
    bstring data = NULL;
    FILE *test_file = NULL;

    test_file = fopen(bdata(target), "r"); 
    check(test_file, "Failed to open %s file that's required.", 
            bdata(target));

    data = bgets((bNgetc)fgetc, test_file, '\n');
    if(data) btrimws(data);


error:
    // fallthrough on purpose
    if(test_file) fclose(test_file);
    bdestroy(target);
    return data;
};


int Profile_check_setting(bstring path, const char *file)
{
    bstring target = bformat("%s/%s", bdata(path), file);
    int rc = access(bdatae(target, ""), R_OK);
    bdestroy(target);

    return rc == 0;
}


Profile *Profile_load(bstring profile_dir)
{
    Profile *prof = calloc(sizeof(Profile), 1);
    check_mem(prof);

    prof->command = bformat("%s/run", bdata(profile_dir));
    prof->pid_file = Profile_read_setting(profile_dir, "pid_file");
    prof->restart = Profile_check_setting(profile_dir, "restart");

    return prof;

error:
    return NULL;
}
