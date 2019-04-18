.. _matsys_cmat_manual_shaderspecifications_shader_specifications:

Shader Specifications
=====================

The examples of material definitions that we have seen so far were only
composed of texture map specifications. In the
:ref:`MatSys Introduction <what_is_it_an_introduction>` we have learned
that internally, “shaders” take materials to finally implement how they
are rendered and how they look. In cases like our example materials
above, the MatSys selects shaders from a built-in library automatically,
picking the one that best fits the material.

But what if you want to change the way how your materials are rendered,
either just in the details or radically from ground up? What if you
don't want regular Phong lighting? Or you want an entirely different
method to render diffuse-maps or a different way to combine them with
normal-maps? What if the built-in shaders don't cover a way of rendering
that you prefer? It also happens that the built-in shaders do not take
cube-maps into account at all, so what if you want to have a material
with cube-map environment reflections?

The answer to these questions is two-fold: You have to override the
automatic shader selection by assigning a shader manually. But you also
have to “know” that shader: You have to know its name, or you cannot
assign it to your material. You have to know wether it needs a
diffuse-map or a cube-map or both, or any other combination of texture
map images, so that you can specify them in your material definition.
Sometimes you have to know even more details about it.

The good news is that the MatSys renderers have more shaders built-in
than just those that are considered during the automatical selection.
They are documented below, inclusive sample material definitions, and
you can use them to override the auto-selection. Alternatively, if you
know or are a programmer, you can also make your own shaders. By making
own shaders you gain the greatest flexibility that you can get, because
only shaders eventually define how materials look, and shaders can do
everything that the underlying hardware can do!

There is another issue that you should know about shaders: Each material
gets not only one shader assigned, but *two*. This is because the Cafu
Material System renders materials in two steps: First, the *ambient*
part of a material is rendered, that is, everything that is rendered
even if no light source is present. The *ambient shader* of a material
is responsible for how this part is rendered. Then, the parts of the
material looks that are contributed by each light source are rendered
separately, as for example diffuse reflections, specular highlights and
other effetcs. The per-light-source contributions are controlled by the
*light shader* of a material.

You set the ambient and light shader of a material (and thereby override
the automatic selection) with the following keywords in the material
definitions body:

``AmbientShader MyAmbientShader``
   assigns the shader with name ``MyAmbientShader`` as the ambient
   shader to this material.

``LightShader MyLightShader``
   assigns the shader with name ``MyLightShader`` as the
   per-light-source shader to this material.

Here is an example for how these statements could be used in a material
definition:

::

       Textures/Kai/barrel_rst
       {
           AmbientShader myCarMetallicBlue_ambient
           LightShader   myCarMetallicBlue_light

           diffusemap    Textures/Kai/barrel_rst_diff.png
           normalmap     flipNMyAxis(Textures/Kai/barrel_rst_norm.png)
           cubeMap       Textures/SkyDomes/ReflectiveCubeMap#.jpg
       }

.. _built-in_special-purpose_shaders:

Built-in special-purpose Shaders
--------------------------------

Currently, all MatSys renderers come with several special-purpose
shaders built-in (special-purpose means that they are never taken into
account for automatic assignment). In most cases, they are actually
pairs of shaders, one shader for the ambient and one shader for the
per-light contribution of the same effect, but you will find that the
exception to this rule is more often true than not. More built-in
shaders are currently in preparation, for example for special effects
like cube-map environment reflections, realistic water surfaces, etc.
Note that any programmer can also write his own shaders, allowing him to
implement *any* rendering effect that he wants! This makes the MatSys
highly flexible, extensible and future-proof, and was one of its primary
design goals.

The following special-purpose shaders are currently built into all
renderers of the MatSys. Please note these shaders and their related
examples are pretty advanced, and you might want to skip them until
later. It may be convenient to first finish reading the rest of this
documentation, which will help to fully understand the special-purpose
shader examples.

The SkyDome Shader
~~~~~~~~~~~~~~~~~~

In order to create surfaces that are invisible like fully transparent
glass and instead only show the far sky dome beyond them, the ambient
shader **``A_SkyDome``** exists. It basically only requires a cube-map
for the sky to be specified. In fact, it ignores the specification of
any other (i.e. diffuse-, normal-, etc.) maps. However, in most cases
you'll want to specify additional keywords that further describe the
properties of the sky surfaces. Here is an example:

