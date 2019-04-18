.. _the_edit_surface_properties_tool:

The Edit Surface Properties Tool
================================

|image0|

The Edit Surface Properties tool gives you control over the *surfaces*
of *Bezier patches* and *brush faces*. You can use it to apply and
change materials and modify their scale, shift and rotation on the
surface. Sophisticated controls are available to determine these and
other attributes of a surface.

The Edit Surface Properties tool can be activated by clicking on the
related icon in the Tools toolbar or by pressing the ``SHIFT+A``
shortcut key.

Left-Clicks: Selecting and Picking
----------------------------------

|image1|

A normal click with the left mouse button on a surface (Bezier patch or
brush face) in the 3D view accomplishes two tasks:

-  it *selects* the surface (you can see how it becomes highlighted by a
   reddish overlay), and
-  it *picks* the properties of the surface and updates the values in
   the related dialog accordingly (the **Orientation** and **Material**
   sections are updated).

As usual, you can hold the ``CTRL`` key to select more than one surface
at a time. Please note that ``CTRL``-clicked surfaces are selected, but
not picked. That is, only the first selected surface (left mouse button
click without ``CTRL``) implies the pick operation.

In fact, selection and picking are actions that are independent of each
other:

Both:
   A normal left mouse button click performs both.
Selection only:
   Holding the ``CTRL`` key while clicking toggles the selection, but
   doesn't pick.
Picking only:
   You can also pick but not select by holding the ``ALT`` key during
   the mouse click, which activates the eyedropper mode. The properties
   of such clicked surfaces will appear in the dialog, but their
   selection status won't change.

Finally, you can also hold the ``SHIFT`` key during a left mouse button
click. The ``SHIFT`` key does not add anything special when the click is
on a Bezier patch, but for brush faces, it causes *all* faces of the
affected brush to become selected. (If ``CTRL`` is not held at the same
time, the clicked brush face will also be picked for updating the dialog
values.) You can combine the ``SHIFT`` with the ``CTRL`` key in order to
toggle the selection status of all faces of the brush.

Brushes that were selected when the Edit Surfaces Tool was activated are
automatically changed into face selections.

You can hide the reddish overlay of selected surfaces by checking the
**Hide Selection Overlay** checkbox in the **Tool Mode** section of the
dialog. This improves the visual perceptibility of the surfaces in some
cases, which in turn can be helpful when using this tool.

Orientation and Material
------------------------

Once you have selected one or more surfaces, you can modify their
orientation and material attributes.

If you have multiple surfaces selected at the same time, the new value
of the changed attribute is applied to all of them, while all other
attributes are left alone. This allows you for example to replace the
material or to set a common scale on a number of surfaces all at the
same time, while the individual shift, rotation and other surface
aspects are left alone.

.. _id_right-click_application:

Right-Click Application
-----------------------

The orientation and material attributes in the dialog can also be
applied to surfaces by right-clicking on them. The right-clicked surface
needs not be selected for this to work, and all attributes (orientation
and material) are applied at the same time. This feature makes the
application of the attributes in the dialog to a number of surfaces
quick and convenient.

With right-click application, the way in which the attributes are
applied to the surfaces can be modified at the **Right MB mode** choice
in the **Tool Mode** section of the dialog. The following options are
available:

Apply Normal
   Applies the material and the orientation attributes in the dialog
   “normally” to the surface.
Apply Material only
   Applies the material, but not the orientation attributes.
Apply View Aligned
   This mode works like a slide-projector: The material is projected
   onto the clicked surface, where the base of the cameras view pyramid
   defines the plane of projection. (Contrary to a real slide-projector
   though, parallel rather than perspective projection is employed.
   Variants of parallel projection are also employed by the other apply
   modes and are more appropriate for the task at hand.) The orientation
   attributes are also taken into account. This is a great mode for
   texturing a rocky wall or any other irregular shape (that consists of
   multiple adjacent surfaces) seamlessly.
Apply Edge Aligned
   This mode makes sure that when you click on a surface *next* to the
   previously picked one (i.e. left-clicked, possibly with ``ALT``), the
   material is seamlessly aligned across the common edge. This is a
   worthwhile feature especially when the two surfaces are not coplanar.
   (It even works when the clicked surface is not immediately adjacent
   to the other.)
   **(FIXME!)** What exactly are the restrictions when Bezier patches
   are used?
Apply Projective
   Like Apply Normal, but this mode also applies the *texture planes U-
   and V-vectors* of the picked surface to the clicked surface. This
   mode is the generalization of both the Apply Normal as well as the
   Apply View Aligned modes, and as the underlying technique is a
   mathematical projection, it is called Apply Projective. Refer to the
   advanced section below for more technical information.
   This is the mode of choice whenever you want to texture a Bezier
   patch exactly like an adjacent brush face. For example, if you have a
   wall or a floor that is made of both brush faces and Bezier patches,
   and you want to apply the material on the brush faces seamlessly to
   the Bezier patches, just pick up the surface attributes from a brush
   face, and Apply Projective to the Bezier patches.

When ``SHIFT`` is held during a right-click on a brush surface, the
application is performed on all surfaces of the brush.

