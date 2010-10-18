local coroutine = coroutine

module 'ui'

local SCREENS = {
    ['welcome'] = [[

 __  __ ____  ____  ____ ____  
|  \/  |___ \| __ )| __ ) ___| 
| |\/| | __) |  _ \|  _ \___ \ 
| |  | |/ __/| |_) | |_) |__) |
|_|  |_|_____|____/|____/____/ 
                               

Welcome to the Mongrel2 BBS.
]],
    ['name'] = "What's your name?",

    ['welcome_newbie'] = 'Awesome! Welcome to our little BBS. Have fun.\n',

    ['password'] = "What's your password?",

    ['motd'] = [[
MOTD: There's not much going on here currently, and we're mostly just trying out
this whole Lua with Mongrel2 thing. If you like it then help out by leaving a 
message and trying to break it. -- Zed

Enter to continue.]],

    ['menu'] = [[
---(((MAIN MENU))---
1. Leave a message.
2. Read messages left.
3. Send to someone else.
4. Read messages to you.
Q. Quit the BBS.
]],

    ['bye'] = "Alright, see ya later.",

    ['leave_msg'] = "Enter your message, up to 20 lines, then enter . by itself to end it:\n",

    ['read_msg'] = "These are left by different users:\n",

    ['menu_error'] = "That's not a valid menu option.",

    ['posted'] = "Message posted.",

    ['new_user'] = "Looks like you're new here. Type your password again and make it match.",

    ['bad_pass'] = "Right, I can't let you in unless you get the user and password right. Bye.",

    ['repeat_pass'] = "Password doesn't matched what you typed already, try again.",

    ['error'] = "We had an error, try again later.",
}

function display(conn, request, data)
    conn:reply_json(request, {type = 'screen', msg = data})
end

function ask(conn, request, data, pchar)
    conn:reply_json(request, {type = 'prompt', msg = data, pchar = pchar})
    return coroutine.yield()
end

function screen(conn, request, name)
    display(conn, request, SCREENS[name])
end

function prompt(conn, request, name)
    return ask(conn, request, SCREENS[name], '> ')
end

function exit(conn, request, name)
    conn:reply_json(request, {type = 'exit', msg = SCREENS[name]})
end


