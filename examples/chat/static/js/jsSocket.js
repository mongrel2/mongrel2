/*jslint browser: true, passfail: false, forin: true, undef: true, white: true*/

/*
* jsSocket: generic javascript socket API
*   (c) 2008 Aman Gupta (tmm1)
*
*   http://github.com/tmm1/jsSocket
*/

/*globals window, JSONstring, atob*/
function jsSocket(args) {
    if (this instanceof arguments.callee) {
        if (typeof this.init === "function") {
            this.init.apply(this, (args && args.callee) ? args : arguments);
        }
    } else {
        return new arguments.callee(arguments);
    }
}

// default location of jsSocket.swf
jsSocket.swf = "/static/flash/jsSocket.swf";

// dict of sockets
jsSocket.sockets = {};

// util functions
jsSocket.util = {
    observe: function (el, eventName, func) {
        if (el.addEventListener) {
            el.addEventListener(eventName, func, false);
        } else if (el.attachEvent) {
            el.attachEvent('on' + eventName, func);
        }
    },

    extend: function (orig, ext) {
        for (var name in ext) {
            orig[name] = ext[name];
        }
    },

    embedFlash: function (flashId, wrapperId, sizedReads) {
        var wrapper = document.createElement('div'),
            flashvars = "id=" + flashId + "&sizedReads=" + sizedReads;

        wrapper.id = wrapperId;
        wrapper.style.position = 'absolute';

        wrapper.innerHTML = '<embed id="' + flashId + '" width="1" height="1" flashvars="' + flashvars + '" autoplay="false" wmode="transparent" bgcolor="#ffffff" pluginspage="http://www.adobe.com/go/getflashplayer" type="application/x-shockwave-flash" src="' + jsSocket.swf + '" style="display: block;"/>';

        return wrapper;
    },

    nop: function () {
        return;
    }
};

// flash entry point for callbacks
jsSocket.callback = function (id, type, data) {
    var sock = jsSocket.sockets[id];
    sock.callback.apply(sock, [type, data]);
};


