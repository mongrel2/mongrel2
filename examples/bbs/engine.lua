local print = print
local coroutine = coroutine
local ui = require 'ui'

module 'engine'

local STATE = {}

function run(conn, engine)
    while true do
        local request = conn:recv_json()
        local data = request.data
        local done = false
        local eng = STATE[request.conn_id]
        local good
        local error

        if data.type == 'disconnect' then
            print("disconnect", request.conn_id)
            done = true
        elseif data.type == 'msg' then
            if eng then
                print "RESUME!"
                good, error = coroutine.resume(eng, request)
            else
                print "CREATE"
                eng = coroutine.create(engine)
                good, error = coroutine.resume(eng, conn, request)
            end

            done = coroutine.status(eng) == "dead"
            print("status", coroutine.status(eng))
            if error then
                print("ERROR", error)
            end
        else
            print("invalid message.")
        end

        print("done", done, "eng", eng)

        if done then
            ui.exit(conn, request, 'error')
            STATE[request.conn_id] = nil
        else
            STATE[request.conn_id] = eng
        end
    end
end
