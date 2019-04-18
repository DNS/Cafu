.. _d_and_3d_views:

2D and 3D Views
===============

The 2D and 3D views are the center of the activities when you work with
a map. They have a number of features that make working with them very
comfortable and straightforward.

In order to familiarize yourself with the material in this section
quickly, we recommended that you try it out and reproduce it with one of
the example maps that come with Cafu. Use the **File → Open…** menu for
loading a map from the ``DeathMatch/Maps`` directory, e.g.
``Kidney.cmap``.

By default, the :ref:`main window <the_main_window_user_interface>` is
divided into three views that show your map from different sides and
perspectives. In the center is the 3D view, which shows the map as a
perspective rendering. The two 2D views show the map from the top and
from the side like an architects plan.

The view windows
----------------

|The title bar and buttons of a view window.| Each view is a window
whose title bar indicates the type and render mode of the view. You can
maximize, minimize or close each view window with the buttons in the
upper right corner. In order to create a new 2D or 3D view, choose
**View → New 2D view** or **View → New 3D view** from the main menu.

By clicking and dragging a windows title bar, the views can be docked,
undocked and arranged as you like. Press ``Ctrl`` while dragging a
window in order to keep it floating (prevent it from docking to one of
the highlighted dock positions).

.. _the_active_view:

The active view
---------------

A view is *activated* when you move the mouse pointer into its window.

Keyboard input, menu commands and status bar information all refer to
the most recently activated view. For example, if you want to navigate
the views with the keyboard as presented below, just move the mouse
pointer into the desired view window, and all keyboard input will be
directed to it.

.. _ch_view_mode:

Changing the view mode
----------------------

|The context menu of a 3D view.| |The context menu of a 2D view.| In
order to change the mode of a 2D or 3D view, use:

-  the ``Tab`` key,
-  the ``Shift+Tab`` key,
-  or the context menu (right mouse button click).

| 
| The 2D views show the map from the top, the front or the side:

|2D Top| |2D Front| |2D Side|

The 3D views show the map in different render modes, for example
“wire-frame”, “flat colored”, “edit materials” or “full materials”:

|3D Wireframe| |3D Flat| |3D Full Materials|

Note that render mode “3D Edit Mats” often looks much like “3D Full
Mats”, but whereas the latter shows the materials in their “natural”
appearance (as in the game engine, e.g. translucent, distorted,
invisible, black, etc.), “3D Edit Mats” shows plainly textured surfaces
instead. This can be very helpful for seeing the materials properly for
editing purposes.

Navigating the 2D views
-----------------------

Use the following keyboard and mouse input in order to navigate the 2D
views:

============================= ================================================================================================================================================
Keyboard Input                Action
============================= ================================================================================================================================================
Arrow keys ``←↑↓→``           Scroll the map into the direction of the arrow.
``Space``                     Pan the view: while the key is being held down, scroll the map by moving the mouse.
``+`` or ``-``                Zoom in or out (on mouse pointer).
``1``, ``2``, ``3``, …, ``0`` Zoom to a preset level.
``Ctrl+E``                    Center all 2D views onto the currently selected object(s).
``Z``                         Toggle the pan feature: Pressing ``Z`` once is like holding down ``RMB`` or ``Space`` all the time. Pressing ``Z`` anew turns panning off again.
``Tab``                       Cycle through top, front or side view modes as described :ref:`above <ch_view_mode>` (``Shift+Tab`` for opposite order).
Mouse Input                   Action
``RMB`` (click)               Open the context menu (or apply a tool-specific function).
``RMB`` (drag)                Pan the view: while the button is being held down, scroll the map by moving the mouse.
Wheel                         Zoom in or out (on mouse pointer).
============================= ================================================================================================================================================

In the status bar at the bottom of the screen you will see both the
current position of the mouse pointer in world coordinates as well as
the current zoom level of the active view. The currently active tool may
provide additional information in the status bar as well.

.. _navigating_the_3d_views:

Navigating the 3D views
-----------------------

