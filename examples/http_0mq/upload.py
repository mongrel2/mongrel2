from mongrel2 import handler
try:
    import json
except:
    import simplejson as json

import hashlib

sender_id = "82209006-86FF-4982-B5EA-D1E29E55D481"

conn = handler.Connection(sender_id, "tcp://127.0.0.1:9997",
                          "tcp://127.0.0.1:9996")
while True:
    print "WAITING FOR REQUEST"

    req = conn.recv()

    if req.is_disconnect():
        print "DISCONNECT"
        continue

    elif req.headers.get('x-mongrel2-upload-done', None):
        expected = req.headers.get('x-mongrel2-upload-start', "BAD")
        upload = req.headers.get('x-mongrel2-upload-done', None)

        if expected != upload:
            print "GOT THE WRONG TARGET FILE: ", expected, upload
            continue

        body = open(upload, 'r').read()
        print "UPLOAD DONE: BODY IS %d long, content length is %s" % (
            len(body), req.headers['content-length'])

        response = "UPLOAD GOOD: %s" % hashlib.md5(body).hexdigest()

    elif req.headers.get('x-mongrel2-upload-start', None):
        print "UPLOAD starting, don't reply yet."
        print "Will read file from %s." % req.headers.get('x-mongrel2-upload-start', None)
        continue

    else:
        response = "<pre>\nSENDER: %r\nIDENT:%r\nPATH: %r\nHEADERS:%r\nBODY:%r</pre>" % (
            req.sender, req.conn_id, req.path, 
            json.dumps(req.headers), req.body)

        print response

    conn.reply_http(req, response)

