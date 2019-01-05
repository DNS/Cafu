.. _the_edit_terrain_tool:

The Edit Terrain Tool
=====================

|image0|

| The Edit Terrain Tool allows users to easily manipulate the surface
  structure of a terrain.
| The surface structure of a terrain is stored in a height map
  containing elevation values that build up the terrain. Each elevation
  value lies within a certain value range whereas higher values mean a
  higher terrain elevation at this position (please refer to the
  `Heightmap <https://en.wikipedia.org/wiki/Heightmap>`__ Wikipedia
  article for more information on height maps).
| To visualize this elevation values 3-dimensionally the engine create a
  smooth terrain surface model from the height map.
| This tool lets the user modify the elevation values of a terrain by
  presenting them as a 2-dimensional height map that can be edited
  similar to a paint program as well as a 3-dimensional surface
  structure.
| You can activate the Edit Terrain tool by clicking on the related icon
  marked in the left screen or by pressing the ``SHIFT+F`` keyboard
  shortcut.
| To create an editable terrain please refer to
  :ref:`The New Terrain Tool <the_new_terrain_tool>`.

General
-------

The tool is mainly composed of three parts:

-  The 2-dimensional terrain height map representation in the top-down
   2D view.
-  The 3-dimensional terrain surface representation in the 3D view.
-  The terrain editor dialog.

| The 2D and 3D view let you select and modify terrains with your mouse
  and therefore incorporate the tools main functionality. The dialog
  lets you change tool parameters and select different terrain editor
  sub tools (informations on tool parameters and sub tools are explained
  in detail below).
| |image1| |image2| The terrain editor always works within a adjustable
  circular area that is visualized in the top down 2D view as well as in
  the 3D view as seen in the images on the right.
| To modify a terrain you first have to select one by clicking on it.
  Then you can see the red circle at the position of your mouse cursor
  (if it is inside the bounds of the selected terrain) showing the area
  of the terrain that will be affected by the currently active sub tool.
| By left clicking you can modify the terrain in the area of effect. You
  can even hold down the left mouse button and move the mouse to easily
  modify the terrain along your mouse movements.
| This tool handling is the same in the 2D and 3D views and allows to
  easily switch between 2D and 3D view editing according to the user
  preference.

The (top down) 2D View
----------------------

| |image3| When a terrain isn't selected by the terrain editor it is
  rendered like every other brush so just its center point and bounding
  box is visible (in the same way as it is rendered if the terrain
  editor tool is inactive).
| To select a terrain for editing you have to click either on the edges
  of its bounding box or on its center point in the same way as with
  :ref:`The Selection Tool <the_selection_tool>`.

| |image4| When a terrain is selected its height map is rendered in a
  gray scale mode in the 2D view. Looking at the image to the left you
  can see that higher elevations are rendered brighter whereas lower
  elevations are rendered darker. This lets the user easily assess and
  modify a terrains surface structure even from a non 3-dimensional
  view.
| The tool cursor (the red circle) shows the current tool position and
  affected area and will always be set to the mouse position in the 2D
  view (as long as the mouse is positioned inside the bounding box of
  the terrain).
| |image5| The 2D view has a context menu when the terrain editor tool
  is active. It lets you adjust the appearance of the terrain in the 2D
  view.

-  Height map 1:1 zoom: Zooms the 2D view so each elevation value of the
   terrains height data will be rendered as one pixel in the 2D view.
-  View color

|image6| |image7| |image8|

-  Greyscale: Renders the height map in a greyscale gradient from black
   to white where black represents the lowest elevation and white the
   highest elevation.
-  Rainbow: Renders the height map in a rainbow gradient from blue to
   red where blue represents the lowest elevation and red the highest
   elevation.
-  Debug: Renders the height map in repeating greyscale gradients from
   black to white. This mode is useful to make detailed terrain
   modifications where the spectrum of the other color gradients would
   be to small to see the differences.

The 3D View
-----------

| |image9| A terrains surface structure is rendered in the 3D view
  surface regardless if the tool is active or inactive or if the terrain
  is selected by the terrain editor or not.
| The only difference is that you will see the circular tool cursor on
  the terrains surface if the terrain is selected and the tool is
  active.
| To select a terrain in the 3D view you just click on it in the same
  way as with :ref:`The Selection Tool <the_selection_tool>`.
