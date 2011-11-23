require 'model.site'

local input = assert(Tir.load_file("./", "config.json"))
local config = assert(json.decode(input))

Generator:run(config.contents)

