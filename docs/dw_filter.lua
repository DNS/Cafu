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


function Link(link)
    if false then
        print("")
        print("neuer Link:")

        io.write("content: ")
        for k,v in pairs(link.content) do
            if v.tag == "Str" then
                io.write("'" .. v.text .. "' ")
            else
                io.write(v.tag .. " ")
            end
        end
        io.write("\n")

        print("target: " .. link.target)
        print("title: " .. link.title)
        print("id: " .. link.attr.identifier)
        printTable("classes: ", link.attr.classes)
        printTable("attribs: ", link.attr.attributes)
     -- print("tag: " .. link.tag .. "  " .. link.t)
    end

    if #link.attr.classes > 0 and link.attr.classes[1] == "wikilink1" then
        -- print("~~~~~~~~~~~~~~")
        tg = ""
        for k,v in pairs(link.content) do
            if v.tag == "Str" then
                tg = tg .. v.text
            elseif v.tag == "Space" then
                tg = tg .. "_"
            else
                print("Unbekannter Inhalt in Link! " .. v.tag)
                tg = tg .. v.tag
            end
        end
        tg = tg:lower()
        -- print("---------> neu: " .. tg)

        return pandoc.RawInline("rst", ":ref:`" .. tg .. "`")
    end

    return link
end
