import flash.external.ExternalInterface;

class jsSocket {
  private var sock:XMLSocket;
  private var id:String;

  private function calljs(type, data) {
    ExternalInterface.call('jsSocket.callback', this.id, type, data);
  }

  public function jsSocket(id) {
    this.id = id;

    ExternalInterface.addCallback('open',  this, open);
    ExternalInterface.addCallback('send',  this, send);
    ExternalInterface.addCallback('close', this, close);

    this.calljs('onLoaded', true);
  }

  public function open(host, port) {
    System.security.loadPolicyFile('xmlsocket://' + host + ':' + port);
    sock = new XMLSocket();
    
    var self = this;
    sock.onConnect = function(s) { self.calljs('onOpen', s); }
    sock.onData    = function(d) { self.calljs('onData', d); }
    sock.onClose   = function( ) { self.calljs('onClose');   }

    return sock.connect(host, port);
  }

  public function send(data) {
    if (data != 'null') // calling send() from js with no arguments sets data == 'null'
      return sock.send(data);
  }

  public function close() {
    sock.close();
    sock.onClose();
  }

  static function main(mc) {
    _root.jsSocket = new jsSocket(_root.id);
  }
}
