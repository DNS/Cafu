.. _the_map_menu:

The Map Menu
============

|image0|

Snap to grid
------------

Switches grid snapping on and off.

Show grid
---------

Turns grid rendering on and off.

Grid settings
-------------

|image1| Adjusts the grid density. **Finer Grid** increases and
**Coarser Grid** decreases the grid density.

Units
-----

|image2| **(FIXME!)** not functional

Go to Brush Number...
---------------------

Opens the :ref:`Goto Brush/Entity <the_goto_brush_entity_dialog>`
dialog.

Show Information
----------------

|image3|

Opens the World Info dialog:

Brushes
   Number of brushes in this map.
Faces
   Number of faces in this map.
Point entities
   Number of point entities in this map.
Brush (solid) entities
   Number of entities related to a brush.
Unique texture count
   Number of different textures.
Texture memory
   Amount of memory used by the maps textures.

Entity Report
-------------

Opens the :ref:`Entity Report <the_entity_report_dialog>` dialog.

Check for Problems...
---------------------

Checks for problems in the map architecture and opens the
:ref:`Map Error Report <the_map_error_report_dialog>` dialog if problems
were found.

Map Properties
--------------

Selects the world object (and therefore recursively all map objects) and
opens the :ref:`Inspector <the_entity_inspector_dialog>` dialog to show
the properties of the map world entity.

Load Pointfile
--------------

Loads a pointfile into the map (see
:ref:`Dealing with Leaks <dealing_with_leaks>` for more details).

Unload Pointfile
----------------

Removes a previously loaded pointfile.

.. |image0| image:: /images/mapping/cawe/menureference/menumap.png
   :class: medialeft
.. |image1| image:: /images/mapping/cawe/menureference/menumapgrid.png
   :class: mediaright
.. |image2| image:: /images/mapping/cawe/menureference/menumapunits.png
   :class: mediaright
.. |image3| image:: /images/mapping/cawe/menureference/worldinfo.png
   :class: medialeft

