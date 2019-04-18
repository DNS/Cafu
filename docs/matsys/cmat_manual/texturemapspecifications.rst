.. _matsys_cmat_manual_texturemapspecifications_texture_map_specifications:

Texture Map Specifications
==========================

The simplest form of a material definition is to only specify the
texture map images that compose the material. The previous example,
repeated here, was an example for such a simple definition:

::

       Textures/Kai/3r_metpan01        // Material definitions start with the material name.
       {
           diffusemap  Textures/Kai/3r_metpan01_diff.png     // This line says which texture is used as diffuse-map.
           normalmap   Textures/Kai/3r_metpan01_norm.png
           specularmap Textures/Kai/3r_metpan01_spec.png
           lightmap    $lightmap
       }

A texture map specification starts with a keyword (e.g. ``diffusemap``)
and is followed by a “map composition”.

Map compositions are normally just the path plus file name of a texture
image relative to the MOD directory, like for example
``Textures/Kai/3r_metpan01_diff.png``. However, map compositions can
also be more complex constructs as explained in the next section
:ref:`Map Compositions <map_compositions>`.

The initial keyword (``diffusemap`` etc.) defines how the texture image
is used in dynamic lighting computations during rendering. This is very
similar to Doom3 materials, and both Cafu and Doom3 implement in this
regard a form of the *Phong lighting model*.

Here comes a list of all available keywords for texture map
specifications, along with a short description of their default meaning.
(The default is dynamic Phong lighting, which however can be overridden,
as explained at
:ref:`Shader Specifications <matsys_cmat_manual_shaderspecifications_shader_specifications>`.)

``diffusemap``
   The texture map image that defines the diffuse color (or diffuse
   reflectivity) of the material. The alpha channel of the diffuse-map
   specifies the translucency of the material.

``normalmap``
   The texture map image that specifies the color-encoded normal-map of
   the materials surface. The normal vectors must be specified in
   tangent space, range compressed (-1 to 1 maps to 0 to 1), and the
   y-axis points top-down. Note that heightmaps can be converted into
   normal-maps as described in section
   :ref:`Map Compositions <map_compositions>` below.

``specularmap``
   The image for the materials specular highlights.

``lumamap``
   This texture map image defines the luminance for this material. Note
   that the light emittance that is defined here is local only (as is a
   general property of the Phong lighting model). It is not cast onto
   other surfaces.

``lightmap``
   This materials lightmap image. This image is normally computed and
   provided by the Cafu engine or the program (e.g. the material viewer
   or CaWE) with which you use the material. Although you can provide
   texture map file names as with the above keywords, that rarely ever
   makes sense. Normally, simply specify the special lightmap
   ``$lightmap`` in combination with the ``lightmap`` keyword in order
   to use whatever lightmap the Cafu engine provides for this material.
   This is demonstrated in the preceding example.
   **(!)** If you don't use this keyword at all, or specify something
   different than ``$lightmap``, the compile tools will not cover
   surfaces that use this material with a lightmap, and these surfaces
   will neither receive nor reflect Radiosity light. Note that for many
   effect materials, this is desired and very useful behaviour, as for
   example with decals, skies, water surfaces etc.

``shlmap``
   The image that contains color-encoded coefficients for *Spherical
   Harmonic Lighting* for this material. As for lightmaps, you may
   specify arbitrary texture map file names with this keyword, but
   normally just use ``$shlmap`` in order to let the engine supply the
   proper SHL map.

