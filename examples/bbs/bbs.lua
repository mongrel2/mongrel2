local strict = require 'strict'
local m2 = require 'mongrel2'
local ui = require 'ui'
local engine = require 'engine'
local db = require 'db'

local function new_user(conn, req, user, password)
    req = ui.prompt(conn, req, 'new_user')

    while req.data.msg ~= password do
        req = ui.prompt(conn, req, 'repeat_pass')
    end 

    db.register_user(user, password)

    ui.screen(conn, req, 'welcome_newbie')
end


local function read_long_message(conn, req, user)
    local msg = {}
    ui.screen(conn, req, 'leave_msg')
    req = ui.ask(conn, req, 'From: ' .. user .. '\n---', '')

    while req.data.msg ~= '.' and #msg < 24 do
        table.insert(msg, req.data.msg)
        req = ui.ask(conn, req, '', '')
    end

    return msg
end

local function message_iterator(db, i)
    if i > 0 then
        return i-1, db.message_read(i-1)
    end
end
local function messages(db)
    return message_iterator, db, db.message_count()
end

local MAINMENU = {
    ['1'] = function(conn, req, user)
        local msg = read_long_message(conn, req, user)

        db.post_message(user, msg)

        ui.screen(conn, req, 'posted')
    end,

    ['2'] = function(conn, req, user)
        ui.screen(conn, req, 'read_msg')

        for i, msg in messages(db) do
            ui.display(conn, req, ('---- #%d -----'):format(i))
            ui.display(conn, req, msg .. '\n')

            if i == 0 then
                ui.ask(conn, req, "Last message, Enter for MAIN MENU.", '> ')                
            else
                req = ui.ask(conn, req, "Enter, or Q to stop reading.", '> ')
                if req.data.msg:upper() == 'Q' then
                    ui.display(conn, req, "Done reading.")
                    return
                end
            end
        end

        ui.display(conn, req, "No more messages.")
    end,

    ['3'] = function (conn, req, user)
        req = ui.ask(conn, req, 'Who do you want to send it to?', '> ')
        local to = req.data.msg

        if db.user_exists(to) then
            local msg = read_long_message(conn, req, user)
            db.send_to(to, user, msg)

            ui.display(conn, req, "Message sent to " .. to)
        else
            ui.display(conn, req, 'Sorry, that user does not exit.')
        end
    end,

    ['4'] = function(conn, req, user)
        if db.inbox_count(user) == 0 then
            ui.display(conn, req, 'No messages for you. Try back later.')
            return
        end

        while db.inbox_count(user) > 0 do
            local msg = db.read_inbox(user)
            ui.display(conn, req, msg)
            ui.ask(conn, req, "Enter to continue.", '> ')
        end
    end,


    ['Q'] = function(conn, req, user)
        ui.exit(conn, req, 'bye')
    end
}


local function m2bbs(conn, req)
    ui.screen(conn, req, 'welcome')

    -- Have the user log in
    req = ui.prompt(conn, req, 'name')
    local user = req.data.msg

    req = ui.prompt(conn, req, 'password')
    local password = req.data.msg

    print('login attempt', user, password)

    if not db.user_exists(user) then
        new_user(conn, req, user, password)
    elseif not db.auth_user(user, password) then
        ui.exit(conn, req, 'bad_pass')
        return
    end

    -- Display the MoTD and wait for the user to hit Enter.
    ui.prompt(conn, req, 'motd')

    repeat
        req = ui.prompt(conn, req, 'menu')
        local selection = MAINMENU[req.data.msg:upper()]
        print("message:", req.data.msg)

        if selection then
            selection(conn, req, user)
        else
            ui.screen(conn, req, 'menu_error')
        end
    until req.data.msg:upper() == "Q"
end


do
  -- Load configuration properties
  local config = {}
  setfenv(assert(loadfile('config.lua')), config)()

  -- Connect to the Mongrel2 server
  print("connecting", config.sender_id, config.sub_addr, config.pub_addr)
  
  local ctx = mongrel2.new(config.io_threads)
  local conn = ctx:new_connection(config.sender_id, config.sub_addr, config.pub_addr)

  -- Run the BBS engine
  engine.run(conn, m2bbs)
end
