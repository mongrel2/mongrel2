local strict = require 'strict'
local sidereal = require 'sidereal'
local table_concat = table.concat
local print = print
local assert = assert

module 'db'

local DB = assert(sidereal.connect('localhost', 6379))

function reconnect()
    return sidereal.connect('localhost', 6379)
end

local function format_message(user, msg)
    return ('From: %s\n%s'):format(user, table_concat(msg, "\n"))
end

function post_message(user, msg)
    DB:rpush("MESSAGES", format_message(user, msg))
end

function message_count()
    return DB:llen("MESSAGES")
end

function message_read(i)
    return DB:lindex("MESSAGES", i)
end

function send_to(to, from, msg)
    DB:lpush("INBOX:" .. to, format_message(from, msg))
end


function register_user(user, password)
    DB:hset("LOGINS", user, password)
end

function user_exists(user)
    return DB:hexists("LOGINS", user)
end

function inbox_count(user)
    return DB:llen("INBOX:" .. user)
end

function read_inbox(user)
    return DB:lpop("INBOX:" .. user)
end

function auth_user(user, password)
    return DB:hget("LOGINS", user) == password
end