Each 3D view has an associated camera that can be manipulated in order
to show the map from arbitrary points, directions and perspectives. Use
the following keyboard and mouse input in order to navigate (the cameras
of) the 3D views:

Keyboard Input
Action

Arrow keys ``←↑↓→``

Rotate the camera (look around).

``W``, ``A``, ``S``, ``D``

Move the camera left/right and forwards/backwards (along its depth
axis).

``Space``

An alternative to the ``RMB`` that works exactly alike, including all
combinations of ``Shift`` and ``Ctrl``.

``Z``

Toggle camera control: Pressing ``Z`` once is like holding down ``RMB``
or ``Space`` all the time. Pressing ``Z`` anew turns camera control off
again.

``Tab``

Cycle through the render modes as described :ref:`above <ch_view_mode>`
(``Shift+Tab`` for opposite order).

``1``, ``2``

Advanced: Move the far clip plane closer to or farther from the camera.

Mouse Input

Action

``RMB`` (click)

Open the context menu (or apply a tool-specific function).

``RMB`` (drag)

Rotate: Move the mouse to rotate the camera (look around).

``RMB+Shift``

Pan: Move the mouse to pan the camera left/right and up/down.

``RMB+Ctrl``

Fly: Move the mouse to pan the camera left/right and move it
forwards/backwards.

``RMB+Ctrl+Shift``

Walk: Move the mouse to rotate and move the camera in the XY-plane.

``MMB`` (drag)

Orbit around the point under the mouse pointer (horizontally and
vertically).

``MMB+Shift``

Orbit, and move the camera closer to or farther from the object under
the mouse pointer.

``MMB+Ctrl``

Same as ``MMB+Shift``.

Wheel

| “Zoom” (move the camera closer to or farther from the object under the
  mouse pointer).
| Combine with ``Ctrl`` *or* ``Shift`` for slower (but more precise)
  movement.
| Combine with ``Ctrl`` *and* ``Shift`` for even slower (but even more
  precise) movement.

Notes and references:

-  Except for opening the context menu, which requires a RMB *click*,
   the mouse buttons RMB and MMB must be pressed *and held* while the
   mouse is moved to control the camera.
-  You can use the :ref:`Camera Tool <the_camera_tool>` for more exotic
   (and less frequently needed) camera control features, such as
   creating additional cameras or setting the camera origin and
   orientation in a 2D view.
-  The :ref:`Configure CaWE <d_views1>` dialog has options that affect
   the 3D views and their cameras, e.g. for setting the movement speed
   or reversing the mouse Y-axis.

.. _mapping_cawe_views_video:

Video
-----

`This video <https://youtu.be/xLe1xmdA8YY>`__ demonstrates the most
important concepts for navigating the 2D and 3D views.

| 
| You can also download the high-quality edition:
  `CaWE_Navigating_the_Views.mp4 <http://www.cafu.de/files/videos/CaWE_Navigating_the_Views.mp4>`__

.. |The title bar and buttons of a view window.| image:: /images/mapping/cawe/views/2d_view_titlebar.png
   :class: mediaright
.. |The context menu of a 3D view.| image:: /images/mapping/cawe/views/3d_view_context_menu.png
   :class: mediaright
.. |The context menu of a 2D view.| image:: /images/mapping/cawe/views/2d_view_context_menu.png
   :class: mediaright
.. |2D Top| image:: /images/mapping/cawe/views/2d_view_top.png
   :class: media
.. |2D Front| image:: /images/mapping/cawe/views/2d_view_front.png
   :class: media
.. |2D Side| image:: /images/mapping/cawe/views/2d_view_side.png
   :class: media
.. |3D Wireframe| image:: /images/mapping/cawe/views/3d_view_wireframe.png
   :class: media
.. |3D Flat| image:: /images/mapping/cawe/views/3d_view_flat.png
   :class: media
.. |3D Full Materials| image:: /images/mapping/cawe/views/3d_view_full.png
   :class: media

