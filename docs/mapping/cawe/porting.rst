.. _importing_maps_from_other_games_editors:

Importing Maps from other Games/Editors
=======================================

If you have already made maps with other editors for games like
Half-Life 1, Half-Life 2, Quake 3 or Doom 3, you possibly wish to use
them with CaWE and Cafu as well. For this purpose, several importers are
provided that can load such maps directly into CaWE.

|The "File Open" dialog also imports maps from other games and editors.|
In order to import a foreign map, just activate the **File → Open…**
menu item or press **CTRL+O** in order to open the file open dialog.

The drop-down box near the bottom presents a list of all file types that
CaWE can directly load or import. Just choose the desired file type and
proceed to open a map file of that type as usual.

After the import, there are normally two additional problems that you
may encounter: material and entity mismatches.

*Material mismatches* are caused by the fact that the material systems
of all game engines are different from each other. While we can create
new materials in Cafu under the same names as those that occur in the
imported map, converting the used textures and other material details is
generally not feasible. Some importers therefore substitute all material
occurrences in the imported map file with native Cafu materials that are
valid, but have different names.

In all cases, you'll have to re-texture the imported map manually with
native CaWE materials. This is recommended and frequently desired for
best results anyway. Employing the powerful
:ref:`Edit Surfaces <the_edit_surface_properties_tool>` tool and the
:ref:`Replace Materials <the_replace_materials_dialog>` dialog gets the
job quickly and conveniently done. Making use of the “Only show USED
materials” checkbox in the
:ref:`Material Browser <the_material_browser>` is also helpful.

The following list summarizes what you have to know and consider about
*entity mismatches*:

-  Because the existing entities in your world were defined and made for
   another game, they will mismatch with the Cafu MOD entity
   definitions! Note that there are two kinds of mismatch:

   #. Some entities exist both in the other game as well as Cafu (for
      example translucent brushes, ladders, …), but their names and/or
      their properties differ.
   #. Other entities only exist in the other game and Cafu has no
      equivalent entity at all, or vice versa! For example, Cafu
      DeathMatch has different weapons, but unfortunately lacks of many
      of the advanced entities of other games. I am very sorry, but at
      present you have to find some kind of work-around in such cases.
      However, I am doing my best to improve the situation in the
      future.

-  Here are some suggestions to fix the entity mismatchings:

   #. It's probably the safest (easiest, but most expensive) to simply
      delete *all* existing entities from your world, and only keep the
      geometry. Then, recreate them from the Cafu (DeathMatch) entities.
   #. In many cases it might be preferable to simply open the
      **Properties** dialog for each entity, and to reset their
      properties there. Just make sure that you actually check out each
      individual property tag.
   #. If you are very experienced, you might solve many mismatchings
      directly by loading the ``map`` file into a text editor.

-  Although Cafu ignores unknown entities (e.g. entities left from other
   games), and fills-in default values for unknown properties, it is
   still best to do the conversion carefully in order to avoid
   unexpected results. For example, be specially careful with
   ``func_wall`` entities: you'll have to reset their *rendermode*
   properties (translucent, blue becomes invisible, …) *explicitly*!

.. |The "File Open" dialog also imports maps from other games and editors.| image:: /images/mapping/cawe/importmaps.png
   :class: medialeft

