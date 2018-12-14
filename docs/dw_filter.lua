function printTable(label, t)
    io.write(label)
    for k,v in pairs(t) do
        io.write(v .. " ")
    end
    io.write("\n")
end

-- This function strips wrapper <div> elements that are not needed
-- and cause redundant `container::` blocks in reStructuredText output.
function Div(div)
    if div.attr.identifier ~= "" then return end
    if #div.attr.attributes ~= 0 then return end
    if #div.attr.classes ~= 1 then return end

    c = div.attr.classes[1]
    if c ~= "level1" and c ~= "level2" and c ~= "level3" and c ~= "level4" and c ~= "level5" and
       c ~= "li" then
        return
    end

    -- print("\nneues <div>")
    -- -- print("id: " .. div.attr.identifier)
    -- -- printTable("attribs: ", div.attr.attributes)
    -- printTable("classes: ", div.attr.classes)

    return div.content
end
