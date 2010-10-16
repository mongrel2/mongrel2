local config = require 'config'
local sidereal = require 'sidereal'
local table = table
local ipairs = ipairs
local print = print

module 'db'

local DB = sidereal.connect('localhost', 6379)

local function format_message(user, msg)
    return 'From: ' .. user .. "\n" .. table.concat(msg, "\n")
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
    return DB:hexists("LOGINS", user) == 1
end

function inbox_count(user)
    print("INBOX:" .. user)
    return DB:llen("INBOX:" .. user)
end

function read_inbox(user)
    return DB:lpop("INBOX:" .. user)
end

function auth_user(user, password)
    local auth = DB:hget("LOGINS", user)
    return auth == password
end

