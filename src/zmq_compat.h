/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _ZMQ_COMPAT_H
#define _ZMQ_COMPAT_H

#include <zmq.h>

/*
 * Defines the 0MQ 3.1.1 API in terms of the 0MQ 3.1.0 and 2.X APIs, for
 * backwards compatibility (0MQ 3.0 is not supported, because it broke wire
 * protocol compatibility, among other things).  Simply include "zmq_compat.h"
 * after (or instead of) including <zmq.h>.  Then, write all
 * zmq_{send,recv,poll} calls using the 0MQ 3.1 API.
 *
 * However, in 0MQ 3.1, zmq_msg_{send,recv} return -1 on error, and a positive
 * value on success; 0MQ 2.X zmq_{send,recv} always returned 0 on success.  All
 * uses of zmq_msg_{send,recv} must test for: < 0 (falure) or: >= 0 (success).
 *
 * From http://www.zeromq.org/docs:3-1-upgrade
 */

#ifndef ZMQ_DONTWAIT
#   define ZMQ_DONTWAIT     ZMQ_NOBLOCK
#endif
#if ZMQ_VERSION_MAJOR == 2
#  define zmq_msg_send(msg, sock, opt) zmq_send(sock, msg, opt)
#  define zmq_msg_recv(msg, sock, opt) zmq_recv(sock, msg, opt)
#  define ZMQ_POLL_MSEC    1000        //  zmq_poll is usec
#elif ZMQ_VERSION_MAJOR > 2
#  if ZMQ_VERSION_MAJOR == 3 && ZMQ_VERSION_MINOR < 1
#    error "0MQ 3.0 not supported"
#  elif ZMQ_VERSION_MAJOR == 3 && ZMQ_VERSION_MINOR == 1 
#    if ZMQ_VERSION_PATCH == 0
#      define zmq_msg_send(msg, sock, opt) zmq_sendmsg(sock, msg, opt)
#      define zmq_msg_recv(msg, sock, opt) zmq_recvmsg(sock, msg, opt)
#    endif
#  endif
#  define ZMQ_POLL_MSEC    1           //  zmq_poll is msec
#endif

#endif /* ! defined( _ZMQ_COMPAT_H ) */
