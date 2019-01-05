.. _scene_setup:

Scene Setup
===========

One of the Model Editor's main fields of application is to let you
visually inspect the model as rendered by the Cafu Engine and as it will
appear in the game.

For this purpose, the **Scene Setup** dialog lets you change the details
on how the Model Editor renders the model in the 3D view. Refer to
section :ref:`The Main Window <the_main_window>` for more information
about the 3D view itself.

|image0|

General
-------

Background Color
   sets the “sky” or background color. Future versions of the Model
   Editor will allow to load cube maps for proper sky environment
   rendering as well.
Show Origin
   determines whether the principle axes of model space, emanating from
   the origin, are shown.
Show Grid
   , when selected, renders a spatial coordinate grid, centered at the
   origin.
Grid Spacing
   sets the spacing between grid lines.

Camera
------

Camera attributes are rarely entered manually: Normally, you navigate
the 3D view as described at section
:ref:`The Main Window <the_main_window>`, and the camera details update
automatically.

-  **Pos** shows the x-, y- and z-coordinates of the camera position.
-  **Angles** shows the orientation of the camera as angles for
   **Pitch** and **Yaw**.
-  The **Advanced** settings control the shape of the camera's view
   frustum: **vertical FOV** is the vertical field-of-view, and
   **near/far plane dist** is the distance of the near/far clipping
   plane relative to the cameras position.

Scene Elements
--------------

Ground Plane
~~~~~~~~~~~~

Show
   determines if the ground plane is shown at all.
Height (z-Pos)
   sets the z-position of the ground surface.
Auto Height
   , if set, automatically adjusts the z-position of the ground surface
   to the dimensions of the model.
Material
   sets the material that is used to render the ground plane.

Model
~~~~~

Show Meshes
   determines whether the meshes of the model are rendered.
Show Skeleton
   , if activated, renders the joints and the “bones” of the model.
Show triangle normals
   , if activated, renders the normal vector of each triangle. The color
   of the normal vector indicates the
   `Polarity <http://api.cafu.de/c++/structCafuModelT_1_1MeshT_1_1TriangleT.html#a9ebe7b31c1b2de715578434c1dd6b03d>`__
   of the triangle.
Show tangent-space
   , if activated, renders the axes of tangent space at each vertex of
   each mesh.
Debug material
   allows to override the mesh materials of the currently selected skin
   with “debug” materials that facilitate the inspection of the model:
   **plain (white)** is useful for inspecting the shades resulting from
   lighting, **wire-frame** shows the individual triangles that the
   meshes are composed of.

Animation Control
-----------------

Frame No.
   is the frame number in the currently playing sequence. It advances
   automatically if an animation is playing, but can be set manually as
   well.
Speed
   is the relative speed with which the animation is currently playing.
   It's automatically set to 0 or 1 if you press the Play or Pause
   buttons in the toolbar.
Loop
   , if set, forces the Model Editor to play the current animation in an
   infinite loop.

Light Sources
-------------

Ambient Light Color
   is the color of the ambient light. Ambient light is also there when
   all other light sources have been turned off.

Light 1, 2, 3
   are dynamic light sources as they can also be set in the Map Editor
   and occur in the game:

   On
      determines whether the light source is on or off.
   Cast Shadows
      sets if the light source casts shadows at all.
   Pos
      the position of the light source.
   Radius
      the radius of the light source.
   Color
      the color of the light source.

.. |image0| image:: /images/modeleditor/scene-setup.png
   :class: mediaright

