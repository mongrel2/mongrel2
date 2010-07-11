

function andthen(fsm, event)
{
    var closure = function() {
        fsm.handle(event, this.json);
        return false;
    }

    return closure
}

function transthen(fsm, state, event) {
    var closure = function() {
        fsm.trans(state);
        fsm.handle(event, this.json);
        return false;
    }

    return closure
}



var FSM = function(states) 
{
    this.states = states;
    this.state = this.states.start;
    this.state_name = 'start';
    this.intervalId = null;
}

FSM.prototype.handle = function(event, data) 
{
    var res = null;
    var target = this.state[event];

    if(target) {
        if(this.onevent) this.onevent(this, event);

        try {
            res = target(this, data);
        } catch(e) {
            if(this.onexception) {
                this.onexception(this, e);
            } else {
                throw e;
            }
        }
    } else if(this.onunknownevent) {
        // the target does not exist and they have registered an event for that
        this.onunknownevent(this, event);
    }
    return res
}


FSM.prototype.trans = function(target) 
{
    var new_state = this.states[target]
    if(new_state) {
        if(this.ontrans) this.ontrans(this, target)
        this.state_name = target
        this.state = new_state
    } else if(this.onunknownstate) {
        this.onunknownstate(fsm, target)
    }
}

FSM.prototype.start = function(event) {
    this.states.start(this, event);
}


