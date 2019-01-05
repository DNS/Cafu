.. _the_new_light_tool:

The New Light Tool
==================

|image0|

The New Light tool is used to place light entities and is actually a
subpart of the :ref:`New Entity tool <the_new_entity_tool>`. The
difference is, that the New Light tool can only place light entities,
whereas the :ref:`New Entity tool <the_new_entity_tool>` can place all
entities.

Lights are used to illuminate a map and come in two forms:

**Static lights**: These lights are precalculated by the editor when
compiling a map. Lightmaps are built to define which parts of the level
textures are illuminated and shadowed.

**Dynamic lights**: These lights are placed in the editor but their
illumination and shadow casting is calculated dynamically by the engine
during runtime. This makes them perfect to calculate light and shadow
with movable objects, which is impossible with static lights. It is also
possible to move the light source itself, which makes this a very
flexible light source, but has also very high processing costs.

You can activate the New Light tool using the related icon in the tool
bar or by pressing ``SHIFT``\ +\ ``L``.

The Tool Options Bar
--------------------

|image1|

In the options bar of the New Light tool you can choose what light you
want to create.

There are 2 options available:

PointLight
   A static light source from which lightmaps are calculated.
PointLightSource
   A dynamic light source which is handled by the engine at runtime.

Creating a light
----------------

To create a light source, you can click on a location in the 3D view
where the light source is instantly created.

If you click in the 2D views a point is created, which can be properly
positioned before creating the light source by pressing the ``ENTER``
key.

Configuring a light
-------------------

To configure the properties of a light source, you have so select the
light with the :ref:`Selection tool <the_selection_tool>` and choose
**Object Properties** from the **Edit** menu or press
``ALT``\ +\ ``ENTER``.

The properties for the **PointLight** include:

Opening Angle
   Opening angle of this light source
Intensity red/green/blue
   Intensity of the different light colors

The properties for the **PointLightSource** include:

Dyn. light radius
   Radius of light dissemination
Dyn. light diffuse color
   Color of diffuse light
Dyn. light specular color
   Color of specular light

New Light tool keyboard shortcuts
---------------------------------

-  ``ENTER``

   -  create light source after placing it in a 2D view

-  ``ESC``

   -  switch back to the Selection tool

-  ``ALT``\ +\ ``ENTER``

   -  if light source selected → open its properties dialog

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_newlight.png
   :class: medialeft
   :width: 80px
.. |image1| image:: /images/mapping/cawe/editingtools/cawe_tooloptionsbar_newlight.png
   :class: media

