local config = require 'config'
local sidereal = require 'sidereal'
local table = table
local ipairs = ipairs
local print = print

module 'db'

local DB = sidereal.connect('localhost', 6379)

local INBOXES = {}
local LOGINS = {}

function post_message(user, msg)
    local formatted = 'From: ' .. user .. "\n" .. table.concat(msg, "\n")

    DB:rpush("MESSAGES", formatted)
end

function message_count()
    return DB:llen("MESSAGES")
end

function message_read(i)
    return DB:lindex("MESSAGES", i)
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

