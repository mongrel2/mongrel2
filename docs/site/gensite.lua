local tir = require 'tir.view'
local markdown = require 'markdown'
local posix = require 'posix'
local json = require 'json'
local util = require 'tir.util'


Generator = {
    output = 'output/',
    source = 'src/',
}


function Generator:load_template(data, in_name)
    local out_ext = data.template:match("(%.[a-zA-Z0-9]+)$")
    local out_front = in_name:match("(.+)%.[a-zA-Z0-9]+$")

    local out_name = out_front .. out_ext
    local template = Tir.view(data.template)

    return template, out_name
end


function Generator:load_data(in_name)
    local input = assert(Tir.load_file(self.source, in_name))
    local data = assert(json.decode(input))
    return data
end


function Generator:write(data, body, out_name)
    local out = assert(io.open(self.output .. out_name, 'w'))
    out:write(body(data))
end


function Generator:render(data, in_name)
    local body, out_name = self:load_template(data, in_name)
    self:write(data, body, out_name)
    return out_name
end


function Generator:extract_meta(stat, out_name, md)
    local month, day, year = out_name:match("(%a+)_(%d+)_(%d+)")
    local date = nil

    if month then
        date = ("%s-%s-%s"):format(month, day, year)
    else
        date = posix.strftime("%b-%d-%Y", stat.ctime)
    end

    local intro = markdown(md:match("=+\n(.-)\n\n"))

    local meta = {
        date = date,
        link = out_name,
        title = md:match("(.-)%s==") or "NO TITLE",
        intro = intro or ""
    }

    return meta
end


function Generator:render_contents(data, source, output)
    local base_strip = "^" .. self.source
    local results = {}

    local mdfiles = posix.glob(source .. "*.md")

    if mdfiles then
        table.sort(mdfiles, function(a,b) return a>b end)
        for _, path in ipairs(mdfiles) do
            local target = path:gsub(base_strip, "")
            local template, out_name = self:load_template(data, target)

            local content = Tir.load_file(self.source, target)
            data.contents = markdown(content)

            out_name = self:render(data, target)

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



local input = assert(Tir.load_file("./", "config.json"))
local config = assert(json.decode(input))

Generator:run(config.contents)