::

       Textures/SkyDomes/PK_BrightDay2
       {
           // Activate the A_SkyDome shader as the ambient shader
           AmbientShader A_SkyDome

           // Have no dynamic light affect surfaces with this material.
           LightShader   none
           noDynLight

           // The '#' in the next line is auto-replaced with the relevant suffixes (_px, _ny, ...).
           cubeMap Textures/SkyDomes/PK_BrightDay2#.png, wrapS clampToEdge, wrapT clampToEdge


           // Don't write into the z-Buffer, so that entities (like missiles) outside of the map can still be drawn.
           ambientMask d

           // This material does not cast dynamic shadows.
           noShadows

           // Don't create or keep lightmaps for this material, don't participate in Radiosity computations.
           meta_noLightMap

           // This keyword states that this material casts sunlight.
           meta_sunlight
               // The irradiance of the sunlight in Watt/m^2 that comes (or shines) through this material.
               // Values like (100  90  80) might work, too.
               (220  180  100)
               // The direction of the incoming sunlight rays. The z-component should be negative.
               // (These values match the actual position of the sun in the cube-maps.)
               (-17 -699 -715)
       }

The **``AmbientShader A_SkyDome``** line activates the ambient sky dome
shader. Note that the far away sky dome is *not* affected by light of
any dynamic light source, and therefore we assign the **``none``**
shader as the per-lightsource shader in order to make sure that no
dynamic light is applied to sky surfaces. The **``noDynLight``** keyword
does essentially the same as **``LightShader none``** and will soon be
obsoleted. Until then, please use it together with each occurrence of
**``LightShader none``**.

The **``cubeMap …``** statement specifies the cube-map that is to be
used for this sky. If you also specified other texture maps like
diffuse-maps or specular-maps, they would simply be ignored, as the
**``A_SkyDome``** shader has no use for them.

The remaining keywords further specify important properties of this
material. Please refer to section
:ref:`Keyword Reference <keyword_reference>` for a detailed description.
Short explanations of their meanings are given in the comments in the
above example.

.. _the_terrain_shader:

The Terrain Shader
~~~~~~~~~~~~~~~~~~

The terrain shader exists in order to render the Cafu outdoor terrains,
which work a bit different than regular, Phong-lit surfaces. You
activate the ambient terrain shader by writing

::

       AmbientShader A_Terrain

in the materials body. Normally, you would now to expect to also assign
a terrain-specific shader for the per-lightsource contribution to the
terrain, as in

::

       LightShader L_Terrain

However, I have not yet written the **``L_Terrain``** shader, and so we
have to turn off dynamic lighting for terrains:

::

       LightShader none
       noDynLight

**(?)** Why do terrains not account for light by dynamic light sources?
Well, there are several reasons:

#. Terrain is typically employed by mappers in outdoor areas that are in
   bright sunlight. The effect of dynamic light sources would barely be
   visible, if at all (but cost *a lot* of performance instead).
#. Dynamic light sources are normally very “small” when being compared
   to the extends of terrains, and mappers tend to place them inside
   buildings rather than in the open area. This further limits the
   per-lightsource contribution on terrain surfaces.
#. I simply have not yet had the time to write the **``L_Terrain``**
   shader.

**(!)** But if for example somebody wanted to model an indoor cave with
the Cafu terrain technique, having a shader that accounts for the light
of dynamic light sources even on terrains would make a lot of sense.
Good news is that, as indicated above, it is *easy* to write such a
shader – I'll probably do so as soon as I need one.

Next, the **``A_Terrain``** (and the future **``L_Terrain``**) shader
requires a diffuse-map to be specified. This diffuse-map will be scaled
to match the physical size of the terrain, that is, it will cover the
terrain completely. In order to get the edges of the terrain properly
textured, it makes sense to also specify **``clampToEdge``** border
wrapping for the diffuse texture:

::

       diffusemap Textures/Terrains/BPRockB_tx1.png, wrapS clampToEdge, wrapT clampToEdge
       lightmap $lightmap

