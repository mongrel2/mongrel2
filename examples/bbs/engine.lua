local print = print
local pcall = pcall
local coroutine = coroutine
local ui = require 'ui'

module 'engine'

local STATE = {}

function run(conn, engine)
    while true do
        -- Get a message from the Mongrel2 server
        local good, request = pcall(conn.recv_json, conn)

        if good then
            local msg_type = request.data.type

            if msg_type == 'disconnect' then
            -- The client has disconnected
                print("disconnect", request.conn_id)
                STATE[request.conn_id] = nil
            elseif msg_type == 'msg' then
            -- The client has sent data
                local eng = STATE[request.conn_id]

                -- If the client hasn't sent data before, create a new engine.
                if not eng then
                    eng = coroutine.create(engine)
                    STATE[request.conn_id] = eng

                    -- Initialize the engine with the client's connection
                    coroutine.resume(eng, conn)
                end

                -- Pass the data on to the engine
                local good, error = coroutine.resume(eng, request)
                print("status", coroutine.status(eng))

                -- If the engine is done, stop tracking the client
                if coroutine.status(eng) == "dead" then
                    STATE[request.conn_id] = nil
                    if not good then
                    -- There was an error
                        print("ERROR", error)
                        ui.exit(conn, request, 'error')
                    end
                end
            else
                print("invalid message.")
            end

            print("eng", STATE[request.conn_id])
        end
    end
end
