
function escapeHTML( html ) {
    var trans = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#x27;'
    };
    return (html + '').replace(/[&<>\"\']/g, function(c) { return trans[c]; });
}

function handlekeys(field, state, e)
{
    var keycode;

    if (window.event) {
        keycode = window.event.keyCode;
    } else if (e) {
        keycode = e.which;
    } else {
        return true;
    }

    if (keycode == 13) {
       state.handle('SEND');
       return false;
    } else {
       return true;
    }
}

function display(text) {
    var div = document.createElement("div");
    div.innerHTML = text;
    var cl = document.getElementById("terminal");
    cl.appendChild(div);
    div.scrollIntoView();
}

function change_prompt(new_prompt) {
    var p = document.getElementById("prompt")
    p.scrollIntoView();
    p.focus();
}



function clear_prompt() {
    change_prompt("");
    var prompt = document.getElementById("prompt");
    prompt.value = "";
}

function prompt(text, pchar) {
    clear_prompt();
    display(text);
    change_prompt(pchar);
}


state = new FSM({
    start: function(fsm, event) {
        display("Connecting to the Mongrel2 BBS....");
        fsm.trans('connecting');
        BBS.init(fsm);
    },

    connecting: {
        CONNECT: function(fsm, event) {
            display("Connected to Mongrel2 BBS.");
            BBS.send("connect");
            fsm.trans('connected');
        },
    },


    connected: {
        PROMPT: function(fsm, event) {
            prompt(event.msg, event.pchar);
            fsm.last_pchar = event.pchar;
        },

        EXIT: function(fsm, event) {
            clear_prompt();
            display(event.msg);
        },

        SCREEN: function(fsm, event) {
            display(event.msg);
        },

        CLOSE: function(fsm, event) {
            addMessage('<em>Disconnected, will reconnect...</em>');
            fsm.trans('connecting');
        },

        SEND: function(fsm, event) {
            var input = document.getElementById("prompt");
            BBS.send(input.value);
            display(fsm.last_pchar + input.value);
            clear_prompt();
        }
    }
});

