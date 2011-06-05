#include "minunit.h"
#include <unixy.h>
#include <unistd.h>
#include <sys/types.h>

char *test_Unixy_getcwd()
{
    bstring dir = Unixy_getcwd();
    mu_assert(dir != NULL, "getcwd failed.");
    debug("CWD is: %s", bdata(dir));

    bdestroy(dir);
    return NULL;
}

char *test_Unixy_chroot_fails()
{
    bstring dir = Unixy_getcwd();
    mu_assert(dir != NULL, "can't getcwd in chroot test.");

    int rc = Unixy_chroot(dir);
    mu_assert(rc == -1, "We shouldn't be able to chroot unless running as root.");

    bdestroy(dir);

    return NULL;
}

char *test_Unixy_drop_priv_fails()
{
    bstring dir = Unixy_getcwd();
    mu_assert(dir != NULL, "can't getcwd in chroot test.");

    Unixy_drop_priv(dir);
    // the results of this are so variable we can't check it
    
    bdestroy(dir);
    return NULL;
}


char *test_Unixy_pid_file()
{
    bstring pid_path = bfromcstr("tests/test.pid");
    unlink((const char *)pid_path->data);

    int rc = Unixy_pid_file(pid_path);
    mu_assert(rc == 0, "Failed to make pid file.");

    rc = Unixy_pid_file(pid_path);
    mu_assert(rc == -1, "Can't run it more than once per process.");

    rc = Unixy_remove_dead_pidfile(pid_path);
    mu_assert(rc == -1, "Should NOT be able to remove the PID file while running.");
    bdestroy(pid_path);

    return NULL;
}


char * all_tests() {
    mu_suite_start();

    mu_run_test(test_Unixy_getcwd);
    if(getuid() != 0) {
        mu_run_test(test_Unixy_chroot_fails);
        mu_run_test(test_Unixy_drop_priv_fails);
    }
    mu_run_test(test_Unixy_pid_file);

    return NULL;
}

RUN_TESTS(all_tests);