``cubeMap``
   A six-sided cube-map image that is used e.g. for environmental
   mapping. Normally, cube-map specifications are per default *ignored*,
   because they are not regularly used with Phong lighting. Instead,
   they are specially activated as described in section
   :ref:`Shader Specifications <matsys_cmat_manual_shaderspecifications_shader_specifications>`.
   As a single cube-map is actually composed of six individual images, a
   special convention for its file name is employed: A ``'#'`` character
   in the file name is automatically replaced with the six possible
   cube-map suffixes \_px, \_nx, \_py, \_ny, \_pz and \_nz. For example,
   consider this material definition from the
   ``Cafu-9.06/Games/DeathMatch/Materials/SkyDomes.cmat`` file:
   ::

          Textures/SkyDomes/PK_BrightDay2
          {
              AmbientShader A_SkyDome
              LightShader   none      // == noDynLight

              // The '#' in the next line is auto-replaced with the relevant suffixes (_px, _ny, ...).
              cubeMap Textures/SkyDomes/PK_BrightDay2#.png, wrapS clampToEdge, wrapT clampToEdge

              // ...
          }

   This example has keywords and elements that will be explained further
   below, but for now observe that the file name that is assigned to the
   cube-map is ``Textures/SkyDomes/PK_BrightDay2#.png``. When the
   Material System loads the six individual images from disk, it will
   therefore load them from the files
   ``Textures/SkyDomes/PK_BrightDay2_px.png``,
   ``Textures/SkyDomes/PK_BrightDay2_nx.png``,
   ``Textures/SkyDomes/PK_BrightDay2_py.png``, and so on.

``cubeMap2``
   This is like the above ``cubeMap`` keyword. It allows you to specify
   a second cube-map for special-purpose shaders that require two
   cube-maps. This keyword too is normally ignored unless a shader is
   specified that makes use of it. See
   :ref:`Shader Specifications <matsys_cmat_manual_shaderspecifications_shader_specifications>`
   for more details.

You can specify arbitrary combinations of these keywords in one
material, as only the ``diffusemap`` keyword is mandatory. However, if
you use the same keyword more than once, only the last occurrence is
considered. The order of the keywords occurrences is not relevant.

.. _map_compositions:

Map Compositions
----------------

Texture map image specifications with the above keywords can not only be
simple file names, but also be more powerful **Map Compositions**. A map
composition is a description of how a *single* texture map image is
composited from several source images on disk. Here is an example for a
simple material whose normal-map is defined by a complex map
composition:

::

       Textures/Kai/barrel_rst
       {
           diffusemap Textures/Kai/barrel_rst_diff.png
           normalmap  combineNMs(MyNm1.png, hm2nm(add(MyHm2.jpg, MyHm3.tga)))
           lightmap   $lightmap
       }

(This example is overly complex for demonstration purposes, and not
really meaningful. Real-life examples are normally much simpler.)

The expressions that are valid to define a map composition are defined
as follows. Please note that the *arbitrary nesting* of expressions is
expressly permitted, yielding great freedom for artists.

``filename``
   This is the most simple expression: a path plus a filename, as e.g.
   ``Textures/Kai/barrel_rst_diff.png`` in the above example for the
   ``diffusemap``. The path is relative to the directory of the MOD for
   which this material script was written. Supported file extensions
   include png, tga, jpg and bmp.

``add(e1, e2)``
   This expression adds the colors of ``e1`` and ``e2``, where ``e1``
   and ``e2`` can be arbitrary sub-expressions. The resulting RGBA
   values are clamped to 1.0.

``mul(e1, e2)``
   This expression multiplies/modulates/filters the colors of ``e1`` and
   ``e2``.

``combineNMs(e1, e2)``
   Treats the colors of ``e1`` and ``e2`` as color-compressed normal
   vectors, and combines or “adds” them in a mathematically correct
   fashion. (This it *not* the same as the ``add(…)`` operation.)

``hm2nm(e1, scale)``
   Assumes that ``e1`` is a gray-scale heightmap and converts it into a
   normal-map. The relative height of the heightmap is scaled by factor
   ``scale`` in order to weaken or pronounce the resulting effect.
   Values between 1.0 and 10.0 are normal use, but numbers greater than
   10.0, less than 1.0, or even negative numbers are allowed, too.

``flipNMyAxis(e1)``
   Considers the colors of ``e1`` as color compressed normal-vectors,
   and flips their y-component. This is useful for normal-maps that have
   their y-component pointing into the wrong direction. Such normal-maps
   occurred in the early days of dynamic lighting or were created for
   other programs than Cafu. This function is for fixing such cases, and
   should rarely be needed.

