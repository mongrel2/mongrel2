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

#ifndef _headers_h
#define _headers_h

#include <bstring.h>

extern struct tagbstring HTTP_CONTENT_LENGTH;
extern struct tagbstring HTTP_TRANSFER_ENCODING;
extern struct tagbstring HTTP_HOST;
extern struct tagbstring HTTP_METHOD;
extern struct tagbstring HTTP_VERSION;
extern struct tagbstring HTTP_URI;
extern struct tagbstring HTTP_PATH;
extern struct tagbstring HTTP_QUERY;
extern struct tagbstring HTTP_FRAGMENT;
extern struct tagbstring HTTP_BODY;
extern struct tagbstring JSON_METHOD;
extern struct tagbstring XML_METHOD;
extern struct tagbstring HTTP_IF_MATCH;
extern struct tagbstring HTTP_IF_NONE_MATCH;
extern struct tagbstring HTTP_IF_MODIFIED_SINCE;
extern struct tagbstring HTTP_IF_UNMODIFIED_SINCE;
extern struct tagbstring HTTP_POST;
extern struct tagbstring HTTP_GET;
extern struct tagbstring HTTP_HEAD;
extern struct tagbstring HTTP_DELETE;
extern struct tagbstring HTTP_PUT;
extern struct tagbstring HTTP_OPTIONS;
extern struct tagbstring HTTP_PATTERN;
extern struct tagbstring HTTP_USER_AGENT;
extern struct tagbstring HTTP_CONNECTION;
extern struct tagbstring HTTP_X_FORWARDED_FOR;
extern struct tagbstring HTTP_EXPECT;
extern struct tagbstring HTTP_URL_SCHEME;
extern struct tagbstring HTTP_HTTP;
extern struct tagbstring HTTP_HTTPS;
extern struct tagbstring HTTP_REMOTE_ADDR;
extern struct tagbstring DOWNLOAD_CREDITS;

#endif
