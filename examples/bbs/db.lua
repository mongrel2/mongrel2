local config = require 'config'
local sidereal = require 'sidereal'
local table = table
local ipairs = ipairs
local print = print

module 'db'


local function format_message(user, msg)
    return 'From: ' .. user .. "\n" .. table.concat(msg, "\n")
end

function post_message(user, msg)
    local DB = sidereal.connect('localhost', 6379)
    DB:rpush("MESSAGES", format_message(user, msg))
end

function message_count()
    local DB = sidereal.connect('localhost', 6379)
    return DB:llen("MESSAGES")
end

function message_read(i)
    local DB = sidereal.connect('localhost', 6379)
    return DB:lindex("MESSAGES", i)
end

function send_to(to, from, msg)
    local DB = sidereal.connect('localhost', 6379)
    DB:lpush("INBOX:" .. to, format_message(from, msg))
end


function register_user(user, password)
    local DB = sidereal.connect('localhost', 6379)
    DB:hset("LOGINS", user, password)
end

function user_exists(user)
    local DB = sidereal.connect('localhost', 6379)
    return DB:hexists("LOGINS", user) == 1
end

function inbox_count(user)
    local DB = sidereal.connect('localhost', 6379)
    return DB:llen("INBOX:" .. user)
end

function read_inbox(user)
    local DB = sidereal.connect('localhost', 6379)
    return DB:lpop("INBOX:" .. user)
end

function auth_user(user, password)
    local DB = sidereal.connect('localhost', 6379)
    local auth = DB:hget("LOGINS", user)
    return auth == password
end

