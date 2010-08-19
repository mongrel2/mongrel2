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

#include "http11_parser.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dbg.h>

#define LEN(AT, FPC) (FPC - buffer - parser->AT)
#define MARK(M,FPC) (parser->M = (FPC) - buffer)
#define PTR_TO(F) (buffer + parser->F)

/** Machine **/

%%{
  
  machine http_parser;

  action mark {MARK(mark, fpc); }


  action start_field { MARK(field_start, fpc); }
  action write_field { 
    parser->field_len = LEN(field_start, fpc);
  }

  action start_value { MARK(mark, fpc); }

  action write_value {
    if(parser->http_field != NULL) {
      parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, fpc));
    }
  }

  action request_method { 
    if(parser->request_method != NULL) 
      parser->request_method(parser->data, PTR_TO(mark), LEN(mark, fpc));
  }

  action request_uri { 
    if(parser->request_uri != NULL)
      parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, fpc));
  }

  action fragment {
    if(parser->fragment != NULL)
      parser->fragment(parser->data, PTR_TO(mark), LEN(mark, fpc));
  }

  action start_query {MARK(query_start, fpc); }
  action query_string { 
    if(parser->query_string != NULL)
      parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, fpc));
  }

  action http_version {	
    if(parser->http_version != NULL)
      parser->http_version(parser->data, PTR_TO(mark), LEN(mark, fpc));
  }

  action request_path {
    if(parser->request_path != NULL)
      parser->request_path(parser->data, PTR_TO(mark), LEN(mark,fpc));
  }

  action done { 
    parser->body_start = fpc - buffer + 1; 
    if(parser->header_done != NULL)
      parser->header_done(parser->data, fpc + 1, pe - fpc - 1);
    fbreak;
  }

  action socket {
      parser->socket_started = 1;
  }

  action json {
      parser->json_sent = 1;
  }


#### HTTP PROTOCOL GRAMMAR
# line endings
  CRLF = ("\r\n" | "\n");

# character types
  CTL = (cntrl | 127);
  safe = ("$" | "-" | "_" | ".");
  extra = ("!" | "*" | "'" | "(" | ")" | ",");
  reserved = (";" | "/" | "?" | ":" | "@" | "&" | "=" | "+");
  unsafe = (CTL | " " | "\"" | "#" | "%" | "<" | ">");
  national = any -- (alpha | digit | reserved | extra | safe | unsafe);
  unreserved = (alpha | digit | safe | extra | national);
  escape = ("%" xdigit xdigit);
  uchar = (unreserved | escape);
  pchar = (uchar | ":" | "@" | "&" | "=" | "+");
  tspecials = ("(" | ")" | "<" | ">" | "@" | "," | ";" | ":" | "\\" | "\"" | "/" | "[" | "]" | "?" | "=" | "{" | "}" | " " | "\t");

# elements
  token = (ascii -- (CTL | tspecials));

# URI schemes and absolute paths
  scheme = "http";
  absolute_uri = (scheme ":" (uchar | reserved )*);

  path = ( pchar+ ( "/" pchar* )* ) ;
  query = ( uchar | reserved )* %query_string ;
  param = ( pchar | "/" )* ;
  params = ( param ( ";" param )* ) ;
  rel_path = ( path? %request_path (";" params)? ) ("?" %start_query query)?;
  absolute_path = ( "/"+ rel_path );

  Request_URI = ( "*" | absolute_uri | absolute_path ) >mark %request_uri;
  Fragment = ( uchar | reserved )* >mark %fragment;
  Method = ( upper | digit | safe ){1,20} >mark %request_method;

  http_number = ( "1." ("0" | "1") ) ;
  HTTP_Version = ( "HTTP/" http_number ) >mark %http_version ;
  Request_Line = ( Method " " Request_URI ("#" Fragment){0,1} " " HTTP_Version CRLF ) ;

  field_name = ( token -- ":" )+ >start_field %write_field;

  field_value = any* >start_value %write_value;

  message_header = field_name ":" " "* field_value :> CRLF;

  Request = Request_Line ( message_header )* ( CRLF @done );

  SocketStart = "<policy-file-request/>\0" @socket @done;
  SocketJSON = ("@" rel_path ) >mark %request_path " {" any** "\0" @json @done;
  SocketRequest = SocketStart | SocketJSON;

main := Request | SocketRequest;

}%%

/** Data **/
%% write data;

int http_parser_init(http_parser *parser) {
  int cs = 0;
  %% write init;
  parser->cs = cs;
  parser->body_start = 0;
  parser->content_len = 0;
  parser->mark = 0;
  parser->nread = 0;
  parser->field_len = 0;
  parser->field_start = 0;
  parser->socket_started = 0;
  parser->json_sent = 0;

  return(1);
}


/** exec **/
size_t http_parser_execute(http_parser *parser, const char *buffer, size_t len, size_t off)  
{
  if(len == 0) return 0;

  const char *p, *pe;
  int cs = parser->cs;

  assert(off <= len && "offset past end of buffer");

  p = buffer+off;
  pe = buffer+len;

  assert(pe - p == len - off && "pointers aren't same distance");

  %% write exec;

  assert(p <= pe && "Buffer overflow after parsing.");

  if (!http_parser_has_error(parser)) {
      parser->cs = cs;
  }

  parser->nread += p - (buffer + off);

  assert(parser->nread <= len && "nread longer than length");
  assert(parser->body_start <= len && "body starts after buffer end");
  assert(parser->mark < len && "mark is after buffer end");
  assert(parser->field_len <= len && "field has length longer than whole buffer");
  assert(parser->field_start < len && "field starts after buffer end");

  return(parser->nread);
}

int http_parser_finish(http_parser *parser)
{
  if (http_parser_has_error(parser) ) {
    return -1;
  } else if (http_parser_is_finished(parser) ) {
    return 1;
  } else {
    return 0;
  }
}

int http_parser_has_error(http_parser *parser) {
  return parser->cs == http_parser_error;
}

int http_parser_is_finished(http_parser *parser) {
  return parser->cs >= http_parser_first_final;
}
