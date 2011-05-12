import json

def parse_netstring(ns):
    len, rest = ns.split(':', 1)
    len = int(len)
    assert rest[len] == ',', "Netstring did not end in ','"
    return rest[:len], rest[len+1:]

def parse(msg):
    sender, conn_id, path, rest = msg.split(' ', 3)
    headers, rest = parse_netstring(rest)
    body, _ = parse_netstring(rest)

    headers = json.loads(headers)

    return uuid, id, path, headers, body

