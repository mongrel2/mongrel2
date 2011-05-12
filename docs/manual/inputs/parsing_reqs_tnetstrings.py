from mongrel2 import tnetstrings

def parse(msg):
    sender, conn_id, path, rest = msg.split(' ', 3)
    headers, rest = tnetstrings.parse(rest)
    body, _ = tnetstrings.parse(rest)

    if type(headers) is str:
        headers = json.loads(headers)

    return Request(sender, conn_id, path, headers, body)