| The tool position is placed where the mouse cursor hits the terrain,
  so in contrary to the 2D view where you work from a top down view onto
  the terrain, you can modify the terrain 3-dimensionally and easily
  work on the slope of a mountain for example.
| An exception of this rule takes effect when the tool is actively used
  (e.g. the user flattens a part of the terrain). To avoid “jumping” of
  the tool cursor it is no longer placed where the mouse cursor hits the
  terrain but instead on a even plane. This makes using the tool in the
  3D view more intuitive.
| Since the terrain editor tool visualizes changes to the terrains
  height data in real time, the surface of the terrain must also be
  updated each time the user modifies the terrain.
| Because updating the terrain surface is a costly operation, a faster
  operation is used to update the terrain surface while actively working
  with a tool (e.g. raising parts of a terrain with the left mouse
  button down). In consequence the terrain surface will be updated with
  smaller detail in real time and the final high quality terrain surface
  is calculated when the user stops using an editing tool (releases the
  left mouse button).

The Terrain Editor Dialog
-------------------------

| |image10| Where the views allow you to modify the terrains height data
  at the tool position, the terrain editor dialog allows you to select
  different sub tools of the terrain editor and to adjust parameters for
  this tools. Furthermore you can change properties of the whole terrain
  with it.
| At the top of the dialog you can see the toolbar with buttons for each
  sub tools (these sub tools are explained in the next chapter).
| Following are 3 tool option sliders that let you adjust tool
  functionality:

-  Radius: The radius determines the size of the circular area of effect
   at the current tool position.
-  Hardness: The hardness describes if the changes made in the circular
   area of effect should smoothly blend into the surrounding or have a
   hard edge at the border of the area of effect.
-  Tool effect: The tool effect has different meanings for different sub
   tools, but can usually be pictured as the intensity with which the
   tool is used.

| 
| At the bottom of the dialog you can find options that affect the whole
  terrain.

-  Resolution: The resolution of the terrain height data (elevation
   values). This has nothing to do with terrains size. The size is
   determined by the terrains bounding box (which can be scaled like
   every other brush with
   :ref:`The Selection Tool <the_selection_tool>`). The resolution
   determines the number of height values that build up the terrains
   surface. A higher value usually means that a terrain surface is
   smoother and more detailed.
-  Import: You can import the terrains height data from various file
   formats including greyscale images in BMP, JPG, PNG and PGM format as
   well as `Terragen <http://www.planetside.co.uk/terragen/>`__ files.
-  Export: Exporting the terrain into a greyscale image (same formats as
   stated above) or `Terragen <http://www.planetside.co.uk/terragen/>`__
   file is possible as well.

Terrain Generation
~~~~~~~~~~~~~~~~~~

|image11| If you have created a new empty terrain and you haven't got
any height data at hand to create its surface, it is often desirable to
automatically generate a basic terrain.

The terrain generation dialog allows for this. By specifying a set of
input parameters it lets you create different base terrains and shows
the result in real time in a preview window.

-  Frequency: A high frequency results in a bumpy surface with many
   hills and valleys whereas a low frequency will create fewer broader
   hills and valleys.
-  Lacunarity: A multiplier that determines how quickly the frequency
   increases for each successive octave in the Perlin-noise function.
   The frequency of each successive octave is equal to the product of
   the previous octave's frequency and the lacunarity value.
-  Octaves: A higher value adds structural detail to the terrain to the
   terrain so its surface becomes more natural.
-  Persistence: A high value increases the surface contrast which
   results in steeper slopes between hills and valleys.
-  Seed: Determines the random arrangement of hills and valleys for a
   terrain. You can play with this value to get different results that
   have all the same characteristics defined by the other parameters
   above. Note that the resulting terrain from a seed will always be the
   same (modified by the other parameters of course), so you can always
   recreate the same terrain again by using the same seed.

You can also find very good and more technical descriptions of these
parameters in the `libnoise
glossary <http://libnoise.sourceforge.net/glossary/index.html>`__.

The Terrain Editor Sub Tools
----------------------------

| Since modifying terrains isn't a matter that can be handled by a
  single tool, the terrain editor consists of several sub tools that
  modify the terrain in their own way.

