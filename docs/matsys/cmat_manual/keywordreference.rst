.. _keyword_reference:

Keyword Reference
=================

This section lists all keywords that are allowed in the body of a
materials definition.

Some of the keywords influence the automatic selection of the shader for
the material, others just give more control over the shader that is
eventually used for the material, for making better use of the features
of that shader, increasing its usefulness.

Note that the built-in shaders of all MatSys renderers were written to
comply to the documentation of the keywords below, or rather, the
documentation was written to reflect the implementation of the built-in
shaders. If you employ a custom shader (e.g. a self-written one)
instead, there is no guarantee that this shader interprets and
implements the keywords in an identical manner, because the shader is
flexible and free to do anything that its author made it to. This is
where you need a documentation that is specific to that shader, see
:ref:`Built-in_special-purpose_Shaders <built-in_special-purpose_shaders>`
for the built-in ones.

In general, most keywords should occur only once per material
definition. If such a keyword is stated several times, only its last
occurrence is taken into account, overriding its previous statements.
The order in which such keywords are specified in the body of the
material definition is irrelevant, you may state the keywords in any
order.

| However, some keywords are allowed to occur several times, for example
  for enumerating textures or numbers. For such keywords, the order in
  which they appear *is* important. Keywords with this feature are
  explicitly mentioned below.
| **(FIXME!)** We should get rid of this exception by changing the
  syntax to e.g. **``shaderParamExpr ($expr1, $expr2, …, $exprN)``**.

.. _matsys_cmat_manual_keywordreference_shader_specifications:

Shader Specifications
---------------------

``AmbientShader $name``, ``LightShader $name``

These keywords are used to override the automatic shader selection. They
are explained in detail in section
:ref:`Shader Specifications <matsys_cmat_manual_shaderspecifications_shader_specifications>`.

*The precise meaning of all other keywords is “vague”, that is, they
depends on the implementation by the selected shader* **(!)**

.. _matsys_cmat_manual_keywordreference_texture_map_specifications:

Texture Map Specifications
--------------------------

``diffusemap $mc``, ``normalmap $mc``, ``specularmap $mc``,
``lumamap $mc``, ``lightmap $mc``, ``shlmap $mc``, ``cubeMap $mc``,
``cubeMap2 $mc``, ``shaderParamMapC $mc``

With these keywords you specify texture map compositions **``$mc``** for
use with this material. The keywords and texture map compositions are
explained in detail in section
:ref:`Texture Map Specifications <matsys_cmat_manual_texturemapspecifications_texture_map_specifications>`.

The **``shaderParamMapC``** keyword is special, as it can occur
arbitrarily often! You can use it to pass an arbitrary number of
texture-maps to the shader. Materials that employ a custom shader that
for example combines several diffuse-maps and several normal-maps might
use this keyword in order to list all the textures that the shader
employs.

Custom Data
-----------

``shaderParamExpr $expr``, ``shaderParamMapC $mc``

| These two expressions are used to pass additional information to
  custom shaders. They can occur arbitrarily often in a material
  definition, so their order is important.
| **(FIXME!)**: This should be **``shaderParamExpr ($expr1, …)``** etc.
  in order to get rid of the order requirement.

| The meaning of the expressions and map compositions that you pass to
  the shader with these keywords entirely depends on how the shader
  interprets them! The :ref:`Terrain <the_terrain_shader>` shader is a
  good example that employs these keywords.
| **(FIXME!)**: Provide more details here.
| **(FIXME!)**: The terrain shader should employ **``shaderParamMapC``**
  rather than **``lumamap``** for its detail-map…

Rendering Options (Universal)
-----------------------------

The following material parameters are *global rendering parameters*.
They affect the entire material, but only how it is rendered, that is,
its “looks”. These parameters are never looked at or taken into account
by the map compile tools.

``noDraw``
   If specified, this material does not render at all, it becomes
   invisible. This is probably mostly useful for debugging. (For
   mappers: ``noDraw`` does *not* cause CaBSP to remove any faces from
   the world. If you want faces to be actually removed, you should
   rather use the ``meta_XXX_TODO`` keyword in order to make CaBSP
   remove them right from the start.)

``noShadows``
   This material never casts shadows. Note that shadow-casting can also
   be controlled on a per-model and per-lightsource basis.

``twoSided``
   Normally, the back or “inside” faces of materials are not rendered.
   This renders both sides of the material. Useful for “indefinitely”
   thin objects like metal grates and fences, spider webs, … Note that
   two-sided faces are currently not properly lit by dynamic light when
   seen from the back side.

``depthOffset $num``
   This sets the polygonal depth offset. Z-buffer values of meshes that
   are rendered with this material are slightly offset as defined by
   $num, which should normally be a negative number. Materials that
   employ ``depthOffset -1.0`` are for example used internally by CaWE,
   in order to render the highlighting of selected objects.