The button **to all Sel.** is equivalent to (but more convenient than)
right-clicking on all currently selected surfaces with the current
**Right MB mode** setting.

Alignment
---------

The materials of selected *brush faces* (not Bezier patches) can be
automatically aligned at the top, bottom, left or right edge of the face
by the related button in the **Alignment** section of the dialog. The
material can also be centered or made fit on the brush face. In all
cases, the effect is achieved by auto-adjusting the Shift and/or Scale
attributes of the selected brush faces appropriately.

( **(FIXME!)** Re-pick the first selected surface in order to update the
dialog attributes! )

The **Treat multiple as one** checkbox determines whether multiple
selected surfaces are treated, with regards to their spatial extends, as
multiple individuals or as a single merged one when one of the Alignment
buttons is pressed.

.. _fit_on_bp:

Fit on Bezier patches
~~~~~~~~~~~~~~~~~~~~~

For Bezier patches, the (highly useful!) **Fit** button works slightly
differently than for brush faces: Although in both cases the material
will be made “fit” to the underlying surface, Bezier patches are then in
a different mode regarding texture-coordinate generation than brush
faces.

This is because for brush faces, you could have achieved the same result
by manipulating the Scale and Shift manually. With Bezier patches, which
can be curved to any shape (e.g. cylindrical or spherical), the same
mathematics cannot achieve the same effect, and thus using the **Fit**
button on Bezier patches puts them into Fit mode that yields the desired
result.

The essence of Fit mode is that the Scale attributes now indicate the
number of repetitions of the material along the surface, rather than the
number of texels per world unit as usual.

Because of this difference, while applying the surface properties of
such Bezier patches to other Bezier patches is straightforward and
intuitive, applying the same attributes to brush faces is sometimes not
possible without distortion. Such cases are easily fixed though by
clicking and editing the affected brush surface.

The line “Mode: Fit” near the Orientation controls indicates that the
current surface values were taken from a Bezier patch that was in Fit
mode.

You can “revert” the mode of such a Bezier patch surface back to normal
by picking the surface attributes of a brush face and applying them to
the Bezier patch in **Apply Projective** mouse-button mode.

Plane Indicators
~~~~~~~~~~~~~~~~

The **wrt. World axes** checkbox indicates whether the texture plane of
the surface happens to be parallel to one of the three major planes
through the world axes.

The **wrt. Face plane** checkbox indicates whether the texture plane of
the surface is parallel to the spatial plane of the surface. This is
never true for Bezier patches (they in general have no inherent spatial
plane), and normally always true for brush faces; however you can use
the **Apply Projective** right-click apply mode in order to force any
texture plane onto any surface.

How do I ...
------------

... texture brush faces?
~~~~~~~~~~~~~~~~~~~~~~~~

Individual brush faces are easily textured by left-clicking them to pick
their attributes into the dialog. Then you use the controls in the
dialog to directly modify the orientation, alignment, etc. The changes
become immediately visible on the selected face in the 3D view. You may
check the **Hide Selection Overlay** checkbox in order to hide the
selection mask.

For texturing a larger area of your map where many brush faces are to be
textured in a similar style, you should first texture one of the
surfaces as described above, then use the
:ref:`Right-Click Application <id_right-click_application>` feature to
apply the texture to all desired surfaces in the 3D view quickly. Make
sure that you understand the apply modes of the right mouse button in
order to get the best out of them.

Repeat this with other textures of your choice, and fine-tune them
individually in the end.

... texture Bezier patches?
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is as easy as texturing brush faces, but sometimes Bezier patches
are textured for a specific purpose:

Bezier patches that are a part of a floor or ceiling plane are often to
be textured so that the transition to the adjacent brush is seamless.
This is easily achieved by picking the texture from the nearby brush,
select **Apply Projective** as the **Right MB mode**, then right-click
apply the material to the Bezier patch.

Bezier patches that represent bulges, dents or small terrains that are
expected to blend seamlessly into neighboring brushes are also well
approach by the projective or view aligned apply modes.

Pipes, pillars, wall arches and similar objects are best textures by
using the **Fit** alignment button. The material then covers them
naturally, and you can conveniently set the number of texture
repetitions by the Scale values in the dialog.

... deal with the "Picking [...] is not possible" message?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This message can occur when picking (left-clicking) Bezier patches that
were created with previous versions of CaWE, or Bezier patches that were
imported from other games and file formats. Such Bezier patches got
custom texture-coordinates assigned and are in “Custom” tex-coord
generation mode; you may refer to section
:ref:`Advanced Considerations <adv_considerations>` below for more
details.

Nothing is inherently wrong with Bezier patches in Custom mode, you just
cannot pick their surface properties and apply them to anything else. As
soon as you assign surface properties from another brush face or Bezier
patch (that is not in Custom mode) though, the Custom mode is
overwritten with the newly applied properties, and the Bezier patch acts
normally then.

... deal with "Mode: Fit" orientation attributes?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Bezier patches that are newly created or whose material has been applied
using the **Fit** button are in the “Fit” texture-coordinate generation
mode, whose essence is that the Scale attributes now indicate the number
of repetitions of the material along the surface, rather than the number
of texels per world unit as usual.

