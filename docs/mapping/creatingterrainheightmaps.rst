.. _creating_height-maps_for_your_terrains:

Creating Height-Maps for your Terrains
======================================

The shapes of the terrains that you can create with
:ref:`The New Terrain Tool <the_new_terrain_tool>` are defined by
*height-maps*. A height-map is usually a square gray-scale image whose
black pixels represent the lowest heights (0%) and whose fully white
pixels describe the highest heights (100%). The gray values between them
represent intermediate height values – the brighter a pixel, the higher
is the resulting terrain at that spot.

CaWE and Cafu support height-maps in all the image file formats that are
also supported for textures (``jpg``, ``png``, ``tga`` and ``bmp``), as
well as Terragen (``ter``) and portable graymap (``pgm``) files. The
image file formats usually have a precision of 8 BPP (bits-per-pixel),
while the ``ter`` and ``pgm`` formats have a much higher precision of 16
BPP, and are therefore the preferred choice for height-maps.

Height-maps must also meet the additional requirement of having side
lengths of (2:sup:`n`\ +1) \* (2:sup:`n`\ +1) pixels, where n is an
integer between 1 and 15. Examples of typical height-map dimensions are
129*129, 257*257, 513*513 and 1025*1025.

Programs for Creating and Editing Height-Maps
---------------------------------------------

Creating and editing high-quality height-maps can be a complicated and
demanding process that typically requires the assistance of specialized
software. I've intensively tested many software packages for this
purpose, many of which just don't meet the requirements. Typical
problems include limited or broken importers and exporters (e.g. only to
and from proprietary file formats), limits in the supported high-map
dimensions (e.g. only 2\ :sup:`n` \* 2\ :sup:`n`, not 2\ :sup:`n`\ +1 \*
2\ :sup:`n`\ +1), usability, stability (e.g. frequent crashes) and the
inability to switch off the “relative height editing mode”, which
seemingly allows users to dig deeper than 0% or raise terrain higher
than 100% by adjusting the relative height of all *other* terrain.

The following lists mentions several programs that I consider useful or
relevant for creating and editing height-maps. None of them is perfect
or can do everything that is needed all in one program, so you might be
also interested in reading the [ **(FIXME!)** Link to tutorial about how
the TechDemo terrain was made. ] tutorial.

CaWE
~~~~

The obvious and best choice for editing the height-maps of terrain
entities is CaWE.

It is possible to edit and modify height-maps directly in the 3D view of
CaWE, in a “live” manner that naturally and immediately shows the
interactions with other elements of the map, as for example brush-based
buildings and walls, other entities, etc. For more information about the
terrain tool please check :ref:`here <the_edit_terrain_tool>`

Note that although CaWE would be the ideal place for height-map *editing
and modification*, it is not necessarily the best place for height-map
*creation*. Although that would be and will be possible, too, other
programs below that are specialized on height-map creation may do a much
better job in creating the initial height-map, especially if very large,
natural-looking shapes are desired. In such cases, you would use another
program to create the initial height-map, then add it to your map in
“rough” form using :ref:`The New Terrain Tool <the_new_terrain_tool>`,
and then continue to edit the fine details in CaWE, for example the
alignment with buildings, other brushes, etc. You may want to have a
look at the [ **(FIXME!)** Link to tutorial about how the TechDemo
terrain was made. ] tutorial in this regard.

Regular 2D Applications
~~~~~~~~~~~~~~~~~~~~~~~

Although not meant to visualize the actual 3d surface itself, nearly all
2d applications are able to provide a decent way of creating a highmap.
However most of these methods are more likely to create a basic uneven
surface that can then be altered inside the Cafu Editor for more precise
interaction with bsp or mesh based geometry.

While most terrain specific tools offer much more natural control over
the final result (like eroding based on water and wind) almost every 2d
application can provide hightmaps.

Noise filter (like perlin noise) are very powerful and can create a wide
variety of surface distortions that are great as a base. There are many
different applications out there commercial and free, yet we would like
to prefer free ones.

Gimp (Freeware)
^^^^^^^^^^^^^^^

Obviously the most common freeware 2d application, that mimics the
commercial *Adobe Photoshop* in many ways and beyond. It matured over
the years into a reliable 2d package and should be considered as a
professional tool. By using filter and custom brushes it is a very good
way to start a decent highmap

