var Chat = {
	socket: null,

	init: function (user, fsm) {
		Chat.socket =  new jsSocket({
			hostname: '127.0.0.1',
			port: 	  6767,
            path:     '@chat',
		    onOpen:   Chat.onOpen,
	        onData:   Chat.onData,
	        onClose:  Chat.onClose
		});
        Chat.user = user;
        Chat.fsm = fsm;
	},

	onOpen: function () {
        Chat.fsm.handle('CONNECT', null);
	},

	onClose: function () {
        Chat.fsm.handle('CLOSE', null);
	},

	onData: function (data) {
		data = eval('(' + data + ')');
        Chat.fsm.handle(data.type.toUpperCase(), data);
	},

	send: function (message) { 
		Chat.socket.send({'type': 'msg', 'msg': message, 'user': Chat.user});
	},
}
