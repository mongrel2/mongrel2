var wsUri="ws://home.jasonmmiller.org:6767/chatsocket"
var Chat = {
	socket: null,

	init: function (user, fsm) {
        if("MozWebSocket" in window) {
            Chat.socket = new MozWebSocket(wsUri);
        }
        else {
            Chat.socket = new WebSocket(wsUri)
        }
        Chat.socket.onopen = Chat.onOpen
        Chat.socket.onclose = Chat.onClose
        Chat.socket.onmessage = Chat.onData

        Chat.user = user;
        Chat.fsm = fsm;
        Chat.communicate=false;
	},

	onOpen: function () {
        window.setTimeout(Chat.ping,15000);
        Chat.fsm.handle('CONNECT', null);
	},

	onClose: function () {
        Chat.fsm.handle('CLOSE', null);
	},

	onData: function (evt) {
        data=evt.data;
        //alert(data);
		data = JSON.parse(data)
        Chat.fsm.handle(data.type.toUpperCase(), data);
        Chat.communicate=true;
	},

	send: function (message) { 
        Chat.communicate=true;
		Chat.socket.send(JSON.stringify({'type': 'msg', 'msg': message, 'user': Chat.user}));
        //alert(JSON.stringify({'type': 'msg', 'msg': message, 'user': Chat.user}));
	},
    ping: function () {
        if (!Chat.communicate) {
            Chat.socket.send(JSON.stringify({'type': 'ping'}));
        }
        Chat.communicate=false;
        if (Chat.socket.readyState == Chat.socket.OPEN) {
            window.setTimeout(Chat.ping,45000)
        }
    }
}
