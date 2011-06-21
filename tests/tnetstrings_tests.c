#include "minunit.h"
#include <tnetstrings.h>
#include <request.h>
#include <limits.h>
#include <assert.h>


char *test_tnetstring_numbers()
{
    char *result;
    size_t len;

    tns_value_t max = {.type = tns_tag_number, .value.number = LONG_MAX};
    result = tns_render(&max, &len);
    debug("Got a length from LONG_MAX of: %d", len);
    mu_assert(len == 23 || len == 14, "Wrong length on LONG_MAX");
    free(result);

    // WARNING: LONG_MIN is an edge case
    //tns_value_t min = {.type = tns_tag_number, .value.number = LONG_MIN};
    //result = tns_render(&min, &len);
    //mu_assert(len == 24, "Wrong length on LONG_MIN");
    //free(result);

    tns_value_t minus_one = {.type = tns_tag_number, .value.number = -1};
    result = tns_render(&minus_one, &len);
    mu_assert(len == 5, "Wrong length on -1");
    free(result);

    return NULL;
}

char *test_tnetstring_encode()
{
    size_t len = 0;

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

char *test_complex_types()
{
    bstring data = bfromcstr("16:5:hello,5:12345#}");
    tns_value_t *result = tns_parse(bdata(data), blength(data), NULL);
    mu_assert(result != NULL, "Failed to parse a dict.");
    mu_assert(result->type == tns_tag_dict, "Wrong type, should be dict.");

    size_t len = 0;
    char *rendered = tns_render((void *)result, &len);
    mu_assert(rendered != NULL, "Failed to render dict back.");
    mu_assert(biseqcstr(data, rendered), "Round-trip of dict doesn't work.");
    free(rendered);

    bstring key = bfromcstr("hello");
    tns_value_t *val = hnode_get(hash_lookup(result->value.dict, key));
    mu_assert(val != NULL, "Should have hello as key.");
    mu_assert(val->type == tns_tag_number, "Value should be a number.");
    mu_assert(val->value.number == 12345, "Value should equal 12345.");

    tns_value_destroy(result);
    bdestroy(key);

    bassigncstr(data, "32:5:hello,5:12345#5:hello,5:56789#]");
    result = tns_parse(bdata(data), blength(data), NULL);
    mu_assert(result != NULL, "Failed to parse a list.");
    mu_assert(result->type == tns_tag_list, "Wrong type, should be list.");

    val = darray_get(result->value.list, darray_end(result->value.list) - 1);
    mu_assert(val != NULL, "Should have hello as key.");
    mu_assert(val->type == tns_tag_number, "Value should be a number.");
    mu_assert(val->value.number == 56789, "Value should equal 56789.");

    rendered = tns_render((void *)result, &len);
    debug("RENDERED LIST: %s", rendered);
    mu_assert(rendered != NULL, "Failed to render list back.");
    mu_assert(biseqcstr(data, rendered), "Rendered list wrong.");
    free(rendered);

    tns_value_destroy(result);
    bdestroy(data);


    return NULL;
}

char * all_tests() {
    Request_init();
    mu_suite_start();

    mu_run_test(test_tnetstring_encode);
    mu_run_test(test_tnetstring_decode);
    mu_run_test(test_tnetstring_numbers);
    mu_run_test(test_complex_types);

    return NULL;
}

RUN_TESTS(all_tests);