Tip: While working on your terrain you can undo/redo each step you made
by either using
:ref:`The Edit Menu <mapping_cawe_menureference_edit_the_edit_menu>` or
the undo/redo shortcuts ``CTRL-Y`` and ``CTRL-Z``. This allows you to
easily try something with the terrain and undo it if the results are not
what you expected.

The Raise Tool
~~~~~~~~~~~~~~

|image12|

| This tool allows you to raise the terrain inside the tools area of
  effect. A high hardness will result in hard edges along the tool
  circles border and the tool effect determines the amount of world
  units by which the terrain is raised.
| The following images show the usage of the raise tool with a low and
  high hardness:
| |image13|\ |image14| |image15|\ |image16| |image17|\ |image18|

The Lower Tool
~~~~~~~~~~~~~~

|image19|

| This tools lowers the terrain inside the tool circle. A high hardness
  results in hard edges along the tool border and the tool effect
  determines how much the terrain is lowered.
| Note that you can easily switch between the raise and lower tool using
  the middle mouse button. The following images show the usage of the
  lower tool with a low and high hardness:
| |image20|\ |image21| |image22|\ |image23| |image24|\ |image25|

The Flatten Tool
~~~~~~~~~~~~~~~~

|image26|

| The flatten tool like its name says flattens the terrain in its area
  of effect. The tool effect has no meaning for it and is therefore not
  accessible. The hardness works as with every other tool.
| The special thing about the flatten tool (and also the following two
  tools) is that it needs a reference height value. This is the height
  everything is flattened to.
| To get a reference value you have to pick up one from the existing
  terrain elevations. This is either done by activating the Eyedropper
  mode by holding down the ``ALT`` key and then clicking on the position
  from which you want to pick up the height value (this works regardless
  of the activated sub tool).
| Another way to pick up a height value is to use the middle mouse
  button (this only works for the flatten, fill and ablate sub tool).
| The following screenshots show usage of the flatten tool with a
  reference height between the two hills visible in the screens. As you
  can see everything is flattened to the same height.

|image27|\ |image28| |image29|\ |image30|

The Fill Tool
~~~~~~~~~~~~~

|image31|

| Works in the same way as the flatten tool with the exception that only
  those parts of the terrain are flattened that lie below the reference
  height value. This allows you to easily fill up valleys while leaving
  mountains as they are.
| Using the fill tool with the same reference height as in the example
  above results in the flattening of all parts of the terrain except the
  top of the first hill.

|image32|\ |image33| |image34|\ |image35|

The Ablate Tool
~~~~~~~~~~~~~~~

|image36|

| Works in the same way as the flatten tool with the exception that only
  those parts of the terrain are flattened that lie above the reference
  height value. This allows you to easily ablate mountains while leaving
  valleys as they are.
| As shown below the ablate tool only cuts the top of the first hill and
  leaves everything else as it was.

|image37|\ |image38| |image39|\ |image40|

The Blur Tool
~~~~~~~~~~~~~

|image41|

| The blur tool will soften the terrain by reducing the elevation
  difference between the height values in the area of effect. The result
  is a smoother less edged terrain surface.
| The tool effect determines how much the terrain surface is
  blurred/smoothened.

|image42|\ |image43| |image44|\ |image45|

The Sharpen Tool
~~~~~~~~~~~~~~~~

|image46|

| This is the contrary to the blur tool. It sharpens the edges of a
  terrain surface by increasing the elevation difference between height
  values and thus creating a more rocky/edgier terrain surface.
| Again the tool effect determines how intensive the sharpening is.

|image47|\ |image48| |image49|\ |image50|

The Noise Tool
~~~~~~~~~~~~~~

|image51|

| The noise tool raises and lowers parts of the terrain in the area of
  effect at random. This makes the terrain in this area bumpier and adds
  structure to a flat surface.
| The tool effect determines the maximal height difference between the
  current elevation and the randomly chosen one. A high tool effect
  makes the terrain more bumpy a low tool effect less.

|image52|\ |image53|

The Road Tool
~~~~~~~~~~~~~

|image54|

| Unless you want to leave your terrains completely natural realistic
  roads are an important part of them. A road is an even plane that runs
  along a specific path through the terrain and might also have a slope
  between its starting and end point.
| The road tool lets you specify the path that builds up the road and
  automatically creates a slope between the elevation at the roads
  beginning and end.
| To create the roads path you have to click onto different points
  inside your terrain that make up the final road path. You can also
  hold down your left mouse button and continuously create road points
  while moving the mouse to create smooth curves.