As the Cafu map compile tools will also generate a lightmap for the
terrain in order to e.g. take sunlight or other radiosity light sources
into account, we specify the **``lightmap``** keyword with the
engine-supplied lightmap. Note that while the lighting of dynamic
light-sources is *not* taken into account, the light of radiosity light
sources *is*!

Next, the **``A_Terrain``** shader employs the texture-map that is
specified with the **``lumamap``** keyword as a *detail-map* for the
terrain. This is a good example for how the specification of a custom
shader (here **``A_Terrain``**) can entirely alter the meaning of a
material keyword. We will see below how the coarseness (the
repetition-count) of the detail-map is set.

::

       lumamap    Textures/Terrains/CommonDetail2.png      // "A_Terrain" takes the Luma-map as Detail-map (optional).

The detail-map is optional, and may be omitted. However, terrains look a
lot better with them, and so their use is recommended.

| **(?)** You may be wondering wether multiple detail-maps can be
  handled, as for example in FarCry, where they have a detail-map for
  beach sand, one for rocks, one for grass and one for pavement.
| **(!)** The answer is: The **``A_Terrain``** shader can indeed *not*
  handle such detail maps. But as before, it would actually be easy to
  write such a shader as soon as one is needed!

Finally, you need this block of statements in the body of your material
definition:

::

       shaderParamExpr fParam4    // The first eight shader parameters are taken from fParam4 to fParam11
       shaderParamExpr fParam5    // and specify the coefficients of two planes for automatic tex-coord generation.
       shaderParamExpr fParam6
       shaderParamExpr fParam7
       shaderParamExpr fParam8
       shaderParamExpr fParam9
       shaderParamExpr fParam10
       shaderParamExpr fParam11
       shaderParamExpr 21.3       // Scale / repetitions of the Detail-map.

The only thing that you may change here is the number of repetitions of
the detail-map, 21.3 in the above example. The lines above that are
required in order to get the terrain properly rendered and cannot
reasonbly be altered.

For more information on what all the **``shaderParamExpr fParam*``**
lines do, please refer to section **(FIXME!)** (TODO).

Here is a complete example for a terrain shader:

::

       Terrains/BPRockB_tx1
       {
           AmbientShader A_Terrain
           LightShader   none
           noDynLight

           diffusemap Textures/Terrains/BPRockB_tx1.png, wrapS clampToEdge, wrapT clampToEdge
           lightmap   $lightmap
           lumamap    Textures/Terrains/CommonDetail2.png

           shaderParamExpr fParam4     // The first eight shader parameters are taken from fParam4 to fParam11
           shaderParamExpr fParam5     // and specify the coefficients of two planes for automatic tex-coord generation.
           shaderParamExpr fParam6
           shaderParamExpr fParam7
           shaderParamExpr fParam8
           shaderParamExpr fParam9
           shaderParamExpr fParam10
           shaderParamExpr fParam11
           shaderParamExpr 21.3        // Scale / Repetitions of the Detail-map.

           twoSided                    // "twoSided" is required for the SOAR terrain algorithm.
       }

The WaterCubeReflect Shader
~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to turn a polygon into a translucent water surface with moving
waves and reflected environment with Fresnel effect, employ the
**``A_WaterCubeReflect``** shader: **(FIXME!)** (This section is not
complete!)

The "none" Shaders
~~~~~~~~~~~~~~~~~~

You can use the special **``none``** shader both as an ambient or a
per-lightsource shader in order to have no shader for ambient or
per-lightsource contributions at all.

As you have seen in the examples above, this makes sense in several
situations. Especially materials that are not affected by local, dynamic
light sources often have the **``LightShader none``** statement, as for
example for sky dome. Note that currently, you still have to combine any
**``LightShader none``** statement with the **``noDynLight``** statement
in order to take proper effect. The **``noDynLight``** will however
become obsolete in future releases of the Cafu Material System.

Using **``AmbientShader none``** is much less frequently useful, and
almost only ever employed for “invisible” materials. Note that
**``AmbientShader none``** also implies **``LightShader none``**. Also,
in order to take proper effect, the **``noDraw``** keyword is required
with each occurrence of **``AmbientShader none``**, but that requirement
will be removed and **``noDraw``** be obsoleted in future versions of
the MatSys.