``renormalize(e1)``
   Considers the colors of ``e1`` as color compressed normal-vectors,
   and renormalizes them (scales them to unit length). This is mostly
   useful for testing and debugging.

``blue2alpha(e1)``
   This function is for use with old diffuse-maps. It replaces the alpha
   channel of ``e1`` with value 0.0 (transparent) if the RGB color at
   this pixel is pure blue (0.0, 0.0, 1.0), and 1.0 (opaque) otherwise.
   Moreover, pure blue pixels are replaced with the average pixel color
   of the non-blue pixels in order to account for texture filtering.

``(automatic scaling)``
   Whenever you employ one of the above expressions to combine the
   results of two expressions ``e1`` and ``e2`` that have different
   lateral dimensions, ``e2`` is automatically scaled to match the size
   of ``e1``.

You can apply map composition expressions to *all* above mentioned
texture map specification keywords, i.e. they work with ``diffusemap``,
``normalmap``, ``specularmap``, ``cubemap``, etc.

Technically, a map composition is completed before the Cafu engine or
the graphics board see them. In other words, the engine or the 3D
hardware never see the individual images, only the composite result.
*Everything that is done by these composition steps could also be
pre-worked by the artist in an image processing software. There would
be*\ **no**\ *difference for the engine, the hardware, or in the
resource (memory) consumption.* Note that this feature has nothing to do
with dynamic lighting or how a texture map image is combined with images
of other texture map specification keywords!

Finally, you can specify several comma-separated options for the map
composition:

``minFilter``
   This controls MipMap usage and the filter that is used for texture
   minification. While the default setting usually looks best and also
   yields the best performance on modern graphics hardware, sometimes it
   is desireable to turn filtering off and accept some aliasing, as for
   example for font textures. The ``minFilter`` keyword must be followed
   by one of the filter methods

   -  ``nearest`` or ``point``
   -  ``linear`` or ``bilinear``
   -  ``nearest_mipmap_nearest``
   -  ``nearest_mipmap_linear``
   -  ``linear_mipmap_nearest``
   -  ``linear_mipmap_linear`` or ``trilinear`` (This is the default.)

``magFilter``
   This controls the filter that is used for texture magnification and
   must be followed by one of

   ``nearest``
      or **``point``** (There is almost never a reason to use this,
      except for very rare and special purposes, like some kinds of
      debugging.)
   ``linear``
      or **``bilinear``** (This is the default and gives best results.)

``wrapS``
   This controls horizontal texture coordinate wrapping and must be
   followed by one of

   ``repeat``
      for repeating the texture in horizontal direction. This is the
      default.
   ``clamp``
      for clamping the texture in horizontal direction, taking the
      border color into account. As the Cafu MatSys never uses or sets
      the border color, using ``clamp`` is rarely ever useful.
   ``clampToEdge``
      for clamping the texture in horizontal direction to its edge
      color. Often useful with cube-maps or terrain base images.

``wrapT``
   This controls vertical texture coordinate wrapping and must be
   followed by one of

   ``repeat``
      for repeating the texture in vertical direction. This is the
      default.
   ``clamp``
      for clamping the texture in vertical direction, taking the border
      color into account. As the Cafu MatSys never uses or sets the
      border color, using ``clamp`` is rarely ever useful.
   ``clampToEdge``
      for clamping the texture in vertical direction to its edge color.
      Often useful with cube-maps or terrain base images.

``noScaleDown``
   specifies that the texture image is never scaled down, not even if
   the user selects a medium or low texture detail setting for tuning
   the graphics performance. Useful for fonts, HUDs, lightmaps
   (implicitly), some model textures (see example below), and everything
   else that must not get mixed up or blurred by image filtering. Also
   used e.g. for the Cafu splash screen logo, which would get blurred
   otherwise (look into ``Games/DeathMatch/Materials/Splash.cmat`` if
   you want to toy around with it a little 🙂 ).

