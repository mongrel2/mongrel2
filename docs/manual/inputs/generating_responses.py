def send(uuid, conn_id, msg):
    header = "%s %d:%s," % (uuid, len(str(conn_id)), str(conn_id))
    self.resp.send(header + ' ' + msg)


def deliver(uuid, idents, data):
    self.send(uuid, ' '.join(idents), data)

