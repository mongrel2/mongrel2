local tir = require 'tir.view'
local markdown = require 'markdown'
local posix = require 'posix'
local json = require 'json'
local util = require 'tir.util'

local RFC_822 = "%a, %d %b %Y %H:%M:%S %z"

local DATE_MATCH = "(%a+)_(%d+)_(%d+)" 

Generator = {
    output = 'output/',
    source = 'src/',
    global_data = "global.json"
}

function Generator:extract_target_name(template, in_name)
    local out_ext = template:match("(%.[a-zA-Z0-9]+)$")
    local out_front = in_name:match("(.+)%.[a-zA-Z0-9]+$")

    local out_name = out_front .. out_ext

    return out_name
end

function Generator:load_template(data, in_name)
    local newData = assert(json.decode(assert(Tir.load_file('./',self.global_data))))
    for k,v in pairs(newData) do data[k] = v end
    local template = Tir.view(data.template)
    return template, self:extract_target_name(data.template, in_name)
end


function Generator:load_data(in_name)
    local input = assert(Tir.load_file(self.source, in_name))
    local data = assert(json.decode(input))
    return data
end


function Generator:write(data, body, out_name)
    local out = assert(io.open(self.output .. out_name, 'w'))
    out:write(body(data))
    out:close()
end


function Generator:render(data, in_name)
    local body, out_name = self:load_template(data, in_name)
    self:write(data, body, out_name)
    return out_name
end


function Generator:extract_meta(stat, out_name, md)
    local month, day, year = out_name:match(DATE_MATCH)
    local date = nil
    local ctime = stat.ctime < stat.mtime and stat.ctime or stat.mtime

    if month then
        date = ("%s-%s-%s"):format(month, day, year)
    else
        date = os.date("%b-%d-%Y", ctime)
    end

    local intro_md = md:match("=+\n(.-)\n\n") 
    local intro = intro_md and markdown(intro_md) or ""

    local meta = {
        date = date,
        timestamp = ctime,
        pubdate = os.date(RFC_822, ctime),
        link = out_name,
        title = md:match("(.-)%s==") or "NO TITLE",
        intro = intro or ""
    }

    return meta
end

function sort_cmtime(a, b)
    local stat = posix.stat(a)
    local at = stat.ctime < stat.mtime and stat.ctime or stat.mtime

    stat = posix.stat(b)
    local bt = stat.ctime < stat.mtime and stat.ctime or stat.mtime

    return at > bt
end

function sort_digits(a, b)
    local anum = tonumber(string.match(a, "([0-9]+)"))
    local bnum = tonumber(string.match(b, "([0-9]+)"))

    if anum and bnum then
        return anum > bnum
    else
        return sort_cmtime(a, b)
    end
end

function sort_date(a, b)
    if a:match(DATE_MATCH) and b:match(DATE_MATCH) then
        return sort_cmtime(a, b)
    else
        return sort_digits(a, b)
    end
end


function Generator:render_contents(data, source, output)
    local base_strip = "^" .. self.source
    local results = {}

    local mdfiles = posix.glob(source .. "*.md")

    if mdfiles then
        table.sort(mdfiles, sort_date)

        for _, path in ipairs(mdfiles) do
            local target = path:gsub(base_strip, "")
            local content = Tir.load_file(self.source, target)
            data.contents = markdown(content)
            local out_name = nil

            if not rawget(data, 'meta') then
                out_name = self:render(data, target)
            else
                out_name = self:extract_target_name(data.template, target)
            end

            local stat = posix.stat(path)
            results[#results + 1] = self:extract_meta(stat, out_name, content)
        end
    end

    return results
end


function Generator:run(dirs)
    local base_strip = "^" .. self.source

    for _, dir in ipairs(dirs) do
        local source = self.source .. dir
        local output = self.output .. dir

        posix.mkdir(output)
        local configs = posix.glob(source .. '*.json')

        if configs then
            for _, path in ipairs(configs) do
                local target = path:gsub(base_strip, "")
                local data = self:load_data(target)
                
                if data.contents then
                    data.contents = self:render_contents(data.contents, source, output)
                end

                self:render(data, target)
            end
        end
    end
end


