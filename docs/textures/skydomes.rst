.. _skybox_creation_environmental_map_cubemap:

Skybox creation (Environmental Map, Cubemap)
============================================

A **skybox** is a special set of textures that is used to display the
sky. Cafu uses skyboxes to create skies. A skybox consists of **6**
textures. Try to imagine a map that is surrounded by a box. The walls
are covered with the sky textures. Sky + box = Skybox ;)

This documentation focuses at first on the integration process into Cafu
and displays later, how to create a skybox in general!

Here is an example of what those 6 sky textures can look like. If you
have some knowledge in geometry, you'll notice that the textures are in
this example put next to each other in form of an unfolded cube to
demonstrate how this skybox system works.

(Click on the picture to enlarge it) |image0|

.. _how_to_integrate_a_skybox_into_cafu:

How to integrate a skybox into Cafu
-----------------------------------

The textures can have the dimensions *256×256*, *512×512* and
*1024×1024* (other dimensions don't work or don't make sense). It is
important that they are seamless when being put together, otherwhise you
have viual glitches ingame.

If you have a close look at the picture, you can see that it includes
the names of the textures. As you can see, their names begin with sky1
but end with keys like \_py. The endings tell the engine which texture
demonstrates which site, sky1 is just the name of the sky used in this
tutorial. Like this, it is important to name your textures right. Here
is a small overview about which ending stands for which direction:

**(FIXME!)** This passage needs to be filled with content

-  \_nz :
-  \_nx :
-  \_pz :
-  \_px :
-  \_py : The upper side
-  \_ny : The lower side

Once named your sky textures right, you can place them in the skybox
directory that can be find in the textures directory.

Then, you have to create a shader file for the sky. If you don't know
what a shader is, please have a look at this desciption.

In the case you named your sky sky1, the shader file would look similar
to this:

::

     Textures/SkyDomes/sky1
     {
     AmbientShader A_SkyDome
     LightShader   none      // == noDynLight keyword
     // The '#' in the next line is auto-replaced with the relevant suffixes (_px, _ny, ...).
     cubeMap Textures/SkyDomes/sky1#.jpg, wrapS clampToEdge, wrapT clampToEdge
     ambientMask d           // Don't write into the z-Buffer, so that entities (like missiles) outside of the map can still be drawn.
     noShadows               // This material does not cast dynamic shadows.
     meta_noLightMap         // Don't create or keep lightmaps for this material, don't participate in Radiosity computations.
     meta_sunlight           // This keyword states that this material casts sunlight.
         ( 2  4  6)          // The irradiance of the sunlight in Watt/m^2 that comes (or shines) through this material. Values (100  90  80) might work, too.
         (-2 -5 -9)          // The direction of the incoming sunlight rays. The z-component should be negative. These values match the actual position of the sun in the cube-maps.
     }

Basically, **line 1, 7, 13 and 14** are important.

-  In line 1, you have the textures path for CaWE, basiacally you just
   have to change sky1 into your sky name.
-  In line 7, you have to change the path so the sky name (in this case
   sky1) is right and the texture format (in this case .jpg).
-  In line 13, the shader defines the colour of the sunlight (there's a
   description for this values later in the tutorial).
-  In line 14, the shader defines where the sun is in the sky (important
   for light calculation in maps, a descriptions can be found later in
   this tutorial).

When finished, save your cmat shader file and you can find your sky in
CaWE. Note: the first line of the shader (*Textures/SkyDomes/sky1*) is
also the name of the sky in CaWE.

That's it!

The irradiance of the sunlight
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**(FIXME!)** This passage needs to be filled with content

The direction of the sunlight
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**(FIXME!)** This passage needs to be filled with content

How to create a skybox with Terragen
------------------------------------

Terragen is a terrain generator that can also create skyboxes. The
skyboxes included in Cafu were created with Terragen. There are various
resources in the internet that cover skybox creation with Terragen. Here
are some recommended links:

-  `Official Terragen website <http://www.planetside.co.uk/terragen/>`__
-  `Creating Environment Maps with Terragen, tutorial on
   gamedesign.net <http://www.gamedesign.net/node/9>`__
-  `Static Skies Tutorial with Terragen, tutorial on
   3DNA <http://www.3dna.net/community/staticskies.htm>`__

How to create a skybox with SkyGen
----------------------------------

SkyGen is a simple web based (WebGL) skybox generator.

-  `SkyGen <http://www.nutty.ca/webgl/skygen/>`__

How to create a skybox out of photos
------------------------------------

**(FIXME!)** This passage needs to be filled with content

How to create a skybox out of panoramic images
----------------------------------------------

**(FIXME!)** This passage needs to be filled with content

.. |image0| image:: /images/textures/skytut_1s.gif
   :class: media

