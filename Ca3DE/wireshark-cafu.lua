--[[
    This is a Wireshark dissector for the Cafu Engine network protocol.
    For details, see:

      - Wireshark Users's Guide, "Lua Support in Wireshark":
        http://www.wireshark.org/docs/wsug_html_chunked/wsluarm.html

      - Tutorial to Wireshark dissectors written in Lua:
        http://sharkfest.wireshark.org/sharkfest.09/DT06_Bjorlykke_Lua%20Scripting%20in%20Wireshark.pdf

      - Wireshark Developer's Guide, "Packet dissection":
        http://www.wireshark.org/docs/wsdg_html_chunked/ChapterDissection.html

      - README file mentioned in the Developer's Guide:
        http://anonsvn.wireshark.org/wireshark/trunk/doc/README.developer

      - Wireshark wiki:
        http://wiki.wireshark.org/Development
--]]
do
    local CafuProto = Proto("Cafu", "Cafu Engine network protocol");


    -- "Connection-less" ("out-of-band") message types from client to server.
    local CS0 =
    {
        "CS0_NoOperation",
        "CS0_Ping",
        "CS0_Connect",
        "CS0_Info",
        "CS0_RemoteConsoleCommand"      -- A message consisting of a password and a command string that is to be executed by the server console interpreter.
    }

    -- "Connection-less" ("out-of-band") message types vom server to client.
    local SC0 =
    {
        "SC0_ACK",
        "SC0_NACK",
        "SC0_RccReply",                 -- String reply to a CS0_RemoteConsoleCommand message.
    }

    -- "Connection-established" message types from client to server.
    local CS1 =
    {
        "CS1_PlayerCommand",
        "CS1_Disconnect",
        "CS1_SayToAll",
        "CS1_WorldInfoACK",
        "CS1_FrameInfoACK",
    }

    -- "Connection-established" message types vom server to client.
    local SC1 =
    {
        "SC1_WorldInfo",
        "SC1_EntityBaseLine",
        "SC1_FrameInfo",
        "SC1_EntityUpdate",
        "SC1_EntityRemove",             -- A special case of the SC1_EntityUpdate message: No data follows, remove the entity from the frame instead.
        "SC1_DropClient",
        "SC1_ChatMsg",
    }


    local f = CafuProto.fields

    f.OOB      = ProtoField.uint32("Cafu.OOB", "OOB, out-of-band message", base.HEX)
    f.xy       = ProtoField.uint32("Cafu.xy", "???", base.HEX)

    f.CS0      = ProtoField.uint8("Cafu.CS0", "Message type", base.DEC, CS0)
    f.SC0      = ProtoField.uint8("Cafu.SC0", "Message type", base.DEC, SC0)
    f.CS1      = ProtoField.uint8("Cafu.CS1", "Message type", base.DEC, CS1)
    f.SC1      = ProtoField.uint8("Cafu.SC1", "Message type", base.DEC, SC1)

    -- SC1_WorldInfo
    f.SC1_WI_GameName  = ProtoField.stringz("Cafu.SC1_WI_GameName",  "Game  name")
    f.SC1_WI_WorldName = ProtoField.stringz("Cafu.SC1_WI_WorldName", "World name")
    f.SC1_WI_OurEntID  = ProtoField.uint32("Cafu.SC1_WI_OurEntID",  "Our ent ID")

    -- SC1_EntityBaseLine
    f.SC1_EBL_EntityID    = ProtoField.uint32("Cafu.SC1_EBL_EntityID",    "Entity ID    ")
    f.SC1_EBL_EntityType  = ProtoField.uint32("Cafu.SC1_EBL_EntityType",  "Entity type  ")
    f.SC1_EBL_EntityMapID = ProtoField.uint32("Cafu.SC1_EBL_EntityMapID", "Entity map ID")
    f.SC1_EBL_DeltaMsgLen = ProtoField.uint32("Cafu.SC1_EBL_DeltaMsgLen", "Delta msg len")
    f.SC1_EBL_DeltaMsg    = ProtoField.bytes ("Cafu.SC1_EBL_DeltaMsg",    "Delta msg    ")

    -- SC1_FrameInfo
    f.SC1_FI_NewFrameNr   = ProtoField.uint32("Cafu.SC1_FI_NewFrameNr", "New server frame nr.")
    f.SC1_FI_OldDeltaNr   = ProtoField.uint32("Cafu.SC1_FI_OldDeltaNr", "Old delta  frame nr.")

    -- SC1_EntityUpdate
    f.SC1_EU_EntityID     = ProtoField.uint32("Cafu.SC1_EU_EntityID",    "Entity ID    ")
    f.SC1_EU_DeltaMsgLen  = ProtoField.uint32("Cafu.SC1_EU_DeltaMsgLen", "Delta msg len")
    f.SC1_EU_DeltaMsg     = ProtoField.bytes ("Cafu.SC1_EU_DeltaMsg",    "Delta msg    ")

    -- SC1_EntityRemove
    f.SC1_ER_EntityID     = ProtoField.uint32("Cafu.SC1_ER_EntityID", "Entity ID")

    f.SequNr1  = ProtoField.uint32("Cafu.SequNr1",  "Sender sequence num", base.DEC, nil, 0x7FFFFFFF, "The senders sequence number.")
    f.AckFlag1 = ProtoField.uint32("Cafu.AckFlag1", "Ack-Flag",            base.DEC, nil, 0x80000000, "Does the sender want the receiver to ack this packet?")
    f.SequNr2  = ProtoField.uint32("Cafu.SequNr2",  "Last opp. sequ. num", base.DEC, nil, 0x7FFFFFFF, "The sequence number of the last packet that the sender has received from the opposite party.")
    f.AckFlag2 = ProtoField.uint32("Cafu.AckFlag2", "Ack-Flag",            base.DEC, nil, 0x80000000, "Odd/even flag, flipped by the sender if it has received a reliable packet from the opposite party.")

    f.Rest     = ProtoField.bytes("Cafu.Rest",    "Der noch unbekannte Rest")


    function CafuProto.init()
        -- Called before we make a pass through a capture file and dissect all its packets.
        -- e.g.:  packet_counter = 0
    end


    function CafuProto.dissector(buffer, pinfo, tree)
        local subtree = tree:add(CafuProto, buffer())
        local offset  = 0

        pinfo.cols.protocol = CafuProto.name

        if pinfo.src_port == 30000 then
            pinfo.cols.info = "Sv -> Cl"
            subtree:append_text(", Sv -> Cl")

            if buffer(offset, 4):uint() == 0xFFFFFFFF then
                pinfo.cols.info:append(", out-of-band")

                subtree:add(f.OOB, buffer(offset, 4)); offset = offset + 4
                subtree:add(f.xy,  buffer(offset, 4)); offset = offset + 4

                local MsgType = buffer(offset, 1):uint()
                subtree:add(f.SC0, buffer(offset, 1)); offset = offset + 1

                pinfo.cols.info:append(", " .. SC0[MsgType])
            else
                pinfo.cols.info:append(", connected")

                subtree:add(f.SequNr1,  buffer(offset, 4));
                subtree:add(f.AckFlag1, buffer(offset, 4)); offset = offset + 4

                subtree:add(f.SequNr2,  buffer(offset, 4));
                subtree:add(f.AckFlag2, buffer(offset, 4)); offset = offset + 4

                while offset < buffer:len() do
                    local MsgType = buffer(offset, 1):uint()
                    subtree:add(f.SC1,     buffer(offset, 1)); offset = offset + 1

                    pinfo.cols.info:append(", " .. SC1[MsgType])

                    if SC1[MsgType] == "SC1_WorldInfo" then
                        subtree:add(f.SC1_WI_GameName,  buffer(offset)); offset = offset + buffer(offset):stringz():len() + 1
                        subtree:add(f.SC1_WI_WorldName, buffer(offset)); offset = offset + buffer(offset):stringz():len() + 1

                        subtree:add(f.SC1_WI_OurEntID, buffer(offset, 4)); offset = offset + 4
                    elseif SC1[MsgType] == "SC1_EntityBaseLine" then
                        subtree:add(f.SC1_EBL_EntityID,    buffer(offset, 4)); offset = offset + 4
                        subtree:add(f.SC1_EBL_EntityType,  buffer(offset, 4)); offset = offset + 4
                        subtree:add(f.SC1_EBL_EntityMapID, buffer(offset, 4)); offset = offset + 4

                        local DeltaMsgLen = buffer(offset, 4):uint()
                        subtree:add(f.SC1_EBL_DeltaMsgLen, buffer(offset, 4)); offset = offset + 4
                        subtree:add(f.SC1_EBL_DeltaMsg, buffer(offset, DeltaMsgLen)); offset = offset + DeltaMsgLen
                    elseif SC1[MsgType] == "SC1_FrameInfo" then
                        subtree:add(f.SC1_FI_NewFrameNr, buffer(offset, 4)); offset = offset + 4
                        subtree:add(f.SC1_FI_OldDeltaNr, buffer(offset, 4)); offset = offset + 4
                    elseif SC1[MsgType] == "SC1_EntityUpdate" then
                        subtree:add(f.SC1_EU_EntityID, buffer(offset, 4)); offset = offset + 4

                        local DeltaMsgLen = buffer(offset, 4):uint()
                        subtree:add(f.SC1_EU_DeltaMsgLen, buffer(offset, 4)); offset = offset + 4
                        subtree:add(f.SC1_EU_DeltaMsg, buffer(offset, DeltaMsgLen)); offset = offset + DeltaMsgLen
                    elseif SC1[MsgType] == "SC1_EntityRemove" then
                        subtree:add(f.SC1_ER_EntityID, buffer(offset, 4)); offset = offset + 4
                    end
                end
            end
        else
            pinfo.cols.info = "Sv <- Cl"
            subtree:append_text(", Sv <- Cl")

            if buffer(offset, 4):uint() == 0xFFFFFFFF then
                pinfo.cols.info:append(", out-of-band")

                subtree:add(f.OOB, buffer(offset, 4)); offset = offset + 4
                subtree:add(f.xy,  buffer(offset, 4)); offset = offset + 4

                local MsgType = buffer(offset, 1):uint()
                subtree:add(f.CS0, buffer(offset, 1)); offset = offset + 1

                pinfo.cols.info:append(", " .. CS0[MsgType])
            else
                pinfo.cols.info:append(", connected")

                subtree:add(f.SequNr1,  buffer(offset, 4));
                subtree:add(f.AckFlag1, buffer(offset, 4)); offset = offset + 4

                subtree:add(f.SequNr2,  buffer(offset, 4));
                subtree:add(f.AckFlag2, buffer(offset, 4)); offset = offset + 4

                local MsgType = buffer(offset, 1):uint()
                subtree:add(f.CS1,     buffer(offset, 1)); offset = offset + 1

                pinfo.cols.info:append(", " .. CS1[MsgType])
            end
        end

        if offset < buffer:len() then
            subtree:add(f.Rest, buffer(offset))
        end
    end


    DissectorTable.get("udp.port"):add(30000, CafuProto)
end