| To finally create the road, you have to press the ``RETURN`` key and
  the road is constructed. You can also undo road points by clicking the
  middle mouse button or pressing the ``BACK`` key.
| The hardness determines if the road has a sharp edge to the
  surrounding terrain or smoothly blends into the terrain.

|image55|\ |image56| |image57|\ |image58| |image59|

Creating textures for a terrain
-------------------------------

| At this point you can't create terrain textures using CaWE and have
  therefore to rely on other external tools. Assigning a texture is done
  using
  :ref:`The Edit Surface Properties Tool <the_edit_surface_properties_tool>`.
| To create a texture you have to export your terrains height data and
  import it in an external program that allows you to create a surface
  texture for your terrain.
| Possible solutions are
  `Terragen <http://www.planetside.co.uk/terragen/>`__ and `World
  Machine <http://www.world-machine.com/>`__.

Keyboard shortcuts
------------------

-  ``SHIFT+F``:

   -  Activates the Terrain Editor tool.

-  ``ALT``:

   -  Activates the Eyedropper mode that lets the user pick up a
      reference height value.

-  ``ENTER``:

   -  If the Road sub tool is active a road is constructed from the
      available road reference points.

-  ``BACK``:

   -  Removes the last set road reference point.

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_terrainedit.png
   :class: medialeft
   :width: 80px
.. |image1| image:: /images/mapping/cawe/editingtools/terraineditor_tool3d.png
   :class: mediaright
.. |image2| image:: /images/mapping/cawe/editingtools/terraineditor_tool2d.png
   :class: mediaright
.. |image3| image:: /images/mapping/cawe/editingtools/terraineditor_2dview_nosel.png
   :class: mediaright
   :width: 200px
.. |image4| image:: /images/mapping/cawe/editingtools/terraineditor_2dview.png
   :class: medialeft
   :width: 200px
.. |image5| image:: /images/mapping/cawe/editingtools/terraineditor_2dview_context.png
   :class: mediaright
.. |image6| image:: /images/mapping/cawe/editingtools/terraineditor_gradientgrey.png
   :class: media
.. |image7| image:: /images/mapping/cawe/editingtools/terraineditor_gradientrainbow.png
   :class: media
.. |image8| image:: /images/mapping/cawe/editingtools/terraineditor_gradientdebug.png
   :class: media
.. |image9| image:: /images/mapping/cawe/editingtools/terraineditor_3dview.png
   :class: mediaright
   :width: 200px
.. |image10| image:: /images/mapping/cawe/editingtools/terraineditor_dialog.png
   :class: mediaright
.. |image11| image:: /images/mapping/cawe/editingtools/terraineditor_generate.png
   :class: mediaright
.. |image12| image:: /images/mapping/cawe/editingtools/terraineditor_tool_raise.png
   :class: media
.. |image13| image:: /images/mapping/cawe/editingtools/subtool_raise1.png
   :class: media
   :width: 100px
.. |image14| image:: /images/mapping/cawe/editingtools/subtool_raise2.png
   :class: media
   :width: 100px
.. |image15| image:: /images/mapping/cawe/editingtools/subtool_raise3.png
   :class: media
   :width: 100px
.. |image16| image:: /images/mapping/cawe/editingtools/subtool_raise4.png
   :class: media
   :width: 100px
.. |image17| image:: /images/mapping/cawe/editingtools/subtool_raise5.png
   :class: media
   :width: 100px
.. |image18| image:: /images/mapping/cawe/editingtools/subtool_raise6.png
   :class: media
   :width: 100px
.. |image19| image:: /images/mapping/cawe/editingtools/terraineditor_tool_lower.png
   :class: media
.. |image20| image:: /images/mapping/cawe/editingtools/subtool_lower1.png
   :class: media
   :width: 100px
.. |image21| image:: /images/mapping/cawe/editingtools/subtool_lower2.png
   :class: media
   :width: 100px
.. |image22| image:: /images/mapping/cawe/editingtools/subtool_lower3.png
   :class: media
   :width: 100px
.. |image23| image:: /images/mapping/cawe/editingtools/subtool_lower4.png
   :class: media
   :width: 100px
.. |image24| image:: /images/mapping/cawe/editingtools/subtool_lower5.png
   :class: media
   :width: 100px
