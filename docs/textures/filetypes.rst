.. _texture_types:

Texture types
=============

| |image0| Most surface materials that occur in Cafu are defined by the
  mathematical composition of several texture images, where each texture
  image describes another aspect of the material. The most common
  texture image types are explained here.

In almost all cases, you start making a new material with the
**diffuse-map**. Diffuse-maps are almost always created manually in an
image processing program, e.g. hand-painted or delivered from a
photograph, similar to the way “old-style” textures were made in the
past. The diffuse-map shows material color when the surface is diffusely
lit. Thus, you should not draw any hard shadows into the diffuse-map -
they are automatically created by the engine later from the information
that comes from the normal-map and the light sources. You may however,
in some cases, draw some soft shadows into the diffuse map, a sort of
“reachability factor” (“How hard is it for the light to reach a certain
spot on the texture?”). This implies that a diffuse-map that shows for
example corrugated metal or a rough rock surface could be an image that
only has a single shade of grey! Also note that the final material when
rendered by the engine tends to look best when you use pixel values of
medium brightness. Moreover, high-contrast and high-frequency components
should be used with care in diffuse-maps, as such components often
interfere with normal-maps later, compromizing the effect of dynamic
lightning. Also the specular highlights might look strange with such
diffuse-maps.

**Specular-maps** (sometimes also called gloss-maps) define the
shininess of the material. They are conveniently created together with
or derived from their diffuse-map. Bright values mean that the material
is very shiny, dark values mean that the material is mat. Note that
specular-maps are not limited to gray-scaled images: Their tone (color)
modulates with the color of the light source. Specular-maps often have
the strongest impact on dynamic lightning. Note that for many materials
that only have diffuse light reflection characteristics (e.g.
sandstone), specular-maps can often be omitted entirely.

**Luminance-maps** define the light that a texture emits. As with
specular-maps, they are easiest created together with their diffuse-map,
and often very simple in nature. The light of luminance-maps is local to
the texture, and does not cast on any other surfaces or objects. Typical
occurances for luminance-maps are with LED panels or computer-screens,
but frequently they are not present at all, because most materials do
not actively emit light themselves.

**Height-maps** (also called bump-maps) are gray-scale images that
define the height of a surface: dark is low and white is high. They
often only serve as an intermediate product for creating normal-maps. In
fact, Cafu converts all height-maps to normal-maps internally before
use. Some people convert the diffuse-maps to gray-scale images in order
to obtain height-maps, but this does almost always yield in bad quality
- this is just a lazy trick that one should never use. Instead,
height-maps should be painted or created from scratch. This is almost
always a very difficult task though, and works best with either natural
or organic materials or high-frequency components like scratches, dents,
and so on. Another method is to obtain height-maps from the depth buffer
information of some rendered geometry. In this case, however, I'd
recommend to skip height-maps entirely, and render normal-maps directly.

**Normal-maps** are the most important component in dynamic lightning.
They contain information about the shape of the surface by
color-encoding the surfaces normal vectors. They are normally never
hand-made, but either derived from height-maps or created from true 3D
geometry. For example, plug-ins for Photoshop as well as for TheGimp
exist for converting a height-map into a normal-map. For normal-maps
that represent technical features like the example panel above, the best
results are achieved by creating them from 3D geometry though, which for
example is possible with 3D Studio Max. Please note that combining
diffuse-maps and normal-maps that both have high-frequency components
(and the diffuse-map possibly highly contrasting colors) tends to
compromize the effect of dynamic lightning.

.. |image0| image:: /images/textures/doc1_1.gif
   :class: mediaright

