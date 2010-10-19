var BBS = {
	socket: null,

	init: function (fsm) {
		BBS.socket =  new jsSocket({
			hostname: '127.0.0.1',
			port: 	  6767,
            path:     '@bbs',
		    onOpen:   BBS.onOpen,
	        onData:   BBS.onData,
	        onClose:  BBS.onClose
		});
        BBS.fsm = fsm;
	},

	onOpen: function () {
        BBS.fsm.handle('CONNECT', null);
	},

	onClose: function () {
        BBS.fsm.handle('CLOSE', null);
	},

	onData: function (data) {
		data = eval('(' + data + ')');
        BBS.fsm.handle(data.type.toUpperCase(), data);
	},

	send: function (message) { 
		BBS.socket.send({'type': 'msg', 'msg': message});
	},
}
