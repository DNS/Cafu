-- This is a helper function for `print`.
-- It redirects all output to Console.Print() and recurses into tables,
-- printing them with proper indentation.
-- Parameter `ti` is the table indent string that is used for printing the
-- current level of key/value pairs.
local function ti_print(ti, ...)
    local n = select("#", ...)

    for i = 1, n do
        local v = select(i, ...)

        if type(v) == "table" and #ti < 32 then
            Console.Print("{\n")
            for a, b in pairs(v) do
                if type(b) ~= "function" and type(b) ~= "userdata" then
                    Console.Print(ti .. "    ")
                    ti_print(ti .. "    ", a, ":=", b)
                end
            end
            Console.Print(ti)
            Console.Print("}")
        else
            Console.Print(tostring(v))
        end

        if i ~= n then Console.Print(" ") end
    end

    Console.Print("\n")
end


function print(...)
    ti_print("", ...)
end
