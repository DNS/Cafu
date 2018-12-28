function printTable(label, t)
    io.write(label)
    for k,v in pairs(t) do
        io.write(v .. " ")
    end
    io.write("\n")
end

-- This function strips wrapper <div> elements that are not needed
-- and cause redundant `container::` blocks in reStructuredText output.
-- Examples:
--     <div class="level1"><p>...</p><p>...</p></div> becomes <p>...</p><p>...</p>
--     <li><div class="li">list item text</div></li> becomes <li>list item text</li>
--     <div class="table"><table>...</table></div> becomes <table>...</table>
function Div(div)
    if div.attr.identifier ~= "" then return end
    if #div.attr.attributes ~= 0 then return end
    if #div.attr.classes < 1 then return end

    c = div.attr.classes[1]
    if c ~= "level1" and c ~= "level2" and c ~= "level3" and c ~= "level4" and c ~= "level5" and
       c ~= "li" and c ~= "table" then
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

    -- Turns <a href="..." class="wikilink1"> HTML hyperlinks into internal references in RST, where appropriate.
    -- For example,
    -- turn <a href="/general:developer_faq#how_do_i_dynamically_reload_the_map_script_in-game" class="wikilink1" title="general:developer_faq">update a map script while the game is running</a>
    -- into :ref:`update a map script while the game is running <how_do_i_dynamically_reload_the_map_script_in-game>`
    --
    -- See https://groups.google.com/d/msg/pandoc-discuss/Nqv_6TyQZa0/_7lf0O-PCAAJ for details.
    if #link.attr.classes > 0 and link.attr.classes[1] == "wikilink1" then
        -- print("~~~~~~~~~~~~~~")
        local content = ""
        for k,v in pairs(link.content) do
            if v.tag == "Str" then
                content = content .. v.text
            elseif v.tag == "Space" then
                content = content .. " "
            else
                print("Unbekannter Inhalt in Link! " .. v.tag)
                content = content .. v.tag
            end
        end

        local target

        if link.target:find("#") then
            target = link.target:gsub(".*%#", "")
            -- print("### mit  Anchor: " .. content .. " --> " .. target)
        else
            -- We should never get here: after `fix_a_href_attribs.py`, our input
            -- files no longer contain any corresponding link targets.
            target = content:lower():gsub(" ", "_")
            print("### ohne Anchor: " .. content .. " --> " .. target)
        end

        return pandoc.RawInline("rst", ":ref:`" .. content .. " <" .. target .. ">`")
    end

    -- Replace <a href="/_detail/..." class="media"><img .../></a> with <img .../>.
    -- DokuWiki produced low-res editions of the images and linked to their full-res counterparts.
    -- As we always use the full-res editions, we don't need the wrapper link.
    if #link.attr.classes > 0 and link.attr.classes[1] == "media" then
        if link.target:sub(1, 9) == "/_detail/" or link.target:sub(1, 9) == "/lib/exe/" then
            return link.content
        end
    end

    return link
end


-- Fixes the paths to our images.
function Image(img)
    if false then
        print("")
        print("neues Bild:")

        io.write("caption: ")
        for k,v in pairs(img.caption) do
            if v.tag == "Str" then
                io.write("'" .. v.text .. "' ")
            else
                io.write(v.tag .. " ")
            end
        end
        io.write("\n")

        print("src: " .. img.src)
        print("title: " .. img.title)
        print("id: " .. img.attr.identifier)
        printTable("classes: ", img.attr.classes)
        printTable("attribs: ", img.attr.attributes)
     -- print("tag: " .. img.tag .. "  " .. img.t)
    end

    if img.src:find("lib/images/smileys/icon_exclaim.gif") then return pandoc.Strong("(!)") end
    if img.src:find("lib/images/smileys/icon_question.gif") then return pandoc.Strong("(?)") end
    if img.src:find("lib/images/smileys/icon_wink.gif") then return pandoc.Str("😉") end
    if img.src:find("lib/images/smileys/icon_lol.gif") then return pandoc.Str("😃") end
    if img.src:find("lib/images/smileys/icon_smile.gif") then return pandoc.Str("🙂") end
    if img.src:find("lib/images/smileys/icon_biggrin.gif") then return pandoc.Str("😀") end
    if img.src:find("lib/images/smileys/fixme.gif") then return pandoc.Strong("(FIXME!)") end

    if img.src:find("/_media/") then
        img.src = img.src:gsub("/_media/", "/images/")
        img.src = img.src:gsub(":", "/")
        img.src = img.src:gsub("%?.*", "")
        -- print(" ----> " .. img.src)
    end

    if img.src:find("/lib/exe/") then
        img.src = img.src:gsub(".*%%2F", "/images/")
        -- print(" ----> " .. img.src)
    end

    return img
end


function CodeBlock(cb)
    if false then
        print("")
        print("neuer CodeBlock:")
        print("text: " .. cb.text)
        print("id: " .. cb.attr.identifier)
        printTable("classes: ", cb.attr.classes)
        printTable("attribs: ", cb.attr.attributes)
     -- print("tag: " .. cb.tag .. "  " .. cb.t)
    end

    if #cb.attr.classes > 0 and cb.attr.classes[1] == "code" then
        table.remove(cb.attr.classes, 1)
    end
    if #cb.attr.classes > 0 and cb.attr.classes[1] == "dos" then
        cb.attr.classes[1] = "doscon"
    end

    return cb
end
