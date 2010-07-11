#ifdef __APPLE__

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

/* Wrapper function for sendfile that mac OS X uses */
int mac_sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
    off_t my_count = count;
    if(sendfile(in_fd, out_fd, *offset, &my_count, NULL, 0))
        return -1;
    *offset += my_count;
    return my_count;
}

#endif
