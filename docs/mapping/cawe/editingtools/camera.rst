.. _the_camera_tool:

The Camera Tool
===============

|image0|

In section :ref:`2D and 3D Views <d_and_3d_views>` we have explained how
2D and 3D views work and how you use them to explore and navigate the
map. We have also mentioned (albeit briefly) that each 3D view has a
“camera” assigned: A camera specifies a viewer *origin* (sometimes also
called “eye position”) and an *orientation* (“direction”). When you use
the mouse or keyboard to look, move, pan, zoom or orbit in a 3D view,
you actually manipulate the related camera position and orientation, and
the 3D view in turn uses the updated camera details to update the scene
rendering.

We believe that the mouse and keyboard camera controls of section
:ref:`2D and 3D Views <d_and_3d_views>` are so powerful and easy to use
that you will use them virtually all the time.

The purpose of the Camera tool is to add the less frequently needed
extras: You can create new cameras (and assign them in turns to 3D
views), and edit their origins and orientations in the 2D views.

To activate the Camera tool, click on the related icon in the tool bar
or press ``Shift+C``.

Camera display in 2D views
--------------------------

|Cameras in a 2D view when a tool other than the Camera tool is active.
The most recently used camera is highlighted.| When a tool other than
the Camera tool is active, cameras are shown with a gray dot at their
origin and a gray line that indicates their viewing direction
(orientation).

| The most recently used (or changed) camera is highlighted in a
  brighter shade of gray: For example,
  :ref:`activating a 3D view <the_active_view>` highlights the related
  camera.

|Cameras in a 2D view when the Camera tool is active. The most recently
used camera is highlighted.| When the Camera tool is active, the cameras
are displayed in colors so that they are easier to spot in the map.

| As before, the most recently used (or changed) camera is highlighted
  in brighter colors.

Creating and deleting cameras
-----------------------------

|Use Shift+LMB to create a new camera.| To create a new camera, press
and hold the ``Shift`` key, then use the left mouse button to drag a
line in one of the 2D views. The starting point of the line becomes the
camera origin, the line itself defines the orientation.

Having multiple cameras distributed at key locations in the map can
facilitate map navigation a lot, as a 3D view can cycle through all
available cameras as described :ref:`below <switching_cameras>`.

At this time, cameras that are not assigned to a 3D view are not saved
with the map; they are lost when the map is closed. You can manually
delete an active camera via the **Edit → Delete** menu item. The last
camera cannot be deleted, as there is at least one 3D view that it is
assigned to.

Manipulating the origin and orientation
---------------------------------------

The most powerful and most convenient methods to manipulate a camera are
the controls described in section
:ref:`Navigating the 3D views <navigating_the_3d_views>` (in addition,
the controls described there do not require the Camera tool to be
active: they work always).

Sometimes however, it is worthwhile to set the origin and/or the
orientation in a 2D view:

|Click and drag the dot in order to move the camera origin.| Click and
drag the dot in order to move the camera origin.

| • Holding the ``Alt`` key while dragging toggles grid snapping.
| • Holding the ``Ctrl`` key moves the line as well (and thus the camera
  as a whole).

|Click and drag the end of the line in order to change the camera
orientation.| Click and drag the end of the line in order to change the
camera orientation. (The *length* of the line does not matter. You can
for example drag the end of the line onto the object of interest, to
center it precisely in the related 3D view.)

| • Holding the ``Alt`` key while dragging toggles grid snapping.
| • Holding the ``Ctrl`` key moves the dot as well (and thus the camera
  as a whole).

.. _switching_cameras:

Switching cameras
-----------------

In order to assign a 3D view (the :ref:`active <the_active_view>` 3D
view) another camera, use the ``Page Up`` and ``Page Down`` keys to
cycle through all cameras in the map.

Camera tool keyboard shortcuts
------------------------------

============= ==================================================================================
Key           Action
============= ==================================================================================
``Page Up``   Assign the next camera to the currently active 3D view.
``Page Down`` Assign the previous camera to the currently active 3D view.
``ESC``       Quit the camera tool and switch to the :ref:`Selection <the_selection_tool>` tool.
``Shift``     LMB-clicking and dragging in a 2D view creates new camera.
``Ctrl``      Move the camera as a whole when dragging one of its handles.
``Alt``       Temporarily toggle grid snapping while dragging a camera handle.
============= ==================================================================================

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_camera.png
   :class: medialeft
   :width: 80px
.. |Cameras in a 2D view when a tool other than the Camera tool is active. The most recently used camera is highlighted.| image:: /images/mapping/cawe/editingtools/cameras_2d_tool_other.png
   :class: medialeft
.. |Cameras in a 2D view when the Camera tool is active. The most recently used camera is highlighted.| image:: /images/mapping/cawe/editingtools/cameras_2d_tool_camera.png
   :class: medialeft
.. |Use Shift+LMB to create a new camera.| image:: /images/mapping/cawe/editingtools/cameras_create.png
   :class: medialeft
.. |Click and drag the dot in order to move the camera origin.| image:: /images/mapping/cawe/editingtools/cameras_move_pos.png
   :class: medialeft
.. |Click and drag the end of the line in order to change the camera orientation.| image:: /images/mapping/cawe/editingtools/cameras_move_dir.png
   :class: medialeft