``polygonMode $pm``
   This sets the polygon mode of a mesh. CaWE employs this keyword for
   rendering the objects in the 3D wire-frame mode. ``$pm`` must be one
   of

   -  ``filled``
   -  ``wireframe``
   -  ``points``

Rendering Options (Ambient only)
--------------------------------

The following parameters do only affect the ambient contribution of a
material (as implemented by the Material Systems ambient default
shaders). The map compile tools never look at these parameters. Some of
these parameters take expressions as their arguments, which are denoted
by ``$expr``. Expressions can be simple numbers or more complex
constructs, they are discussed in greater detail in subsection
:ref:`Expressions <expressions>`.

``alphaTest $expr``
   Activates the alpha test with the result of ``$expr`` as the
   reference value. The alpha test passes if and only if the alpha value
   of the ambient contribution (which normally comes from the
   diffuse-map) is greater than the reference value. Note that the
   reference value can be specified as an expression, that is, it can be
   *varying over time*. This can produce interesting effects (i.e.
   materials that appear to grow or shrink) if the diffuse-map comes
   with an appropriate alpha channel. Negative values turn the alpha
   test off. The test is off by default.

``blendFunc $src_factor $dst_factor``
   This parameter determines the blend function for the ambient
   contribution of the material. ``$src_factor`` and ``$dst_factor``
   must be one of ``zero``, ``one``, ``dst_color``, ``src_color``,
   ``one_minus_dst_color``, ``one_minus_src_color``, ``dst_alpha``,
   ``src_alpha``, ``one_minus_dst_alpha``, or ``one_minus_src_alpha``.
   Note that not all combinations make sense. Using ``blendFunc`` will
   be documented in greater detail in future releases of this text. Per
   default, blending is turned off.

``red $expr``, ``green $expr``, ``blue $expr``, ``alpha $expr``, ``rgb $expr``, ``rgba $expr``
   These parameters all define expressions for (channels of) the color
   with which the ambient contribution is modulated. I have defined the
   default ambient shaders such that for materials that have luma-maps,
   only the luma-map is modulated. Materials that have no luma-map get
   the entire ambient contribution modulated. This allows to create
   effects such as panels that have flickering LEDs, glowing lights etc.
   Note that you can specify different expressions for different color
   channels. That is, if you have a luma-map for a computer panel that
   has both red and green LEDs, you can for example have the red LEDs
   morse SOS, while the green LEDs change gradually by a sinus function.
   The default expression for all color channels is 1.0 (identity).

``ambientMask $turnoff``
   This specifies into which framebuffer channel should *not* be written
   when rendering the ambient pass. ``$turnoff`` must be a combination
   of the characters ``r``, ``g``, ``b``, ``a`` and ``d``, where ``r``
   refers to the red color channel, ``g``, ``b`` and ``a`` refer to the
   green, blue and alpha color channels respectively, and ``d`` refers
   to the depth buffer value.
   For example, ``ambientMask d`` is used with many particle (sprites)
   material definitions, in order to avoid that particles change the
   contents of the depth buffer, in order to meet the fact that
   particles are often rendered in an unsorted, additive manner.
   ``ambientMask d`` is also used for sky-dome material defs, so that we
   are able to render e.g. far away missiles or other objects in the sky
   that would otherwise be rejected by the depth test against the
   (nearer) sky dome polygons.
   Note that employing ``ambientMask d``, that is, *not* writing into
   the depth buffer during the ambient pass, also makes dynamic lighting
   of such affected meshes impossible. Therefore, ``ambientMask d``
   should always be combined with ``LightShader none`` and
   ``noDynLight``. **(FIXME!)**: The MatSys should issue a warning if
   that is not the case.
   Another example for employing this keyword is ``ambientMask gb``,
   which makes sure that nothing gets rendered into the green and blue
   color channels, leaving only red (and alpha and depth). This yields
   an effect similar to Doom3, when you see only red while the player
   has a foreboding vision.

``useMeshColors``
   Additionally to the above color definitions, this will modulate the
   materials color with the colors that are specified with the vertices
   of the mesh that is to be rendered. This option does currently only
   work in a very limited set of shaders, and is mostly useful
   internally in CaWE, the Cafu World Editor, e.g. for rendering
   wire-frame stuff. You will rarely ever need this in a real-world
   material script.

Rendering Options (Light only)
------------------------------

``noDynLight``
   If specified, this material does not receive any light from dynamic
   light sources, only the ambient contribution is rendered.
   (Technically, the material does not even get a per-light-source
   shader assigned.) Useful e.g. for sky domes, additive effects like
   particles, translucent surfaces like water and glass etc.

``lightMask $turnoff``
   This is very similar to ``ambientMask``, but it only affects the
   per-lightsource passes. This keyword is currently not used in any
   Cafu material.

