#undef NDEBUG
#include "minunit.h"
#include <tnetstrings.h>
#include <request.h>
#include <assert.h>

FILE *LOG_FILE = NULL;

char *test_tnetstring_encode()
{
    size_t len = 0;
    unsigned long i = 0;

    for(i = 0; i < 100000; i++) {
        tns_value_t val = {.type = tns_tag_null};
        char *result = tns_render(&val, &len);
        mu_assert(len == 3, "Wrong length on null.");
        free(result);

        tns_value_t num = {.type = tns_tag_number, .value.number = 12345};
        result = tns_render(&num, &len);
        mu_assert(len == 8, "Wrong length on number.");
        free(result);

        bstring data = bfromcstr("hello");
        tns_value_t str = {.type = tns_tag_string, .value.string = data};
        result = tns_render(&str, &len);
        mu_assert(len == 8, "Wrong length on string.");
        bdestroy(data);
        free(result);

        tns_value_t boolean = {.type = tns_tag_bool, .value.bool = 1};
        result = tns_render(&boolean, &len);
        mu_assert(len == 7, "Wrong length on true.");
        free(result);

        boolean.value.bool = 0;
        result = tns_render(&boolean, &len);
        mu_assert(len == 8, "Wrong length on false.");
        free(result);
    }

    return NULL;
}

char *test_tnetstring_decode() 
{
    bstring data = bfromcstr("0:~");
    tns_value_t *result = tns_parse(bdata(data), blength(data), NULL);

    mu_assert(result != NULL, "Failed to get a true.");
    mu_assert(result->type == tns_tag_null, "Wrong type, should be null.");
    free(result);

    bassigncstr(data, "4:true!");
    result = tns_parse(bdata(data), blength(data), NULL);
    mu_assert(result != NULL, "Failed to parse a true.");
    mu_assert(result->type == tns_tag_bool, "Wrong type, should be bool.");
    mu_assert(result->value.bool == 1, "Wrong value, should be 1.");
    free(result);

    bassigncstr(data, "5:false!");
    result = tns_parse(bdata(data), blength(data), NULL);
    mu_assert(result != NULL, "Failed to parse a true.");
    mu_assert(result->type == tns_tag_bool, "Wrong type, should be bool.");
    mu_assert(result->value.bool == 0, "Wrong value, should be 0.");
    free(result);

    bassigncstr(data, "5:hello,");
    bstring compare = bfromcstr("hello");
    result = tns_parse(bdata(data), blength(data), NULL);
    mu_assert(result != NULL, "Failed to parse a true.");
    mu_assert(result->type == tns_tag_string, "Wrong type, should be string.");
    mu_assert(biseq(result->value.string, compare), "Wrong value, should be 'hello'.");
    bdestroy(result->value.string);
    free(result);
    bdestroy(compare);

    bassigncstr(data, "5:12345#");
    result = tns_parse(bdata(data), blength(data), NULL);
    mu_assert(result != NULL, "Failed to parse a true.");
    mu_assert(result->type == tns_tag_number, "Wrong type, should be number.");
    mu_assert(result->value.number == 12345, "Wrong value, should be 12345.");
    free(result);

    bdestroy(data);

    return NULL;
}

char * all_tests() {
    Request_init();
    mu_suite_start();

    mu_run_test(test_tnetstring_encode);
    mu_run_test(test_tnetstring_decode);

    return NULL;
}

RUN_TESTS(all_tests);