jsSocket.prototype = {
    num: null,      // numeric id

    id: null,       // dom id
    wrapper: null,  // dom container for flash object

    sock: null,     // flash object

    host: null,     // host == null means window.location.hostname
    port: null,     // port to connect to (443 is recommended to get through firewalls)
    path: null,

    // toggle packet reading mode between null terminated or size prefixed
    // null terminated is the default to be bacwards compatible with flash8/AS2s XMLSocket
    // flash9/AS3 supports size prefixed mode which allows sending raw data without base64
    sizedReads: false,

    init: function (opts) {
        var self = this;

        // update options
        if (opts) {
            jsSocket.util.extend(self, opts);
        }

        // don't autoconnect unless port is defined
        if (!self.port) {
            self.autoconnect = false;
        }

        if(!self.path) {
            self.path = '@';
        }

        // assign unique id
        if (!self.num) {
            if (!jsSocket.id) {
                jsSocket.id = 1;
            }

            self.num = jsSocket.id;
            jsSocket.id += 1;
            self.id =  'jsSocket_' + self.num;
        }

        // register with flash callback handler
        jsSocket.sockets[self.id] = self;

        // install flash socket
        self.wrapperId = 'jsSocketWrapper_' + this.num;
        self.wrapper = jsSocket.util.embedFlash(self.id, self.wrapperId, self.sizedReads);
        document.body.appendChild(self.wrapper);


        jsSocket.util.observe(window, 'beforeunload', self.close);
    },


    loaded: false,       // socket loaded into dom?
    connected: false,    // socket connected to remote host?
    debug: false,        // debugging enabled?

    autoconnect: true,   // connect when flash loads
    autoreconnect: true, // reconnect on disconnect

    // send ping every minute
    keepalive: function () {
        this.send({ type: 'ping' });
    },
    keepalive_timer: null,

    // reconnect logic (called if autoreconnect == true)
    reconnect: function () {
        this.log('reconnecting');

        if (this.reconnect_interval) {
            clearInterval(this.reconnect_interval);
        }

        this.reconnect_countdown = this.reconnect_countdown * 2;

        if (this.reconnect_countdown > 48) {
            this.log('reconnect failed, giving up');
            this.onStatus('failed');
            return;
        } else {
            this.log('will reconnect in ' + this.reconnect_countdown);
            this.onStatus('waiting', this.reconnect_countdown);
        }

        var secs = 0, self = this;

        this.reconnect_interval = setInterval(function () {
            secs += 1;
            var remain = self.reconnect_countdown - secs;

            if (remain === 0) {
                self.log('reconnecting now..');
                clearInterval(self.reconnect_interval);

                self.autoconnect = true;
                self.remove();
                self.init();
            } else {
                self.log('reconnecting in ' + remain);
                self.onStatus('waiting', remain);
            }
        }, 1000);
    },
    reconnect_interval: null,
    reconnect_countdown: 3,

    // wrappers for flash functions

    // open/connect the socket
    // happens automatically if autoconnect is true
    open: function (host, port) {
        if (host) {
            this.host = host;
        }
        if (port) {
            this.port = port;
        }

        this.host = this.host || window.location.hostname;
        if (!this.port) {
            this.log('error: no port specified');
        }

        this.onStatus('connecting');

        return this.sock.open(this.host, this.port);
    },
    connect: function () {
        this.open.apply(this, arguments);
    },

    // send/write data to the socket
    // if argument is an object, it will be json-ified
    send: function (data) {
        if (typeof data === "object") {
            data = JSONstring.make(data);
        }

        return this.sock.send(this.path + ' ' + data);
    },
    write: function () {
        this.send.apply(this, arguments);
    },

    // close/disconnect the socket
    close: function () {
        this.autoreconnect = true;
        if (this.loaded && this.connected) {
            this.sock.close();
        }
    },
    disconnect: function () {
        this.close.apply(this);
    },

    // uninstall the socket
    remove: function () {
        delete jsSocket.sockets[this.id];
        if (this.loaded && this.connected) {
            this.sock.close();
        }

        var swf = document.getElementById(this.wrapperId);
        swf.parentNode.removeChild(swf);
    },

    // debugging
    log: function () {
        if (!this.debug) {
            return;
        }

        var args = Array.prototype.slice.call(arguments, 0);

        if (this.logger) {
            this.logger.apply(null, [[this.id].concat(args)]);
        }
    },

    // Flash callback
    callback: function (type, data) {
        var self = this;

        setTimeout(function () { // wrap in setTimeout(.., 0) to free up flash's ExternalInterface
            data = self[type + "Callback"](self, data) || data;
            self[type](data);
        }, 0);
    },
    
    // JSSocket callbacks
    onLoadedCallback: function (self, data) {
        self.log('loaded');
        self.loaded = true;
        self.sock = document.getElementById(self.id);

        if (self.autoconnect) {
            self.connect();
        }
        
    },
    
    onOpenCallback: function (self, data) {
        if (data === true) {
            self.log('connected');
            self.connected = true;

            if (self.keepalive) {
                self.keepalive_timer = setInterval(function () {
                    self.keepalive.apply(self);
                }, 60 * 1000); 
            }

            self.reconnect_countdown = 3;

            if (self.reconnect_interval) {
                clearInterval(self.reconnect_interval);
            }

            self.onStatus('connected');

        } else {
            self.log('connect failed');
            if (self.autoreconnect) {
                self.reconnect();
            }
        }
        
    },
    
    onCloseCallback: function (self, data) {
        self.connected = false;
        self.log('disconnected');
        self.onStatus('disconnected');

        if (self.keepalive && self.keepalive_timer) {
            clearInterval(self.keepalive_timer);
            self.keepalive_timer = null;
        }

        if (self.autoreconnect) {
            self.reconnect();
        }
    },
    
    onDataCallback: function (self, data) {
        return atob(data);
    },

    // User Defined Callback Hooks
    onLoaded: jsSocket.util.nop,
    onOpen:   jsSocket.util.nop,
    onClose:  jsSocket.util.nop,
    onStatus: jsSocket.util.nop,
    onData:   jsSocket.util.nop
};

