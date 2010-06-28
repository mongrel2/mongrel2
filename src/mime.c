#include <mime.h>
#include <adt/tst.h>
#include <dbg.h>

static tst_t *MIME_MAP = NULL;

enum {
    MAX_EXT_LEN = 128
};

int MIME_add_type(const char *ext, const char *type)
{
    char ext_buf[MAX_EXT_LEN];
    int pi, ri;
    size_t ext_len = strlen(ext);

    check(ext_len > 0, "No zero length MIME extensions allowed: %s:%s", ext, type);
    check(strlen(type) > 0, "No zero length MIME types allowed: %s:%s", ext, type);
    check(ext[0] == '.', "Extensions must start with a . '%s:%s'", ext, type);

    check(ext_len < MAX_EXT_LEN, "MIME extension %s:%s is longer than %d MAX (it's %d)", ext, type, MAX_EXT_LEN, ext_len);

    check(!tst_search_suffix(MIME_MAP, ext, ext_len), "MIME extension %s already exists, can't add %s:%s", ext, ext, type);

    for(pi = ext_len-1, ri = 0; ri < ext_len; ri++, pi--) {
        ext_buf[ri] = ext[pi];
    }
    ext_buf[ext_len] = '\0';

    MIME_MAP = tst_insert(MIME_MAP, ext_buf, ext_len, strdup(type));

    return 0;
error:
    return -1;
}


const char *MIME_match_ext(const char *path, size_t len, const char *def)
{
    const char *type = tst_search_suffix(MIME_MAP, path, len);

    return type == NULL ? def : type;
}
