local config = require 'config'
local sidereal = require 'sidereal'
local table = table
local ipairs = ipairs

module 'db'

local LOGINS = {}
local MESSAGES = {}
local INBOXES = {}


function post_message(user, msg)
    table.insert(MESSAGES, 'From: ' .. user .. "\n" .. table.concat(msg, "\n"))
end

function send_to(to, from, msg)
    if not INBOXES[to] then INBOXES[to] = {} end
    table.insert(INBOXES[to], {from = from, msg = msg})
end


function register_user(user, password)
    LOGINS[user] = password
end

function user_exists(user)
    return LOGINS[user] ~= nil
end

function read_inbox(user)
    local inbox = INBOXES[user]
    INBOXES[user] = nil
    return inbox
end


function auth_user(user, password)
    local auth = LOGINS[user]
    return auth == password
end

function imessages()
    return ipairs(MESSAGES)
end
