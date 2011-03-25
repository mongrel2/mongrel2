import sys
import json
import simplejson
import cjson

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
    value, extra = parse_tnetstring(data)
    result.append(value)

    while extra:
        value, extra = parse_tnetstring(extra)
        result.append(value)

    return result

def parse_pair(data):
    key, extra = parse_tnetstring(data)
    assert extra, "Unbalanced dictionary store."
    value, extra = parse_tnetstring(extra)
    assert value, "Got an invalid value, null not allowed."

    return key, value, extra

def parse_dict(data):
    if len(data) == 0: return {}

    key, value, extra = parse_pair(data)
    result = {key: value}

    while extra:
        key, value, extra = parse_pair(extra)
        result[key] = value
  
    return result
    
def parse_tnetstring(data):
    payload, payload_type, remain = parse_payload(data)

    if payload_type == '#':
        value = int(payload)
    elif payload_type == '}':
        value = parse_dict(payload)
    elif payload_type == ']':
        value = parse_list(payload)
    elif payload_type == '!':
        value = payload == 'true'
    elif payload_type == '~':
        assert len(payload) == 0, "Payload must be 0 length for null."
        value = None
    elif payload_type == ',':
        value = payload
    else:
        assert False, "Invalid payload type: %r" % payload_type

    return value, remain


def dump_dict(data):
    result = []
    for k,v in data.items():
        result.append(dump_tnetstring(k))
        result.append(dump_tnetstring(v))

    payload = ''.join(result)
    return '%d:' % len(payload) + payload + '}'


def dump_list(data):
    result = []
    for i in data:
        result.append(dump_tnetstring(i))

    payload = ''.join(result)
    return '%d:' % len(payload) + payload + ']'


def dump_tnetstring(data):
    if type(data) is long or type(data) is int:
        out = str(data)
        return '%d:%s#' % (len(out), out)
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


TESTS = {
    '0:}': {},
    '0:]': [],
    '44:5:hello,32:11:12345678901#4:this,4:true!0:~]}': 
            {'hello': [12345678901, 'this', True, None]},
    '5:12345#': 12345,
    '12:this is cool,': "this is cool",
    '0:,': "",
    '4:true!': True,
    '5:false!': False,
    '24:5:12345#5:67890#5:xxxxx,]': [12345, 67890, 'xxxxx'],
    '42:7:VERSION,3:XML,6:METHOD,13:4:JSON,3:XML,]}': [],
    '27:3:one,3:two,5:three,4:four,]': [],
    '47:4:test,4:test,4:test,3:100#3:200#4:test,4:test,]': [],
    '34:3:age,3:100#4:name,11:Zed A. Shaw,}': [],
    "78:4:test,4:test,4:test,3:100#3:200#4:test,4:test,27:3:one,3:two,5:three,4:four,]]": [],
}

count = int(sys.argv[1])

for i in xrange(0,count):
    for data, expect in TESTS.items():
        payload, remain = parse_tnetstring(data)
        again = dump_tnetstring(payload)
        back, extra = parse_tnetstring(again)

        print "PAYLOAD:", repr(payload)
        print "AGAIN:", repr(again)
        print "BACK:", repr(back)
        print "EXTRA:", repr(extra)
        print "----"