Refer to section :ref:`Fit on Bezier patches <fit_on_bp>` above and
:ref:`Advanced Considerations <adv_considerations>` below for more
details.

.. _adv_considerations:

Advanced Considerations
-----------------------

The Edit Surface Properties tool has been written in order to make the
texturing of brush faces and Bezier patches simple and consistent.
However, brushes and patches are inherently different, and therefore you
can do things with the one that you cannot do with the other, and vice
versa. This is true for geometric modelling but also has consequences
for texturing them with this tool. Moreover, the tool offers several
advanced ways to apply textures to the different kinds of surfaces, and
you might be interested to learn more on how they work.

This section first describes the texture-coodinate generation modes that
the tool internally uses, then explains the details of the plane
projection mode.

The Tex-Coord Generation Modes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each surface (brush face or Bezier patch) is internally assigned a
texture-coordinate generation mode that determines how the
texture-coordinates at the vertices of the surface are computed:

Plane Projection
   is the most frequently used and most important mode. It is *always*
   used on brush faces (which can never use another mode), but it can
   also be used on bezier patches. The Plane Projection mode is
   explained in greater detail below.

-  The **Fit** mode is special to Bezier patches, and can only be used
   on them. Whenever you press the **Fit** button in the dialog, the
   tex-coord gen. mode of the selected bezier patch is changed to this
   mode. It is typically used on pipes, columns and similarly shaped
   objects, making sure that the material naturally fits into the
   surface of the object. The number of repetitions can be given by the
   “Scale” parameter in the dialog, which therefore has a different
   meaning from the Plane Projection mode.
   (When the **Fit** button is pressed for brush faces, a visually
   similar operation is performed in Plane Projection mode.)

Custom
   is reserved for future use and applies whenever the user has
   specified custom UV texture-coordinates for the vertices of a surface
   that cannot be achieved with the other modes. It is also used on
   Bezier patches that were created when the CaWE map file format did
   not support the tex-coord generation modes. This is true for older
   CaWE maps and for maps that are imported from other games or file
   formats.

When you pick the attributes of a surface into the dialog, it also
remembers the tex-coord generation mode of that surface. Picking is only
possible when the surface is in Plane Projective or Fit mode, never in
Custom mode. Applying previously picked attributes to another surface
(either a brush face or Bezier patch) is always possible, although
applying “Fit” mode attributes to brush faces (which can only be in
Plane Projective mode) can yield unexpected results. **(FIXME!)**

Picking and therefore applying attributes that are associated with
Custom mode to any surface is not possible at all.

The Plane Projection Mode
~~~~~~~~~~~~~~~~~~~~~~~~~

In Plane Projection mode, the texture-coordinates at the vertices of the
desired surface are determined by an orthogonal projection of a texture
plane P onto the vertices. The parameters of the plane are determined by
the Orientation controls in the dialog.

| The following figure presents the setup in a top-down view:
| |image2|

The texture plane P is indicated by the dark blue color, its normal
vector in dark green. The x-axis span vector is indicated in red, as
well as integral multiples of its length in both directions. The y-axis
span vector is not included in the image, as it points up or down, into
or out of the figure.

A texture plane is obtained by picking a surface in Plane Projective
mode. For example, texture plane P in the image was obtained by
left-clicking on the highlighted face of brush A.

Note that the x-axis represents one width of the material that is to be
applied to the surfaces: If you change the Scale values in the dialog,
the length of the x-axis (and y-axis) changes, too, implying that more
or fewer repetitions of the material span a given distance in the world.
Changing the Shift values in the dialog moves the x-axis arrow and its
multiples along the plane. If rotation was applied, the x-axis would
rotate “into” our “out of” the image, while the y-axis became visible.

In the dialog, the **wrt. World axes** and **wrt. Face plane**
checkboxes indicate whether the dialogs last picked plane happens to be
parallel to the major world axes and/or parallel to the physical surface
of the picked-from face.

The dotted lines indicate how the material would be applied from the
texture-plane to the objects surfaces. At object A there is a natural
fit, because integral multiples of the material match the extends of the
object. Object D is also natural case, but the material would appear
shifted on it's surface. You would have to modify the Shift attributes
in order to align it with the extends of the surface.

Note that objects B and C are special cases: When you normally apply the
attributes of plane P to their surfaces, P is moved and rotated first so
that it gets parallel to the desired surface. This is what the **Apply
Normal** mode of the right mouse button does.

The **Apply Projective** mode however does not include the additional
move and rotate step, and applies texture-coordinates *directly as
indicated in the figure*. This is sometimes a very useful feature in
order to achieve certain effects, for example with texturing Bezier
patches in some cases.

The **Apply View Aligned** mode works similarly, except for the fact
that plane P is always forced to be parallel to the base of the view
pyramid of the currently active 3D camera.

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_editfaceprops.png
   :class: medialeft
   :width: 80px
.. |image1| image:: /images/mapping/cawe/editingtools/facepropdialog.png
   :class: mediaright
.. |image2| image:: /images/mapping/cawe/editingtools/texturemapping.png
   :class: media

