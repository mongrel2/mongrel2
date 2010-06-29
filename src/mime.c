#include <mime.h>
#include <adt/tst.h>
#include <dbg.h>


static tst_t *MIME_MAP = NULL;

enum {
    MAX_EXT_LEN = 128
};

int MIME_add_type(const char *ext, const char *type)
{
    bstring ext_rev = bfromcstr(ext);
    bReverse(ext_rev);
    bstring type_str = bfromcstr(type);

    check(blength(ext_rev) > 0, "No zero length MIME extensions allowed: %s:%s", ext, type);
    check(blength(type_str) > 0, "No zero length MIME types allowed: %s:%s", ext, type);
    check(ext[0] == '.', "Extensions must start with a . '%s:%s'", ext, type);

    check(blength(ext_rev) < MAX_EXT_LEN, "MIME extension %s:%s is longer than %d MAX (it's %d)", ext, type, MAX_EXT_LEN, blength(ext_rev));

    check(!tst_search(MIME_MAP, bdata(ext_rev), blength(ext_rev)), 
            "MIME extension %s already exists, can't add %s:%s",
            ext, ext, type);

    MIME_MAP = tst_insert(MIME_MAP, bdata(ext_rev), blength(ext_rev), type_str);

    bdestroy(ext_rev);

    return 0;
error:
    bdestroy(ext_rev);
    bdestroy(type_str);
    return -1;
}


bstring MIME_match_ext(bstring path, bstring def)
{
    bstring type = tst_search_suffix(MIME_MAP, bdata(path), blength(path));

    return type == NULL ? def : type;
}