.. _other_keywords:

Other Keywords
--------------

There are also keywords that define other aspects of the material that
are not directly related to their rendering.

``clip $c1, $c2, …``
   Defines for whom or for which purposes the material is considered
   solid in clipping (collision detection) computations. This is mostly
   interesting in game related settings (e.g. with player or monster
   clip brushes), but also programs like CaBSP and CaLight perform clip
   and ray trace tests that in turn are affected by this keyword.
   ``$c1, $c2, …`` must be a comma-separated list of at least one of the
   flags below. The default setting (i.e. if you do not use the ``clip``
   keyword at all), is ``clip all``. When you use ``clip``, the value is
   first cleared (reset to ``nothing``) and then additively rebuilt from
   the parameter list.
   Here is an explanation of the individual flags:

   -  ``nothing`` means that a surface with this material clips against
      nothing, it is non-solid.
   -  ``players`` means that the surface is solid to players.
   -  ``monsters`` means that the surface is solid to monsters (AI
      entities).
   -  ``moveables`` means that the surface is solid to moveable
      entities.
   -  ``ik`` means that the surface is solid to IK.
   -  ``projectiles`` means that the surface is solid to the projectiles
      of weapons (e.g. water surfaces, but not sky).
   -  ``sight`` means that the surface blocks line-of-sight tests (used
      e.g. for AI).
   -  ``bspPortals`` means that the surface clips portals, that is, it
      is solid to the flood-fill operation of CaBSP.
      **(!)** Note that each Cafu map must *entirely be sealed* by
      materials with the ``bspPortals`` property, or else CaBSP will not
      be able to flood-fill it properly and report that the map has a
      leak.
      Developers, note that this means that maps can be made where
      objects can fall out of the map (e.g. through the floor).
      Considering the clip-world only, this can be desirable e.g. for
      rockets that are fired into the sky, even though Cafu has to treat
      them specially for drawing (render objects in non-PVS “outer”
      leaves). CaBSP issues a warning if it ever finds a material that
      has ``bspPortals``, but not both ``players`` and ``monsters`` set,
      in order to make you aware that in such places, players or
      monsters could fall out of the map.
   -  ``radiance`` means that the surface blocks the transfer of light
      energy in the Radiosity computations of CaLight (most walls do,
      but for example glass and decals usually don't).
   -  ``all`` means that the surface is solid to all of the above (but
      not the ``trigger`` flag below).
   -  ``trigger`` means that the surface is part of a trigger volume.
      Entities that have trigger volumes can detect when something
      enters their volume and then take special actions. For example, a
      player that walks into a trigger volume can cause a script
      function to be run that in turn opens a door and spawns some
      monsters. The ``trigger`` flag is normally not combined with any
      of the other flags, but for special cases it is still possible to
      write e.g. ``clip monsters, trigger`` to define a material that is
      solid only to monsters and a trigger to everything else (e.g.
      players).

``surfaceType $st``
   Defines the type of the surface. ``$st`` must be one of ``none``,
   ``stone``, ``metal``, ``sand``, ``wood``, ``liquid``, ``glass``, or
   ``plastic``. The meaning of the surface types is solely defined by
   the game code, it will usually use them to play footstep sounds,
   ricochet effects, etc.

Meta Keywords
-------------

Finally, here are the meta-parameters that are taken into account by the
Cafu map compile tools. These parameters are not directly related to the
rendering of the material.

``meta_radiantExitance $r $g $b $scale``
   Radiant exitance RGB values plus intensity (scale). Used whenever a
   material should also be a source of radiosity light, as computed by
   CaLight. The radiant exitance is defined by ``$r``, ``$g`` and
   ``$b``, scaled by ``$scale``.

``meta_radiantExitance_byImage``
   Radiant Exitance RGB values from image file. Used by CaLight, plus
   Radiant Exitance intensity (scale) for the RGB values from image
   file. Used by CaLight, but not yet implemented.

``meta_sunlight ($r $g $b) ($x $y $z)``
   This keyword states that the materials casts (radiosity) sunlight.
   The first three numbers define the irradiance of the sunlight in
   Watt/m^2 for the red, green and blue wavelengths. As the light
   actually comes from a very far away lightsource (the sun, moon,
   etc.), it is not cast by the materials itself, but rather *through*
   them, like sunlight shining through a window. The second triple of
   numbers specify the directional vector of the incoming light rays.
   The ``$z`` number should always be negative, so that the light
   appears to come from above.
   In the file ``Games/DeathMatch/Materials/SkyDomes.cmat`` you can see
   exemplary use of this keyword.

``meta_alphaModulatesRadiosityLight``
   Makes CaLight handle the ``diffusemap`` alpha channel and the $alpha
   and $rgba keywords properly. For fences, grates, glass, water, etc.
   Not yet implemented.
