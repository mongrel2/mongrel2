import hashlib
import base64

OP_CONT=0
OP_TEXT=1
OP_BIN=2
OP_CLOSE=8
OP_PING=9
OP_PONG=10

opcodes = (OP_CONT,OP_TEXT,OP_BIN,OP_CLOSE,OP_PING,OP_PONG)

def challenge(v):
    try:
        x=hashlib.sha1(v+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
        return base64.b64encode(x.digest())
    except:
        return ""

def deframe(data):
    #We rely on mongrel2 to have done the length calculation and read
    #Just the header and payload
    fin=(ord(data[0])&0x80)!=0
    rsvd=ord(data[0])&0x70
    opcode=ord(data[0])&0xf
    masked=(ord(data[1])&0x80)!=0
    leng=ord(data[1])&0x7f
    if not masked:
        raise Exception("Packet not masked!")
    payloadStart=6
    if leng == 126:
        payloadStart +=2
    elif leng == 127:
        payloadStart += 8
    maskKey=map(ord,data[payloadStart-4:payloadStart])
    dataOut=[]
    index=0
    for byte in data[payloadStart:]:
        dataOut.append(chr(ord(byte)^maskKey[index%4]))
        index+=1
    return (fin,rsvd,opcode,"".join(dataOut))

def frame(data,opcode=1,rsvd=0):
    header=''
    header+=chr(0x80|opcode|rsvd<<4)
    realLength=len(data)
    #print 'realLength',realLength
    if realLength < 126:
        dummyLength=realLength
    elif realLength < 2**16:
        dummyLength = 126
    else:
        dummyLength=127
    header+=chr(dummyLength)
    if dummyLength == 127:
        header += chr(realLength >> 56 &0xff)
        header += chr(realLength >> 48 &0xff)
        header += chr(realLength >> 40 &0xff)
        header += chr(realLength >> 32 &0xff)
        header += chr(realLength >> 24 & 0xff)
        header += chr(realLength >> 16 & 0xff)
    if dummyLength == 126 or dummyLength == 127:
        header += chr(realLength >> 8 & 0xff)
        header += chr(realLength & 0xff)
    return header+data