Gimp is loacted here: http://www.gimp.org/

Create basic landscape http://springrts.com/wiki/Height_Map_Tutorial

Flash based (Freeware)
^^^^^^^^^^^^^^^^^^^^^^

| Despite the negative critiqe of the last months, Flash is still a
  powerhorse. There are Flash apps amazingly versatile for editing, and
  they are always available as long as there is a web connection

| **Sumopaint** offers a lot of tools and filter to create highmaps
  http://www.sumopaint.com/app/
| **Pixlr** offers a similiar experience like sumo http://pixlr.com/
| **SplashUp** another flash based 2d app http://www.splashup.com/

L3DT (Free/Commercial)
~~~~~~~~~~~~~~~~~~~~~~

| Under developement for many years, **L**\ arge **3D** **T**\ errain is
  an app made for game devs. The basic version is free but there is also
  a Pro version that costs about 34$ for an indie license.
| http://www.bundysoft.com/L3DT/

Geo Control 2 (Demo/Commercial)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| Quite new and very powerful in all its features, Geo Control offers a
  31 day demo with all features or can be purchased for 129€.
| It offers many realistic features like erosion, isoline control for
  artifical leveling and a nice gradient based texturing system. There
  is also a version 1 available for a lower price.
  http://www.geocontrol2.com

World-Machine (Free Edition/Commercial)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

| The free edition is limited to 513*513 pixel but offers all tools. The
  next basic version costs 89$, can go beyond 8k resolution for maps,
  which also authorizes for comercial use.
| This program has become my favourite for height-map *creation*, see
  the manufacturers website at http://www.world-machine.com/ for more
  details. I also used it to create the height-map for the Cafu TechDemo
  with it. While it does not allow to “paint-edit” very fine details in
  a height-map, it is very good at composing logically structured
  terrains in a repeatable manner. The features of all logical devices
  that are used to build a height-map can be edited independent of each
  other, allowing for very flexible, reproducible results.

Leveller (Commercial)
~~~~~~~~~~~~~~~~~~~~~

Looks like it offers quite a few different tools to work with, however
the price of 199 dollar is way too much and there is not much indication
about when the last update actually took place.
http://www.daylongraphics.com/products/leveller/

Terragen (Free Edition/Commercial)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is *the* software for terrain rendering, but due to it's “relative
height editing mode” I'd not recommend it for height-map creation or
editing. Terragen *is* very good though at generating a base (ground)
texture from a height-map! http://www.planetside.co.uk/

Earth Sculptor (Commercial/outdated)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A small app, that works fine and allows for decent manipulation.
Unfortunatly it's development stopped since 2008, but still its worth
noticing it http://www.earthsculptor.com/index.htm

Other 3D Engines
~~~~~~~~~~~~~~~~

There are a few games with a great support for terrain editing, such as
Crysis or Unreal3. Epic offers the Unreal Developer Kit as a free
alternative, and the terrain editor is capable of exporting. This may
sound strange but still offers another option which is also completly
free.

Other Programs
~~~~~~~~~~~~~~

If you have experience with other programs, please add them here.

There are several other good programs, but I don't use them often. Maybe
one of them has released a new version in the meanwhile that addresses
some of the problems of older versions, so it might be worthwhile to
have a look at them! For example, for basic terrain creation, programs
like TerraBrush, Terraformer 1.8b or TerraMaker already do a good job.

Summary
-------

Use any of the above mentioned programs for creating the initial
height-map. However CaWE will allow you to further edit these maps
conveniently in the editor. If you must fine-tune them before that,
patience and creativity is required, although the above programs will
help with that, too.

Once your height-map is complete enough for use, you'll also want to
have a base texture for it that represents the color of the ground and
is used in the material definition with which your terrain is rendered.
While the texturing of the terrain might be possible in future versions
directly in CaWE, too, I currently just import my height-maps into
Terragen for this purpose, make sure the dimensions (lateral size and
altitudes) are correct, setup the Terragen materials, set the sun
appropriately (full-bright and no shadows) and turn off all atmospheric
effects, set an orthogonal camera view and then use the Terragen
rendering result as the base texture.
