package = "tir-template"
version = "0.9-3"

source = { }
source.url = "https://github.com/mongrel2/Tir/releases/download/v0.9-3/Tir-0.9-3.tar.gz"

description = {
   summary = "Tir Mongrel2/Lua Web Framework",
   detailed = "Tir is a framework for doing Web applications in Lua with the Mongrel2 web server.",
   homepage = "http://tir.mongrel2.org",
   license = "BSD",
   maintainer = "zedshaw@zedshaw.com",
}
dependencies = {
   "lua >= 5.1",
   "luaposix == 5.1.2",
   "markdown",
    "md5",
    "luajson",
    "lsqlite3",
    "telescope",
}
build = {
    type = "none",
    install = {
        bin = {
            'bin/tir',
        },

        lua = {
            ['tir.engine'] = 'tir/engine.lua',
            ['tir.error'] = 'tir/error.lua',
            ['tir.form'] = 'tir/form.lua',
            ['tir.m2'] = 'tir/m2.lua',
            ['tir.session'] = 'tir/session.lua',
            ['tir.strict'] = 'tir/strict.lua',
            ['tir.task'] = 'tir/task.lua',
            ['tir.util'] = 'tir/util.lua',
            ['tir.view'] = 'tir/view.lua',
            ['tir.web'] = 'tir/web.lua',
            ['tir.strict'] = 'tir/strict.lua',
            ['tir.testing'] = 'tir/testing.lua',
        }
    }
}
