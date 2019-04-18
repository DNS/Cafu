.. _creating_teleporter_stations:

Creating Teleporter Stations
============================

This tutorial shows how a network of teleporter stations can be added to
a game map: A set of teleporter stations is distributed throughout the
map. Each station has a graphical user interface that the player can use
to select the station that he wants to teleport to, and to activate the
teleportation.

The tutorial involves the placement of new entities into the map, GUI
scripting and map scripting. As such, it is also a gentle introduction
into Cafu game scripting.

All tasks are demonstrated using the sample map **BPWxBeta** that is
included with Cafu.

Creating the teleporter controls GUI
------------------------------------

|The GUI for the controls of the teleporter station.| Each teleporter
station is supposed to have a GUI that the player can use to control
which other station he wants to teleport to, and to activate the
teleportation.

We use the GUI editor component of CaWE in order to create the new GUI,
an example is shown in the image.

| The next section explains how the new GUI is scripted to properly
  react to player input: The ``<`` and ``>`` buttons should increment or
  decrement the number of the station that the player is being
  teleported to, and the big button below should actually trigger the
  teleportation.

Writing custom code for the teleporter controls GUI
---------------------------------------------------

When the GUI was saved in the GUI editor, it automatically wrote two
files:

-  ``Teleporter_init.cgui`` and
-  ``Teleporter_main.cgui``

The first file contains the actual data that the GUI editor loads and
saves, the second file is for your hand-made customizations. In short,
note that the first file is overwritten whenever the GUI editor saves a
GUI file, whereas the second file is only created once as an empty stub
file. The GUI editor touches the second file never again, so that you
can use it for hand-written customizations.

We now edit file ``Teleporter_main.cgui`` and enter the following script
code:

.. code:: lua

   dofile("Games/DeathMatch/GUIs/Teleporter_init.cgui");
    
    
   function ButtonMinus:OnMouseEnter()
       self:set("borderColor", 1.0, 0.0, 0.0, 1.0);
       self:interpolate("textScale", 2.2, 2.4, 500);
   end
    
   function ButtonMinus:OnMouseLeave()
       self:set("borderColor", 0, 0.333333, 0.490196, 0.5);
       self:interpolate("textScale", 2.4, 2.2, 500);
   end
    
   function ButtonMinus:OnMouseButtonUp()
       local NodeNr=tonumber(DestNode:get("text"));
    
       if (NodeNr>1) then
           DestNode:set("text", NodeNr-1);
       end
    
       return true;
   end
    
    
   function ButtonPlus:OnMouseEnter()
       self:set("borderColor", 1.0, 0.0, 0.0, 1.0);
       self:interpolate("textScale", 2.2, 2.4, 500);
   end
    
   function ButtonPlus:OnMouseLeave()
       self:set("borderColor", 0, 0.333333, 0.490196, 0.5);
       self:interpolate("textScale", 2.4, 2.2, 500);
   end
    
   function ButtonPlus:OnMouseButtonUp()
       local NodeNr=tonumber(DestNode:get("text"));
    
       if (NodeNr<MAX_NODES) then
           DestNode:set("text", NodeNr+1);
       end
    
       return true;
   end
    
    
   function ButtonGo:OnMouseEnter()
       self:set("borderColor", 1.0, 0.0, 0.0, 1.0);
       self:interpolate("textScale", 0.5, 0.52, 500);
   end
    
   function ButtonGo:OnMouseLeave()
       self:set("borderColor", 0, 0.333333, 0.490196, 0.498039);
       self:interpolate("textScale", 0.52, 0.5, 500);
   end
    
   function ButtonGo:OnMouseButtonUp()
       local origNr=OUR_NODE_NR;
       local destNr=DestNode:get("text");
    
       game.runMapCmd("teleport(" .. origNr .. ", " .. destNr .. ");");
       return true;
   end
    
    
   -- This function is called as soon as the entity related to this GUI has been initialized.
   -- Note that our entities must have names like "teleporter_2_of_5" for this to work.
   function OnEntityInit()
       -- Figure out the total size of the teleportation network (number of nodes)
       -- that this station is in, and which number this node/station has.
       OUR_NODE_NR, MAX_NODES=string.match(gui:getEntityName(), "(%d+)_of_(%d+)");
    
       OUR_NODE_NR=tonumber(OUR_NODE_NR) or 1;
       MAX_NODES  =tonumber(MAX_NODES) or 3;
    
       InfoStation:set("text", "Station " .. OUR_NODE_NR);
       DestNode:set("text", (OUR_NODE_NR % MAX_NODES)+1);
   end

The first line of the script loads and runs the previously mentioned
first file, that initializes the basics of the GUI. As such, the first
line is crucial because it is responsibe for “connecting” both files.

After the GUI script is fully loaded, the Cafu Engine initializes the
GUI by

-  first calling all ``OnInit()`` methods of all windows (they are
   defined in ``Teleporter_init.cgui``),
-  then calling all ``OnInit2()`` methods of each window (of which we
   have none at all, but if we had, they were in
   ``Teleporter_main.cgui``),
-  and finally calling function ``OnEntityInit()`` for 3D in-game GUIs
   such as ours.