.. |image25| image:: /images/mapping/cawe/editingtools/subtool_lower6.png
   :class: media
   :width: 100px
.. |image26| image:: /images/mapping/cawe/editingtools/terraineditor_tool_flatten.png
   :class: media
.. |image27| image:: /images/mapping/cawe/editingtools/subtool_flatten1.png
   :class: media
   :width: 100px
.. |image28| image:: /images/mapping/cawe/editingtools/subtool_flatten2.png
   :class: media
   :width: 133px
.. |image29| image:: /images/mapping/cawe/editingtools/subtool_flatten3.png
   :class: media
   :width: 100px
.. |image30| image:: /images/mapping/cawe/editingtools/subtool_flatten4.png
   :class: media
   :width: 133px
.. |image31| image:: /images/mapping/cawe/editingtools/terraineditor_tool_fill.png
   :class: media
.. |image32| image:: /images/mapping/cawe/editingtools/subtool_flatten1.png
   :class: media
   :width: 100px
.. |image33| image:: /images/mapping/cawe/editingtools/subtool_flatten2.png
   :class: media
   :width: 133px
.. |image34| image:: /images/mapping/cawe/editingtools/subtool_fill3.png
   :class: media
   :width: 100px
.. |image35| image:: /images/mapping/cawe/editingtools/subtool_fill4.png
   :class: media
   :width: 133px
.. |image36| image:: /images/mapping/cawe/editingtools/terraineditor_tool_ablate.png
   :class: media
.. |image37| image:: /images/mapping/cawe/editingtools/subtool_flatten1.png
   :class: media
   :width: 100px
.. |image38| image:: /images/mapping/cawe/editingtools/subtool_flatten2.png
   :class: media
   :width: 133px
.. |image39| image:: /images/mapping/cawe/editingtools/subtool_ablate3.png
   :class: media
   :width: 100px
.. |image40| image:: /images/mapping/cawe/editingtools/subtool_ablate4.png
   :class: media
   :width: 133px
.. |image41| image:: /images/mapping/cawe/editingtools/terraineditor_tool_blur.png
   :class: media
.. |image42| image:: /images/mapping/cawe/editingtools/subtool_blur1.png
   :class: media
   :width: 100px
.. |image43| image:: /images/mapping/cawe/editingtools/subtool_blur2.png
   :class: media
   :width: 100px
.. |image44| image:: /images/mapping/cawe/editingtools/subtool_blur3.png
   :class: media
   :width: 100px
.. |image45| image:: /images/mapping/cawe/editingtools/subtool_blur4.png
   :class: media
   :width: 100px
.. |image46| image:: /images/mapping/cawe/editingtools/terraineditor_tool_sharpen.png
   :class: media
.. |image47| image:: /images/mapping/cawe/editingtools/subtool_sharpen1.png
   :class: media
   :width: 100px
.. |image48| image:: /images/mapping/cawe/editingtools/subtool_sharpen2.png
   :class: media
   :width: 100px
.. |image49| image:: /images/mapping/cawe/editingtools/subtool_sharpen3.png
   :class: media
   :width: 100px
.. |image50| image:: /images/mapping/cawe/editingtools/subtool_sharpen4.png
   :class: media
   :width: 100px
.. |image51| image:: /images/mapping/cawe/editingtools/terraineditor_tool_noise.png
   :class: media
.. |image52| image:: /images/mapping/cawe/editingtools/subtool_noise1.png
   :class: media
   :width: 100px
.. |image53| image:: /images/mapping/cawe/editingtools/subtool_noise2.png
   :class: media
   :width: 100px
.. |image54| image:: /images/mapping/cawe/editingtools/terraineditor_tool_road.png
   :class: media
.. |image55| image:: /images/mapping/cawe/editingtools/subtool_road1.png
   :class: media
   :width: 100px
.. |image56| image:: /images/mapping/cawe/editingtools/subtool_road2.png
   :class: media
   :width: 133px
.. |image57| image:: /images/mapping/cawe/editingtools/subtool_road3.png
   :class: media
   :width: 100px
.. |image58| image:: /images/mapping/cawe/editingtools/subtool_road4.png
   :class: media
   :width: 100px
.. |image59| image:: /images/mapping/cawe/editingtools/subtool_road5.png
   :class: media
   :width: 133px
