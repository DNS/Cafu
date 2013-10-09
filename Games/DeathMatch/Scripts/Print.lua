-- This is a helper function for `print`.
-- It redirects all output to Console.Print() and recurses into tables,
-- printing them with proper indentation.
-- Parameter `ti` is the table indent string that is used for printing the
-- current level of key/value pairs.
local function ti_print(ti, ...)
    local n = select("#", ...)

    for i = 1, n do
        local v = select(i, ...)

        Console.Print(tostring(v))

        if type(v) == "table" and #ti < 32 then
            Console.Print(" {\n")
            for a, b in pairs(v) do
                Console.Print(ti .. "    ")
                ti_print(ti .. "    ", a, ":=", b)
            end
            Console.Print(ti)
            Console.Print("}")
        end

        if (type(v) == "table" or type(v) == "userdata") and getmetatable(v) then
            Console.Print(" (has metatable)")
        end

        if i ~= n then Console.Print(" ") end
    end

    Console.Print("\n")
end


function print(...)
    ti_print("", ...)
end
