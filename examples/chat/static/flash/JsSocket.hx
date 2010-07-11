class JsSocket {
  static var socket:flash.net.Socket;
  static var id:String;
  static var buffer:String = "";
  static var sizedReads:Bool = false;
  static var packetSize:Int = -1;
  static var slash:EReg = ~/\\/g;

  private static function calljs(type, data:Dynamic) {
    flash.external.ExternalInterface.call('jsSocket.callback', id, type, data);
  }

  private static function debug(data:Dynamic) {
    flash.external.ExternalInterface.call('console.log', data);
  }

  static function main() {
    id = flash.Lib.current.loaderInfo.parameters.id;
    sizedReads = flash.Lib.current.loaderInfo.parameters.sizedReads;

    flash.external.ExternalInterface.addCallback("open", open);
    flash.external.ExternalInterface.addCallback("send", send);
    flash.external.ExternalInterface.addCallback("close", close);

    calljs('onLoaded', true);
  }

  static function open(host, port) {
    flash.system.Security.loadPolicyFile('xmlsocket://' + host + ':' + port);
    socket = new flash.net.Socket();

    socket.addEventListener(flash.events.Event.CONNECT, function(s){
      calljs('onOpen', true);
    });

    socket.addEventListener(flash.events.Event.CLOSE, function(e){
      calljs('onClose', null);
    });

    socket.addEventListener(flash.events.IOErrorEvent.IO_ERROR, function(e){
      calljs('onClose', e.text);
    });

    socket.addEventListener(flash.events.SecurityErrorEvent.SECURITY_ERROR, function(e){
      calljs('onClose', e.text);
    });

    socket.addEventListener(flash.events.ProgressEvent.SOCKET_DATA, function(d){
      if (sizedReads) {
        while (socket.bytesAvailable > 0) {
          // figure out the packet size
          if (packetSize > -1) {
            // we have it already
          } else if (socket.bytesAvailable >= 2) {
            // read the packet size
            packetSize = socket.readShort();
          } else {
            // lets wait
            break;
          }

          // read the packet, if possible
          if (socket.bytesAvailable >= packetSize) {
            calljs('onData', slash.replace(socket.readUTFBytes(packetSize), '\\\\'));
            packetSize = -1;
          } else {
            break;
          }
        }
      } else {
        var size = socket.bytesAvailable;
        var data = new flash.utils.ByteArray();
        socket.readBytes(data);

        buffer += data.toString();

        if (buffer.indexOf("\x00") > -1) {
          var packets = buffer.split("\x00");
          while (packets.length > 1) {
            calljs('onData', packets.shift());
          }
          buffer = packets.shift();
        }
      }
    });

    return socket.connect(host, Std.parseInt(port));
  }

  static function send(data:String) {
    if (socket.connected && data.length > 0) {
      var t = new flash.utils.Timer(0, 1);
      t.addEventListener(flash.events.TimerEvent.TIMER, function(d){
        socket.writeUTFBytes(data);
        socket.writeByte(0);
        socket.flush();
      });
      t.start();

      return true;
    } else
      return false;
  }

  static function close() {
    if (socket.connected)
      socket.close();
  }
}
