.. _the_new_brush_tool:

The New Brush Tool
==================

|image0|

The New Brush tool is, as its name says, used to create new brushes.

Brushes are primitive objects with a simple structure and come in
different shapes, like a block, a cylinder, etc..

Most parts of a map in CaWE are build from brushes, whereas more complex
structures are built as a combination of brushes.

The Brush Tool can be activate by clicking on its related icon or by
pressing ``SHIFT``\ +\ ``B``.

The Tool Options Bar
--------------------

As the New Brush tool is activated, its options bar is visible above the
view windows.

|image1|

Here you can select the shape of the brush you want to create and the
number of sides the brush should have. For some shapes the number of
sides is fixed (as for example the block shape) and number of sides
setting is grayed out.

Creating a new brush
--------------------

To create a new brush you have to drag a box in a 2D view, that
represents the height, width and depth of the brush. Since the brush is
not created after dragging the box you are still able to move and resize
it.

|image2| |image3|

Note that holding down the ``ALT`` key while creating or resizing the
box, you can temporarily ignore the editors snap to grid setting.

To create the new brush you have to press ``ENTER``. The material that
is currently selected in the
:ref:`materials bar <the_main_window_user_interface>` is automatically
applied to the new brush.

Different Shapes
----------------

Block
~~~~~

|image4|

A block is a simple rectangular box with 4 sides.

Wedge
~~~~~

|image5|

A wedge is a triangular box with 3 sides.

Cylinder
~~~~~~~~

|image6|

A cylindrical brush with variable sides from 3 to 32.

Pyramid
~~~~~~~

|image7|

A pyramid is shaped like a cylinder, but the top vertices are all in one
point. It can have 3 to 32 sides.

Sphere
~~~~~~

|image8|

A spherical brush with 3 to 32 sides.

Arch
~~~~

|image9|

An arch with variable values such as wall width, side number and arch
span. These values can be configured in a special arch diaglog.

|image10|

Wall Width
   Thickness of the archs wall.
Number of Sides
   Number of pieces the arch consists of.
Arc
   Span of the arch in degree.
Start Angle
   Start angle offset.
Add Height
   Height difference between 2 arch pieces (see screen below).

|image11|

New Brush tool keyboard shortcuts
---------------------------------

-  ``SHIFT+B``:

   -  Activates the New Brush tool.

-  ``ENTER``:

   -  If a brush box is opened → create the brush (only works in a 2D
      view).

-  ``ALT``:

   -  If creating a brush box or moving/resizing it → temporarily
      deactivate snap to grid

-  ``ESC``:

   -  Back to Selection tool

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_newbrush.png
   :class: medialeft
   :width: 80px
.. |image1| image:: /images/mapping/cawe/editingtools/cawe_tooloptionsbar_newbrush.png
   :class: media
.. |image2| image:: /images/mapping/cawe/editingtools/newbrushcreate.png
   :class: media
.. |image3| image:: /images/mapping/cawe/editingtools/newbrushresize.png
   :class: media
.. |image4| image:: /images/mapping/cawe/editingtools/brushblock.png
   :class: media
.. |image5| image:: /images/mapping/cawe/editingtools/brushwedge.png
   :class: media
.. |image6| image:: /images/mapping/cawe/editingtools/brushcylinder.png
   :class: media
.. |image7| image:: /images/mapping/cawe/editingtools/brushpyramid.png
   :class: media
.. |image8| image:: /images/mapping/cawe/editingtools/brushsphere.png
   :class: media
.. |image9| image:: /images/mapping/cawe/editingtools/brusharch.png
   :class: media
.. |image10| image:: /images/mapping/cawe/editingtools/brusharchdialog.png
   :class: media
.. |image11| image:: /images/mapping/cawe/editingtools/brusharchheight.png
   :class: media

