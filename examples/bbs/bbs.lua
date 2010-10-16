local m2 = require 'mongrel2'
local ui = require 'ui'
local config = require 'config'
local engine = require 'engine'


local ctx = mongrel2.new(config.io_threads)

print("connecting", config.sender_id, config.sub_addr, config.pub_addr)

local conn = ctx:new_connection(config.sender_id, config.sub_addr, config.pub_addr)


local LOGINS = {}
local MESSAGES = {}
local DIRECTS = {}


local function post_message(user, msg)
    table.insert(MESSAGES, 'From: ' .. user .. "\n" .. table.concat(msg, "\n"))
end

local function send_to(to, from, msg)
    if not DIRECTS[to] then DIRECTS[to] = {} end
    table.insert(DIRECTS[to], {from = from, msg = msg})
end

local function new_user(conn, req, user, password)
    req = ui.prompt(conn, req, 'new_user')

    while req.data.msg ~= password do
        req = ui.prompt(conn, req, 'repeat_pass')
    end 

    LOGINS[user] = password
    ui.screen(conn, req, 'welcome_newbie')
end

local function read_long_message(conn, req, user)
    local msg = {}
    ui.screen(conn, req, 'leave_msg')
    req = ui.ask(conn, req, 'From: ' .. user .. '\n---', '')

    while req.data.msg ~= '.' do
        table.insert(msg, req.data.msg)
        req = ui.ask(conn, req, '', '')
    end

    return msg
end



local MAINMENU = {
    ['1'] = function(conn, req, user)
        local msg = read_long_message(conn, req, user)

        post_message(user, msg)

        ui.screen(conn, req, 'posted')
    end,


    ['2'] = function(conn, req, user)
        ui.screen(conn, req, 'read_msg')

        for i,msg in ipairs(MESSAGES) do
            ui.display(conn, req, msg .. '\n')
            ui.ask(conn, req, "Enter To Read More", '> ')
        end

        ui.display(conn, req, "No more messages.")
    end,

    ['3'] = function (conn, req, user)
        req = ui.ask(conn, req, 'Who do you want to send it to?', '> ')
        local to = req.data.msg

        if LOGINS[to] then
            local msg = read_long_message(conn, req, user)
            send_to(to, user, table.concat(msg, "\n"))

            ui.display(conn, req, "Message sent to " .. to)
        else
            ui.display(conn, req, 'Sorry, that user does not exit.')
        end
    end,

    ['4'] = function(conn, req, user)
        local inbox = DIRECTS[user]
        
        if inbox then
            for i, msg in ipairs(inbox) do
                ui.display(conn, req, "From: " .. msg.from .. "\n" .. msg.msg)
                ui.ask(conn, req, "Enter to continue.", '> ')
            end

            DIRECTS[user] = nil
            ui.display(conn, req, "Messages all read.")
        else
            ui.display(conn, req, 'No messages for you. Try back later.')
        end
    end,

    ['Q'] = function(conn, req, user)
        ui.exit(conn, req, 'bye')
    end
}


local function m2bbs(conn, req)
    req = ui.prompt(conn, req, 'welcome')
    user = req.data.msg

    req = ui.prompt(conn, req, 'password')
    password = req.data.msg

    local auth = LOGINS[user]

    if not auth then
        new_user(conn, req, user, password)
    elseif auth ~= password then
        ui.exit(conn, req, 'bad_pass')
        return
    end

    req = ui.prompt(conn, req, 'motd')
    
    if req.data.msg == "n" then
        ui.exit(conn, req, 'bye')
        return
    else
        repeat
            req = ui.prompt(conn, req, 'menu')
            local selection = MAINMENU[req.data.msg]
            print("message:", req.data.msg)

            if selection then
                selection(conn, req, user)
            else
                ui.screen(conn, req, 'menu_error')
            end
        until req.data.msg == "Q"
    end
end

engine.run(conn, m2bbs)

