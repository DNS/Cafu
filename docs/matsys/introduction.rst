.. _what_is_it_an_introduction:

What is it? An Introduction
===========================

The Cafu Material System is the central rendering subsytem of the Cafu
engine and its graphical tools like CaWE, the model viewer, the terrain
viewer, and so on. It takes polygonal meshes of geometry and renders
them on the screen. While doing so, it is responsible for the
*materials* that the mesh surfaces show. Said differently, it defines
how the rendered polygons *look*.

The Material System is *not* involved with or responsible for *what*
gets rendered, *when* it is rendered, how the meshes are spatially
formed or organized, etc.

Purpose, Goals and Features
---------------------------

The introduction of the Cafu Material System solved incredibly many
problems and comes with equally many advantages. Some of them are hard
to explain and/or hard to grasp, but here is a (partial) list anyway:

-  Unique rendering across all programs: The Cafu engine, CaWE, the
   model viewer, the terrain viewer, the material viewer, … all use the
   same rendering code and technology. I was finding myself writing,
   debugging and synchronizing the same code over and over again, for
   each mentioned program and for each supported 3D API, multiplying the
   expenses until chaos. This was one of the main reasons to start the
   MatSys. The savings are enormous, and I can be sure that models in
   the model viewer are rendered 100% identical as in CaWE or the
   engine.
-  Unique rendering across all types of geometry: Worse than the above,
   I found myself implementing the same technology even several times in
   the same program: once for the models, once for the world polygons,
   once for the terrains, etc. Sharing global resources like textures or
   GPU programs was impossible, and the Cafu engine happended to render
   the world polygons with different technology than the models, the
   terrrains with a third, and so on. Debugging was a nightmare. That's
   all over now, everything is now handled by the MatSys, and the
   savings and the leap in improved code-design were *huge*.
-  Can easily add support for new platforms, operating system, APIs.
   Without any danger to break existing code, and even without having to
   touch existing code at all, new renderers and new shaders can be
   introduced: If hot-plugging a new graphics board into your computer
   was possible, the Cafu engine could handle that situation *while
   running*. You can even supply and use a *completely new renderer*
   without stopping or recompiling the executables. Just have the engine
   reload the renderer, and the new technology will be immediately used.
-  Users and artists get a much greater and more flexible control over
   the rendering, as all materials can be edited independently from the
   worlds or models that they are applied to.

Renderers
---------

The MatSys is shipped in several separate modules. Each module offers
the same features, but they are different in that the implementation of
each is based on a different underlying technology. These modules are
called the **Renderers** of the Cafu MatSys. For example, a MatSys
renderer exists that is based on OpenGL 1.2, another is based on the
latest programmable GPU features and others are somewhere in the middle.
In fact, Cafu ships with several of them, and it uses the MatSys by
actually employing one of the renderers.

Technically, renderers are dynamically loaded libraries (``.dll`` files
on Windows systems), and you can find them in the ``Renderers/``
subdirectory of the demo or SDK. When the Cafu engine or another program
starts, it first scans the list of all available renderers,
automatically determines the one that is the best (or most appropriate)
for your system, and then loads it (this specific instance of the
Material System) for use.

This way it does not matter wether your computer is very old or the
latest leading-edge system, wether it uses OpenGL, Direct3D or
software-only rendering or wether its OS is Windows or Linux: You always
get the best possible graphical output – this is one of the goals the
MatSys has been designed for!

Materials
---------

**Materials** define what surfaces look like and what features a surface
has, and if you are interested in Cafu editing, you'll most likely get
in touch with them.

For example, mappers assign materials to world geometry in order to give
all objects the look and feel that they want. Modellers often create new
materials for the creatures that they create, and then assign them to
the polygonal meshes of their model.

When it comes to rendering, the Cafu engine takes both the meshes and
their assigned materials, and hands them to the Material System. The
MatSys then does it's best to render the mesh with that material as
accurately as possible on the available technology.

Each material is referred to by its name, and is defined in a *material
definition script* file. These files have the suffix ``.cmat`` and can
contain the definition of one or more materials. They are simple ASCII
text files, and you can find many examples in the
``Games/DeathMatch/Materials/`` subdirectory of the demo or SDK.

Here is an example of a material definition from
``Games/DeathMatch/Materials/Kai.cmat``:

::

       Textures/Kai/3r_metpan01
       {
           diffusemap  Textures/Kai/3r_metpan01_diff.png
           normalmap   Textures/Kai/3r_metpan01_norm.png
           specularmap Textures/Kai/3r_metpan01_spec.png
           lightmap    $lightmap
       }

*Using* materials is very easy. For example, when you're mapping with
CaWE, you apply materials to world brushes in the same way as well-known
textures used to be applied in other or older map editors, too. Please
see the CaWE User Guide in this Wiki for more information.

*Creating* materials often requires two steps: You first have to create
or acquire the texture maps that are used or required by the material.
These are typically png, jpg or tga images that define the diffuse-,
normal-, specular-, and other component maps that will be referred to by
the material. Then you have to write a material definition into a
``.cmat`` script file like in the example above. Very advanced materials
may also involve a third step, namely in the case that they require
their own unique shader (see below). The documentation in this Wiki has
detailed documentation about each of these steps.

Shaders
-------

The meaning of the word **Shaders** in the context of the MatSys is a
bit different than its meaning in other contexts (the word “shader” has
many meanings, after all). Shaders are objects of C++ code, and each
renderer has several of them built-in (in some cases up to a few
dozens). Most people will therefore never be faced with one of them.

However, shaders are the final instance in rendering, they define
exactly how the looks of a certain material is achieved. Therefore, when
the MatSys is given a certain material, it consults its library of
built-in shaders and selects the very shader that is best suitable for
rendering that material. The selection is automatic in most cases, but
can also be manually overridden by the material.

Even the case that there is a non-optimal assignment may sometimes
occur, e.g. when on the underlying hardware no shader can be implemented
that renders the desired material perfectly. In this case, a sub-optimal
(but still the best) shader is selected. This mechanism provides a
fall-back solution for arbitrarily old hardware, meeting another
important design goal of the MatSys.

Shaders therefore provide the key to be able to always support new
hardware: If new 3D hardware features become available, we can quickly
write a new shader for it, put it into the appropriate renderer, and
then materials can use it, either from auto-selection or by explicit
statement. This ability is a great feature that makes it easy to scale
the MatSys for future hardware, limited (e.g. embedded) hardware, old
hardware, etc.

Future SDKs will come with the ability to plug-in custom shaders, and
the Wiki documentation about shaders will updated until then.

Summary
-------

Technically and internally, the MatSys is organized as follows:

|image0| The real shaders actually have different names than in this
sketch, but shaders with the same name (e.g. ``Shader B``) all (try to)
render a given material in the same way – as much as that is possible on
the underlying rendering technology.

The shaders of each renderer may ship as built-in shaders or can be
provided as custom plug-in shaders by MOD authors.

.. |image0| image:: /images/matsys/aufbaumatsys.png
   :class: medialeft

