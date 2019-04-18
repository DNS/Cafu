.. _using_own_textures:

Using own textures
==================

This tutorial has 3 part. The first part covers textures for maps, the
second part covers skins for models and the third part skybox textures.

Part 1: Map textures
--------------------

Create your texture. Then save it into one of the supported file formats
(those are: .jpg, .png, .tga, .bmp).

Go into your Cafu directory and from there into the textures directory:
**Games → DeathMatch → Textures**. There it should somehow look like
this:

|tut1_1.jpg|

Now create a new directory, I call mine “tutorial”. Go into it and place
your texture there. I call my texture “texture1”, it consists of a
diffuse map, a normal map and a specular map. I add endings to the
different textures to exactly know which texture is which type. My
diffuse maps have a \_diff tag in their name, normal maps get \_norm,
specular maps get \_spec and luminance maps \_lum (not necessary to give
them those tags though).

OK, that was the first half, what we now have to to is to write a so
called “shader” file, a text file including names and paths of textures
that gives the informations about the textures, e.g. if they have normal
maps and so on or if they have transparent parts.

Let's go back to the DeathMatch directory. There you can find the
“Materials” directory in which all shader files are placed. They can be
opened with simple text editors like notepad. If you open for example
the “generic.cmat” file, you can see that it contains informations about
various textures. The system works like this:

::

   Textures/directory_name(in which the textures are palced)/texture name
   {
       type_of_texture Textures/directory_name/texture_patch
   }

The best way to describe those shaders is to actually have a look at
them. Here is the shader for my texture:

::

   Textures/tutorial/texture1
   {
       diffusemap Textures/tutorial/texture1_diff.jpg
       normalmap Textures/tutorial/texture1_norm.jpg
       specularmap Textures/tutorial/texture1_spec.jpg
       lightmap $lightmap
   }

If you have a luminance map you would have to write “lumamap” (without
quotes).

It's easy, isn't it? And that's already it! You can save it as
yourshadername.cmat or within another shader and it will work. Start
your mapping editor and you will be able to use the texture!

**Download:** `Tutorial example files (.zip
file) </_media/textures:tut-textures.zip>`__ **Download:** `Tutorial
example files (.tar.gz file) </_media/textures:tut-textures.tar.gz>`__

Part 2: Model textures
----------------------

Using own skins (model texures) is as easy as using map textures. There
are only slight differences.

Instead of being placed in a directory in the textures directory, you
have to place them in one of the directories in the Models (like the
models themselves) directory. Let's say we place them into the Static
directory that is placed in the Models directory. Our shader-file has to
be placed in the Models directory that can be found in the Materials
directory. It would look like this:

::

   Models/Static/mymodelname/a_name_for_my_skin
   {
       diffusemap Models/Static/a_name_of_my_skin_diff.png
       normalmap Models/Static/a_name_of_my_skin_norm.png
       specularmap Models/Static/a_name_of_my_skin_spec.png

       red ambientLightRed
       green ambientLightGreen
       blue ambientLightBlue
   }

There are only small differences: 1. you have to add a model name too
and not only a texture name (“mymodelname/a_name_for_my_skin”) - note
that you can choose whatever you want, those names dont have to meet the
model name/texture name. 2. in the lower part you can see that you have
to add those red, green and blue informations

Well that's it! If there are questions, feel free to post them in the
forums!

Part 3: Skybox textures
-----------------------

You can find detailed tutorial concerning the integration of new
skyboxes here:

-  :ref:`How to integrate a skybox into Cafu <how_to_integrate_a_skybox_into_cafu>`

.. |tut1_1.jpg| image:: /images/textures/tut1_1.jpg
   :class: media

