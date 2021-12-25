


--[ require 'json' ]--


function table_print(tt, indent, done)
  local done = done or {}
  local indent = indent or 0
  local space = string.rep(" ", indent)

  if type(tt) == "table" then
    local sb = {}

    for key, value in pairs(tt) do
      table.insert(sb, space) -- indent it

      if type (value) == "table" and not done [value] then
        done [value] = true
        table.insert(sb, key .. " = {\n");
        table.insert(sb, table_print(value, indent + 2, done))
        table.insert(sb, space) -- indent it
        table.insert(sb, "}\n");
      elseif "number" == type(key) then
        table.insert(sb, string.format("\"%s\" ", tostring(value)))
      else
        table.insert(sb, string.format(
            "%s = \"%s\"\n", tostring(key), tostring(value)))
       end
    end
    return table.concat(sb)
  else
    return tt .. "\n"
  end
end

function to_string(data)
    if "nil" == type(data) then
        return tostring(nil)
    elseif "table" == type(data) then
        return table_print(data)
    elseif  "string" == type(data) then
        return data
    else
        return tostring(data)
    end
end

function dump(data, name)
    print(to_string({name or "*", data}))
end

-- Helper function that loads a file into ram.
function load_file(from_dir, name)
    local intmp = assert(io.open(from_dir .. name, 'r'))
    local content = intmp:read('*a')
    intmp:close()

    return content
end

function update(target, source, keys)
    if keys then 
        for _, key in ipairs(keys) do
            target[key] = source[key]
        end
    else
        for k,v in pairs(source) do
            target[k] = v
        end
    end
end


-- useful for tables and params and stuff
function clone(source, keys)
    local target = {}
    update(target, source, keys)
    return target
end


-- Simplistic HTML escaping.
function escape(s)
    if s == nil then return '' end

    local esc, i = s:gsub('&', '&amp;'):gsub('<', '&lt;'):gsub('>', '&gt;')
    return esc
end

-- Simplistic URL decoding that can handle + space encoding too.
function url_decode(data)
    return data:gsub("%+", ' '):gsub('%%(%x%x)', function (s)
        return string.char(tonumber(s, 16))
    end)
end

-- Simplistic URL encoding
function url_encode(data)
    return data:gsub("\n","\r\n"):gsub("([^%w%-%-%.])", 
        function (c) return ("%%%02X"):format(string.byte(c)) 
    end)
end

-- Basic URL parsing that handles simple key=value&key=value setups
-- and decodes both key and value.
function url_parse(data, sep)
    local result = {}
    sep = sep or '&'
    data = data .. sep

    for piece in data:gmatch("(.-)" .. sep) do
        local k,v = piece:match("%s*(.-)%s*=(.*)")

        if k then
            result[url_decode(k)] = url_decode(v)
        else
            result[#result + 1] = url_decode(piece)
        end
    end

    return result
end


-- Loads a source file, but converts it with line numbering only showing
-- from firstline to lastline.
function load_lines(source, firstline, lastline)
    local f = io.open(source)
    local lines = {}
    local i = 0

    -- TODO: this seems kind of dumb, probably a better way to do this
    for line in f:lines() do
        i = i + 1

        if i >= firstline and i <= lastline then
            lines[#lines+1] = ("%0.4d: %s"):format(i, line)
        end
    end

    return table.concat(lines,'\n')
end


-- Parses a cookie string into a table
-- A given key will always be associated with a table containing 
-- one or more assigned values.
-- Note:  A cookie string may contain multiple cookies with the same key,
-- (which can happen if similar cookies exist for different paths, domains, etc.)
function parse_http_cookie(cookie)
    local cookies = {}

    if not cookie then return {} end

    local cookie_str = string.gsub(cookie, "%s*;%s*", ";")   -- remove extra spaces

    for k, v in string.gmatch(cookie_str, "([^;]+)=([^;]+)") do
        -- if the key already exists,then just insert the new value
        if cookies[k] then
            table.insert(cookies[k], v)
        -- otherwise, assign the new key to a table containing the one new value
        else
            cookies[k] = {v}
        end
    end

    return cookies		
end

function set_http_cookie(req, cookie)
    --key and value are required, everything else is optional
    assert(cookie and cookie.key and cookie.value, "cookie.key and cookie.value are required")
    --strip out cookie key/value delimiters
    local key = string.gsub(cookie.key, "([=;]+)", "")
    local value = string.gsub(cookie.value, "([=;]+)", "")
    
    local cookie_str = key .. '=' .. value
    
    -- if no path is specified, use the root
    cookie_str = cookie_str .. '; ' .. 'path=' .. (cookie.path or '/')
    
    if cookie.domain then
        cookie_str = cookie_str .. ';' .. 'domain=' .. cookie.domain
    end
    
    if cookie.expires then
        assert("number" == type(cookie.expires), "expires value must be a number - UNIX epoch seconds")
        cookie_str = cookie_str .. ';' .. 'expires=' .. os.date("%a, %d-%b-%Y %X GMT", cookie.expires)
    end		
      
    if cookie.http_only then cookie_str = cookie_str .. '; httponly' end
    
    if cookie.secure then cookie_str = cookie_str .. '; secure' end
    
    -- make sure we actually have a headers table before we go trying to set stuff
    req.headers = req.headers or {}
    -- make sure we have a set-cookie table to work with
    req.headers['set-cookie'] = req.headers['set-cookie'] or {}
    --insert the new cookie as a new set-cookie response header
    table.insert(req.headers['set-cookie'], cookie_str)	
end

