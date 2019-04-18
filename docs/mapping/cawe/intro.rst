.. _introduction_to_editing:

Introduction to Editing
=======================

Brushes
-------

Do you know how a Lego model or a real house is built from bricks?
Invidividual, small bricks are combined to form a complex architectural
structure. CaWE works principally in the same way, and thus if you can
visualize how individual bricks form a model or a big building, you will
find it easy to grasp the concepts behind CaWE.

In CaWE, the bricks are called *brushes*, and they come in several
shapes: blocks, wedges, cylinders, pyramids etc. Moreover, you can
modify the default brushes in many ways. They can be scaled, sheared,
mirrored, cut in pieces, carved and so on. You can create an arbitrary
big number of these brushes in any shape you like, and use and combine
them for building the matter that forms your world: walls, floors,
roofs, furniture, rocks, … . Brushes eventually get *materials* assigned
that represent the surface properties of that brush, as for example
rock, glass, concrete, sand or water. The creation of brushes is
detailed at `The New Brush
Tool </mapping:cawe:editingtools:newbrush#the_new_brush_tool>`__.

Bezier Patches and Terrains
---------------------------

Modelling really complex surfaces can be difficult with brushes alone,
especially if the surfaces should be curved, organic, or very big. Cafu
and CaWE therefore provide two additional basic elements that complement
brushes: *Bezier patches* are curved surfaces that you can imagine like
bent or stamped metal plates. They can be used to model pipes, curves,
smooth archs, and many other organic objects. The creation of bezier
patches is detailed at `The New Bezier Patch
Tool </mapping:cawe:editingtools:newbezierpatch#the_new_bezier_patch_tool>`__.
*Terrains* are similar, but as their name suggests, aim at surfaces that
are much bigger and more irregular. They're also treated specially by
the engine and have very high performance. The creation of terrains is
detailed at `The New Terrain
Tool </mapping:cawe:editingtools:newterrain#the_new_terrain_tool>`__.

Entities
--------

*Entities* define and represent all other interesting things in a world,
and they come in two flavours:

Point-based entities
   exist at a specific coordinate in space and can be thought of as a
   reference point. Although these entities may refer to objects that
   actually have a volume, for example a monster, they are only treated
   as points in CaWE, and the Cafu engine provides the volume later in
   the game. An example is a monster entity: In CaWE, it just represents
   the point where the monster will be, in Cafu, it will acutally be
   there.
Brush-based entities
   are related to at least one brush or bezier patch and thus have a
   volume. Examples for brush based entities include doors and
   platforms, bodies of water, area triggers, etc.

The creation of entities is described in the
:ref:`New Entity tool <the_new_entity_tool>` documentation. A list with
a description of all available entity types can be found in the
:ref:`Entity Guide <entity_guide>`.

Static Detail Models
--------------------

While *static detail models* are really nothing more than (point-based)
entities of type ``static_detail_model``, they provide another powerful
means to augment the detail in your world in a way that is not easy or
even impossible to achieve with brushes, bezier patches and terrains
alone. Such models can be imported from your favourite 3D modelling
program, they can be arbitrarily complex, and they can for example be
under the control of a level-of-detail system. Examples include models
for furniture, statues, household or laboratory inventory, vehicles,
etc.

Putting it All Together
-----------------------

By combining these simple components, you can create stunningly virtual
worlds whose number of variants is virtually unlimited. Starting with a
rough outline of brushes and possibly terrains, you can fill in
architectural detail with other brushes, patches and entities.
Eventually you will populate your world with more entities that bring
your creation to life.

The final step in making a world is compiling it so that it can be run
with the Cafu engine. This is a quite complex process that precomputes
visibility, lighting and many other details. However, CaWE has a great
built-in facility to make the compilation process as easy as possible
for you. The article
:ref:`Compiling Maps for Cafu <compiling_maps_for_cafu>` has more
information about how CaWE assists in compiling maps.
