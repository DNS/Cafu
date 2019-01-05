.. _loading_game_worlds:

Loading game worlds
===================

While we have introduced the GUI and entity component systems at several
occasions:

#. todo

this document provides an overview of how the Cafu programs actually
instantiate a game world that is composed of game entities.

All Cafu programs can load game worlds, i.e. the game server, client,
the world editor CaWE and the compile tools. We will use the server's
code for reference, but as the code is similar in all programs, we will
cover the specifics of other programs as well.

Also use the class diagram in ``Ca3DE/ClassDiagram.dia`` besides this
text for reference.

The server loads a game world by creating a ``CaServerWorldT`` instance
which derives from ``Ca3DEWorldT``.

The ``Ca3DEWorldT``'s ``const WorldT* m_World`` member contains the data
from the BSP, PVS and Radiosity compile process: for each static entity,
it has the BSP tree, a collision model, lightmaps, etc. ( **(FIXME!)**
not only static, but in fact for *all* entities in the cmap? ) This data
is later correlated to (and becomes a component of) the main entity
instances ``cf::GameSys::EntityT``.

The ``Ca3DEWorldT`` keeps an instance of a ``cf::UniScriptStateT``. This
script state is a Lua instance to which later our objects are bound so
that we then can write code such as:

.. code:: lua

   local new_ent = world:new("EntityT", "left_wing")
   Door:AddChild(new_ent)
   new_ent:GetTransform():set("Origin", 480, -352, 104)

Note that *both* game entities *and* GUI windows, our two sibling
component systems, can be instantiated in the script state of a
``Ca3DEWorldT``.