``noCompression``
   exempts this texture image from being stored in a compressed format
   in video memory, even if the user generally enabled texture
   compression for tuning the graphics performance.
   In the Cafu engine, texture compression is by default enabled for all
   texture images except normal-maps. Although Cafu automatically
   selects and employs the latest and highest quality compression method
   that the graphics driver offers (this even works when the Cafu
   executable is *older* than the driver!), sometimes the compression
   process comes with some loss of image detail or introduces small
   artifacts. ``noCompression`` can then be used to ensure no
   compression for a particular texture.

``useCompression``
   is more or less the opposite of ``noCompression``: it turns
   compression back on. Normally there is no reason to ever use this
   keyword. It exits for symmetry to ``noCompression`` and because
   ``noCompression`` is the default for normal-maps: if you want to have
   compression enabled for a particular normal-map, specifying
   ``useCompression`` will turn it on. However, please note that
   compression artifacts in normal-maps tend to disturb the lighting
   computations so much that the generated output images drop to
   questionable quality.
   Also note that ``useCompression`` is “weak”: If the user generally
   disables all compression, it will have no effect.

The meaning of the ``minFilter``, ``magFilter``, ``wrapS`` and ``wrapT``
options is analogous to their respective meanings in the OpenGL and
DirectX APIs. The OpenGL Programming Guide (the “Red Book”) about OpenGL
version 1.2 and higher has a good explanation about these options.
Although the text is specific to OpenGL, the same concepts apply to the
above mentioned options. The “Red Book” for version 1.1 does not address
the ``clampToEdge`` option, but its text is available online at
http://www.rush3d.com/reference/opengl-redbook-1.1/chapter09.html.

The options ``noScaleDown`` and ``minFilter bilinear`` are often
combined, because both scaling down textures for better graphics
performance as well as using ``trilinear`` filtering for rendering have
a tendency to mix the colors of neighboring pixels. In some cases such
as font textures, even the ``bilinear`` filtering is too much mix-up,
requiring us to combine ``noScaleDown`` with ``minFilter nearest``.

Options Example 1
^^^^^^^^^^^^^^^^^

Here is an example from ``Games/DeathMatch/Materials/Fonts.cmat`` that
demonstrates how the options are used:

::

       Fonts/Arial
       {
           diffusemap ../../Fonts/Arial.png, minFilter nearest, magFilter nearest, noScaleDown
           // ...
       }

Options Example 2
^^^^^^^^^^^^^^^^^

Another example for the ``noScaleDown`` and ``minFilter`` keywords.

| |image0| This is a typical skin texture that a modeller has produced
  for application to one of his model meshes.

|image1| A straightforward material definition would look like this:

::

       Models/Players/Trinity/trinityskin3
       {
           diffusemap Models/Players/Trinity_Skin_diff.png

           red   ambientLightRed
           green ambientLightGreen
           blue  ambientLightBlue
       }

The image to the left shows the result of the this material definition
being applied to a model mesh. Notice the small glitch in the image,
which is a result of mipmaps being applied to the above shown texture:
Mip-mapping mixes black pixels of the hair with adjacent, bright pixels
of the skin, yielding the intermediate colors that are marked in the
result image to the left. Such glitches are even more disturbing and
better visible with animated models, e.g. when the head of the model
slightly turns.

| Note that these kinds of artifacts are *no* bugs, they are a normal
  result from mipmap filtering.

|image2| With the ``noScaleDown`` and ``minFilter bilinear`` keywords
applied in the material script, the glitch disappears as shown here:

::

       Models/Players/Trinity/trinityskin3
       {
           diffusemap Models/Players/Trinity_Skin_diff.png, minFilter bilinear, noScaleDown

           red   ambientLightRed
           green ambientLightGreen
           blue  ambientLightBlue
       }

.. |image0| image:: /images/matsys/example_nomipmaps_1.png
   :class: medialeft
.. |image1| image:: /images/matsys/example_nomipmaps_2.png
   :class: medialeft
.. |image2| image:: /images/matsys/example_nomipmaps_3.png
   :class: medialeft

