.. _entity_guide:

Entity Guide
============

An entity is an object that has characteristics which are different to
the normal world brushes. There are two kinds of entities in Cafu. These
include brush-based entities and point-based entities.

Point-based entities
   exist at a specific coordinate in space and be can be thought of as a
   reference point. Although these entities may refer to objects that
   actually have a volume, for example a monster, they are only treated
   as points in CaWE, and the Cafu engine provides the volume later in
   the game. An example is a monster entity: In CaWE, it just represents
   the point where the monster will be, in Cafu, it will actually be
   there.
Brush-based entities
   are related to at least one brush or bezier patch and thus have a
   volume. Examples for brush based entities include doors and
   platforms, bodies of water, area triggers, etc.

Brush-Based Entities
--------------------

Solid entities are world brushes that have extra characteristics. Note:
This list is not fully complete.

func_illusionary
~~~~~~~~~~~~~~~~

This is a func_wall with no clipping. The player can walk through a
func_illusionary, making it useful for secret paths.

func_water
~~~~~~~~~~

This is the entity for water.

func_ladder
~~~~~~~~~~~

The player can climb up a func_ladder. Please note that this entity is
invisible, so you need to place a textured brush **behind** it.

func_breakable
~~~~~~~~~~~~~~

This is a breakable object. You can set the material as glass, wood,
metal, flesh, cinder block, ceiling tile, computer, unbreakable glass or
rocks. You can also set the strength of the object.

func_particle_generator
~~~~~~~~~~~~~~~~~~~~~~~

This entity emits particles. It could be used for fire, water etc.

func_pushable
~~~~~~~~~~~~~

This is an object that the player can push around the map.

func_terrain
~~~~~~~~~~~~

This is the entity used to add a terrain to a world. You must specify a
heightmap for the terrain, and a detail texture. See
:ref:`Creating heightmaps for your terrains <creating_height-maps_for_your_terrains>`
and the NewMaterials document for more info about terrains.

Point Entities
--------------

info_player_start
~~~~~~~~~~~~~~~~~

The info_player_start is the spawn point for the character(s). You must
have at least one in any map you create, and you must ensure that
**every** info_player_start is inside the map.

PointLight
~~~~~~~~~~

This is a light to complement the texture area lights. You used to have
to put one of these in front of every surface that emitted light, but
now you should not need to use this entity much, as any texture that
looks like a light will emit light in the game.

static_detail_model
~~~~~~~~~~~~~~~~~~~

| *Note: This entity is specific to the Deathmatch mod.*
| This is a model that is included in a map, usually just for visual
  effect. You can specify an external model, how big it should be
  (scale), and if applicable, which animated sequence it should be
  performing. This entity can be used for anything that would be
  difficult to model out of brushes in CaWE.

Please note that this entity doesn't clip, so you need to put a clip
brush around the model if you don't want the player to be able to walk
through it.

Weapons, Ammo and items
~~~~~~~~~~~~~~~~~~~~~~~

| *Note: The following entities are specific to the Deathmatch mod.*
| These entities are the weapons, ammo and items in the deathmatch mod.
  These all start with weapon\_, ammo\_, or item\_. Some of the
  operational weapons included in the deathmatch mod are:

-  weapon_shotgun (Shotgun)
-  weapon_357handgun (357 Handgun)
-  weapon_9mmAR (Assult Rifle)
-  weapon_crossbow (Crossbow)
-  weapon_rpg (RPG Launcher)
-  weapon_handgrenade (Grenade)

Some other weapons are included, but most of them don't work.

Monsters
~~~~~~~~

| *Note: The following entities are specific to the Deathmatch mod.*
| These are basically NPCs (Non-Player Characters). They can be animals,
  allies or enemies. The deathmatch mod currently contains three types
  of monsters:

-  monster_butterfly - A group of three butterflies that fly around in
   circles.
-  monster_companybot - The default Trinity bot that follows you around.
-  monster_eagle - An eagle which flies overhead. Requires a lot of
   space.

Tree
~~~~

**(FIXME!)** I don't know what this is…
