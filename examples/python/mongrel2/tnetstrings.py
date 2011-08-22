# Note this implementation is more strict than necessary to demonstrate
# minimum restrictions on types allowed in dictionaries.

def dump(data):
    if type(data) is long or type(data) is int:
        out = str(data)
        return '%d:%s#' % (len(out), out)
    elif type(data) is float:
        out = '%f' % data
        return '%d:%s^' % (len(out), out)
    elif type(data) is str:
        return '%d:' % len(data) + data + ',' 
    elif type(data) is dict:
        return dump_dict(data)
    elif type(data) is list:
        return dump_list(data)
    elif data == None:
        return '0:~'
    elif type(data) is bool:
        out = repr(data).lower()
        return '%d:%s!' % (len(out), out)
    else:
        assert False, "Can't serialize stuff that's %s." % type(data)


def parse(data):
    payload, payload_type, remain = parse_payload(data)

    if payload_type == '#':
        value = int(payload)
    elif payload_type == '}':
        value = parse_dict(payload)
    elif payload_type == ']':
        value = parse_list(payload)
    elif payload_type == '!':
        value = payload == 'true'
    elif payload_type == '^':
        value = float(payload)
    elif payload_type == '~':
        assert len(payload) == 0, "Payload must be 0 length for null."
        value = None
    elif payload_type == ',':
        value = payload
    else:
        assert False, "Invalid payload type: %r" % payload_type

    return value, remain

def parse_payload(data):
    assert data, "Invalid data to parse, it's empty."
    length, extra = data.split(':', 1)
    length = int(length)

    payload, extra = extra[:length], extra[length:]
    assert extra, "No payload type: %r, %r" % (payload, extra)
    payload_type, remain = extra[0], extra[1:]

    assert len(payload) == length, "Data is wrong length %d vs %d" % (length, len(payload))
    return payload, payload_type, remain

def parse_list(data):
    if len(data) == 0: return []

    result = []
    value, extra = parse(data)
    result.append(value)

    while extra:
        value, extra = parse(extra)
        result.append(value)

    return result

def parse_pair(data):
    key, extra = parse(data)
    assert extra, "Unbalanced dictionary store."
    value, extra = parse(extra)

    return key, value, extra

def parse_dict(data):
    if len(data) == 0: return {}

    key, value, extra = parse_pair(data)
    assert type(key) is str, "Keys can only be strings."

    result = {key: value}

    while extra:
        key, value, extra = parse_pair(extra)
        result[key] = value
  
    return result
    


def dump_dict(data):
    result = []
    for k,v in data.items():
        result.append(dump(str(k)))
        result.append(dump(v))

    payload = ''.join(result)
    return '%d:' % len(payload) + payload + '}'


def dump_list(data):
    result = []
    for i in data:
        result.append(dump(i))

    payload = ''.join(result)
    return '%d:' % len(payload) + payload + ']'


