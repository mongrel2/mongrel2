#include <xrequest.h>
#include <dbg.h>
#include <tnetstrings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <connection.h>

static int mydispatch(Connection *conn, tns_value_t *data)
{
    int fd=-1;
    long long file_size;
    long long sent;
    tns_value_t *tnsname;
    bstring fname;
    check(data->type==tns_tag_list,"List expected.");
    tnsname=darray_get(data->value.list,1);

    check(tnsname->type==tns_tag_string, "String expected for SENDFILE xreq payload.");

    fname=tnsname->value.string;

    fd = open(bdata(fname), O_RDONLY);
    check(fd >=0, "Failed to open file: %s", bdata(fname));
    file_size = lseek(fd, 0L, SEEK_END);
    check(file_size >= 0, "Failed to seek end of file: %s", bdata(fname));
    lseek(fd, 0L, SEEK_SET);
    sent = IOBuf_stream_file(conn->iob, fd, file_size);
    check(sent == file_size, "Error streaming file. Sent %d of %d bytes.",
            sent, file_size);
    fdclose(fd);
    return 0;

    error:
        if (fd>0) {
            fdclose(fd);
        }
        return -1;
}

struct tagbstring SENDFILE = bsStatic("sendfile");

deliver_function xrequest_init(Server *srv, bstring load_path, tns_value_t *config, bstring **keys, int *nkeys)
{
    static bstring sendfile=&SENDFILE;
    *keys=&sendfile;
    *nkeys=1;
    return mydispatch;
}