The ``OnEntityInit()`` method is very important as is determines the
number of this teleporter station (or “node”) and the total number of
nodes, and saves both values in global variables for future use. It also
updates the “Station XY” text of the ``InfoStation`` window that
indicates to the player at which station he is currently standing, and
sets a reasonable default for the destination station that we're
teleporting to.

All other methods are event handlers that the Cafu engine calls whenever
the related event occurs. The ``OnMouseEnter()`` and ``OnMouseLeave()``
methods are only used for adding some visual eye candy, and so we won't
further discuss them.

The most interesting method is ``ButtonGo:OnMouseButtonUp()``, that is
called when the player presses the button that is supposed to activate
the teleporter. However, note that this GUI script is like implementing
the control software that runs on the teleporter station, just like a
desktop program that would run on the station if the teleporter was
real. As such, the GUI cannot implement the teleportation itself, but it
must ask the map script to do it.

The GUI can use the ``runMapCmd()`` function of the ``game`` object in
order to ask the game script to do something. We will later write a game
script function ``teleport(origNr, destNr)`` that does precisely that:
Teleport a player from station ``origNr`` to station ``destNr``, and
thus when the player presses the button, we have this function called
here.

Placing teleporter stations in the game map
-------------------------------------------

|The pair of entities for one teleporter station.| The next task is
quite simple: We place the new teleporters into the map. To do so, use
the :ref:`New Entity <the_new_entity_tool>` tool in order to create two
new entities whereever you want to have a teleporter station:

-  One ``static_detail_model`` that shows the big screen with the
   controls GUI, and
-  one ``info_generic`` entity that indicates the source and destination
   point of the teleportation.

Note that you *must* name the ``static_detail_model`` with a name of the
form ``somename_X_of_Y``, where X is the number of this station and Y is
the total number of the stations. This is because our GUI script above
has been written to figure out these numbers from the name of the
entity.

Implementing teleport() in the map script
-----------------------------------------

The final task is to write the above mentioned ``teleport()`` function
in the map script: For the **BPWxBeta** example map, we create or open
``Games/DeathMatch/Worlds/BPWxBeta.lua``, and enter the following code:

.. code:: lua

   -- It's more readable to call wait(x) rather than coroutine.yield(x). 
   wait=coroutine.yield;
    
   -- This function reloads and runs this script again.
   -- Useful for working with and testing the script "online",
   -- i.e. while the engine is running and without reloading the map!
   function reloadscript()
       dofile("Games/DeathMatch/Worlds/BPWxBeta.lua");
   end
    
    
   -- This function is called by the teleporter GUI script when the user wants to teleport.
   -- origNr and destNr are numbers like 1 or 5 that indicate from and to which node in the
   -- teleportation network the teleportation should occur.
   --
   -- TODO/FIXME: It would be nice if we could learn from an additional parameter which
   --   (player) entity was actually operating the teleporter controls (i.e. who pressed
   --   the "Go" button on the related GUI).
   function teleport(origNr, destNr)
       -- Prevent re-entrancy in the case that this method is called very quickly in succession.
       -- This can happen with the current EntHumanPlayerT code, which is sort of a bug.
       if (isTeleporting) then return true end
    
       local ox, oy, oz=_G["info_generic_" .. origNr]:GetOrigin();
       local dx, dy, dz=_G["info_generic_" .. destNr]:GetOrigin();
    
       Console.Print("Teleporting from node " .. origNr .. " to node " .. destNr .. ", ");
       Console.Print("which is at coordinate (" .. dx .. ", " .. dy .. ", " .. dz .. ").\n");
    
       -- Teleport all entities with name "Player*" to the destination node.
       for PlayerNr=1, 99 do
           local PlayerEnt=_G["Player" .. PlayerNr];
    
           if not PlayerEnt then break end
    
           local px, py, pz=PlayerEnt:GetOrigin();
    
           local ax=px-ox;
           local ay=py-oy;
           local az=pz-oz;
    
           if (pz>oz and math.sqrt(ax*ax + ay*ay + az*az) < 2000.0) then
               -- VARIANT 1:
               -- When teleporting PlayerEnt to (dx+ax, dy+ay, dz+az), it is safe to teleport
               -- multiple players all at the same time (continuing the loop), as their
               -- relative positioning doesn't change.
               -- However, depending on the map geometry, teleporting to (dx+ax, dy+ay, dz+az)
               -- could teleport the player into solid matter, as it requires that at the dest
               -- there is as much free space (in the same absolute direction) as at the origin.
               -- PlayerEnt:SetOrigin(dx+ax, dy+ay, dz+az);
    
               -- VARIANT 2:
               -- When teleporting PlayerEnt to (dx, dy, dz+az), the results are much easier
               -- to foresee, but this requires teleporting only one player at a time.
               PlayerEnt:SetOrigin(dx, dy, dz+az);
               break;
    
               -- TODO/FIXME: We should check that the destination space is actually free
               --   from other players and entities before we relocate PlayerEnt there!
           end
       end
    
       isTeleporting=true;
       wait(0.5);
       isTeleporting=false;
   end

.. |The GUI for the controls of the teleporter station.| image:: /images/mapping/cawe/tutorials/teleporter/teleporter_gui.png
   :class: mediaright
   :width: 320px
.. |The pair of entities for one teleporter station.| image:: /images/mapping/cawe/tutorials/teleporter/teleporter_entities.png
   :class: mediaright
   :width: 320px
