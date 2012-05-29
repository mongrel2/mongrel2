#include "minunit.h"
#include "zmq_compat.h"
#include "server.h"
#include "config/config.h"
#include "config/db.h"
#include "config/module.h"
#include "mime.h"
#include "setting.h"

char *test_Config_load_settings()
{
    Config_init_db("tests/config.sqlite");

    int rc = Config_load_settings();
    debug("LOADED %d SETTINGS", rc);
    mu_assert(rc > 0, "Failed to load the right number of settings.");

    Config_close_db();
    Setting_destroy();

    return NULL;
}

struct tagbstring TEST_MIME = bsStatic("/test.txt");

char *test_Config_load_mimetypes()
{
    Config_init_db("tests/config.sqlite");

    int rc = Config_load_mimetypes();
    debug("LOADED %d MIME TYPES", rc);
    mu_assert(rc > 2, "Failed to load the right number of mimetypes.");

    mu_assert(MIME_match_ext(&TEST_MIME, NULL) != NULL, "Mime load failed.");
    Config_close_db();
    MIME_destroy();
    return NULL;
}

char *test_Config_load() 
{
    Config_init_db("tests/config.sqlite");
    Server *server = Config_load_server("AC1F8236-5919-4696-9D40-0F38DE9E5861");
    mu_assert(server != NULL, "Failed to load server");

    Server_destroy(server);
    Config_close_db();

    return NULL;
}

char *test_Config_load_module()
{
    int rc = Config_module_load("tools/config_modules/null.so");
    mu_assert(rc == 0, "Failed to load the null module.");

    rc = CONFIG_MODULE.init("goodpath");
    mu_assert(rc == 0, "The null module should fail init.");

    rc = CONFIG_MODULE.init("badpath");
    mu_assert(rc == -1, "The null module should fail init.");

    return NULL;
}


char * all_tests() 
{
    mu_suite_start();

    Server_init();

    mu_run_test(test_Config_load_mimetypes);
    mu_run_test(test_Config_load_settings);
    mu_run_test(test_Config_load);
    mu_run_test(test_Config_load_module);

    zmq_term(ZMQ_CTX);

    return NULL;
}

RUN_TESTS(all_tests);
