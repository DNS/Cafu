.. _the_new_terrain_tool:

The New Terrain Tool
====================

|image0| The “New Terrain” tool helps you to create new terrains. These
terrains can be very large, have very good rendering performance, and
are specially treated by the Cafu engine.

The “New Terrain” tool is activated by either clicking on its related
button on the Tools toolbar, or by pressing the ``Shift+T`` keyboard
shortcut.

| When you activate the tool, the tool options bar shows controls for
  creating new terrains:
| |image1|

The ``New terrain heightmap name`` determines the file that is used as
the heightmap image that defines the shape of the new terrain. Use the
combobox to select a previously used heightmap file, or use the
``Browse`` button to select a new file from disk. Note that the
supported file formats include the most common image file formats
(``jpg``, ``png``, ``tga`` and ``bmp``) as well as *Terragen* (``ter``)
and *portable graymap* (``pgm``) files. Also note that heightmaps *need*
to be grayscale and need to be power of two + 1. For example, a 512×512
heightmap needs to be 513×513 to work correctly in Cafu. A 1024×1024
needs to be 1025×1025. And so on. The section
:ref:`Creating Height-Maps for your Terrains <creating_height-maps_for_your_terrains>`
explains how you can create your own height-maps for use here.

The two checkboxes to the right control whether sky walls, a sky ceiling
and a floor are created automatically with the terrain. Although
terrains are often used for large outdoor areas, they must still exist
“inside a room”. That is, terrains must be bounded by brushes that form
four walls, a ceiling and a floor. With the two checkboxes, you can
control whether CaWE should automatically create such bounding elements
for you together with the new terrain (recommended!). You can later
always edit or delete the so created walls (they are regular brushes),
e.g. in order to make a doorway to some indoor or underground area of
your map.

Finally, you have to pick a material for the new terrains surface, which
you can conveniently do as usual with the Materials toolbar.

Note that all these attributes determine how the new terrain will be
initially created. Of course, you can also review and change all these
aspects for existing terrains later.

With everything being setup, you create the new terrain in the same
manner as you create new brushes: Draw a box in the 2D views. The box
determines the bounds of the new terrain, both laterally as well as the
height of the lowest and highest points. When you're satisfied with the
position and size of the box, press ``RETURN`` or right-click in one of
the views to confirm the terrain creation.

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_newterrain.png
   :class: medialeft
.. |image1| image:: /images/mapping/cawe/editingtools/cawe_tooloptionsbar_newterrain.png
   :class: media

