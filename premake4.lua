-- A solution contains projects, and defines the available configurations
--

SRC = { "src/**.h", "src/**.c" }

function test(test_name)
   project(test_name)
      kind("ConsoleApp")
      language("C")
      files {"tests/*.h", "tests/" .. test_name .. ".c"}
      flags { "ExtraWarnings", "FatalWarnings", "Symbols" }

      libdirs {"lib"}
      links { "sqlite3", "zmq", "m2" }
      includedirs { "src" }
      targetdir("build/tests")
      postbuildcommands { 
          "cp ../tests/sample.* tests",
          "cp ../tests/config.sqlite tests",
          "tests/" .. test_name 
      }
end

solution "Mongrel2"
   configurations { "Debug" }
   location "build"
 
   -- A project defines one build target
   project "m2"
      kind "StaticLib"
      language "C"
      files(SRC)
      excludes {"src/mongrel2.c"}
      links { "sqlite3", "zmq" }
      includedirs { "src" }
      targetdir "lib" 
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "ExtraWarnings", "FatalWarnings", "Symbols" }

      postbuildcommands {
          "sqlite3 ../tests/config.sqlite < ../src/config/config.sql",
          "sqlite3 ../tests/config.sqlite < ../src/config/example.sql",
          "sqlite3 ../tests/config.sqlite < ../src/config/mimetypes.sql"
      }

   test("listener_tests")
   test("config_tests")
   test("handler_tests")
   test("pattern_tests")
   test("routing_tests")
   test("db_tests")
   test("host_tests")
   test("proxy_tests")
   test("server_tests")
   test("dir_tests")
   test("listener_tests")
   test("register_tests")
   test("tst_tests")
   test("request_tests")

   project "mongrel2"
      kind "ConsoleApp"
      language "C"
      libdirs {"lib"}
      links { "sqlite3", "zmq", "m2" }
      includedirs { "src" }
      targetdir "bin"
      files { "src/mongrel2.c" }
      flags { "ExtraWarnings", "FatalWarnings", "Symbols" }

