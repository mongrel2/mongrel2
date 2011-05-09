def http_response(body, code, status, headers):
    payload = {'code': code, 'status': status, 'body': body}
    headers['Content-Length'] = len(body)
    payload['headers'] = "\r\n".join('%s: %s' % (k,v) for k,v in
                                     headers.items())

    return HTTP_FORMAT % payload

