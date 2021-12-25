require 'model.site'

json = require 'dkjson'
Tir = { }
Tir.load_file = load_file
Tir.view = view

local input = assert(load_file("./", "config.json"))
local config = assert(json.decode(input))

Generator:run(config.contents)

