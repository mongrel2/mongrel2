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

#include <headers.h>

struct tagbstring HTTP_METHOD = bsStatic("METHOD");
struct tagbstring HTTP_VERSION = bsStatic("VERSION");
struct tagbstring HTTP_URI = bsStatic("URI");
struct tagbstring HTTP_PATH = bsStatic("PATH");
struct tagbstring HTTP_QUERY = bsStatic("QUERY");
struct tagbstring HTTP_FRAGMENT = bsStatic("FRAGMENT");
struct tagbstring HTTP_BODY = bsStatic("BODY");
struct tagbstring JSON_METHOD = bsStatic("JSON");
struct tagbstring XML_METHOD = bsStatic("XML");


struct tagbstring HTTP_POST = bsStatic("POST");
struct tagbstring HTTP_GET = bsStatic("GET");
struct tagbstring HTTP_HEAD = bsStatic("HEAD");
struct tagbstring HTTP_DELETE = bsStatic("DELETE");
struct tagbstring HTTP_PUT = bsStatic("PUT");
struct tagbstring HTTP_OPTIONS = bsStatic("OPTIONS");
struct tagbstring HTTP_PATTERN = bsStatic("PATTERN");
struct tagbstring HTTP_URL_SCHEME = bsStatic("URL_SCHEME");
struct tagbstring HTTP_HTTP = bsStatic("http");
struct tagbstring HTTP_HTTPS = bsStatic("https");


struct tagbstring HTTP_CONTENT_LENGTH = bsStatic("content-length");
struct tagbstring HTTP_HOST = bsStatic("host");
struct tagbstring HTTP_IF_MATCH = bsStatic("if-match");
struct tagbstring HTTP_IF_NONE_MATCH = bsStatic("if-none-match");
struct tagbstring HTTP_IF_MODIFIED_SINCE = bsStatic("if-modified-since");
struct tagbstring HTTP_IF_UNMODIFIED_SINCE = bsStatic("if-unmodified-since");
struct tagbstring HTTP_USER_AGENT = bsStatic("user-agent");
struct tagbstring HTTP_CONNECTION = bsStatic("connection");
struct tagbstring HTTP_EXPECT = bsStatic("expect");

struct tagbstring HTTP_X_FORWARDED_FOR = bsStatic("x-forwarded-for");
