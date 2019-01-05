.. _the_configure_cawe_options_dialog:

The Configure CaWE Options Dialog
=================================

This dialog is opened by selecting **Configure CaWE…** from
:ref:`The File Menu <mapping_cawe_menureference_file_the_file_menu>`.

It is used to configure the general appearance and handling of CaWE.

Game Configurations
-------------------

|image0|

This tab contains configuration options for game configurations. Each
map has to use a game configuration in order to work.

Game configurations are automatically created from the subdirectories of
/Games that have a game data file (.fgd) with the same name as the
directory in it. For example /Games/Deathmatch/Deathmatch.fgd is a valid
game configuration with the name Deathmatch.

In this tab you can edit properties for game configurations that will be
used when editing a map that uses this game configuration.

Game Configuration Selection
   Selects the game configuration to adjust its properties.

Default Point Entity class
   Sets the Point Entity class that is selected by default when a new
   map is opened/created.
Default Brush Entity class
   Sets the Brush Entity class that is selected by default when a new
   map is opened/created.
Default texture scale
   Sets the default texture scale for this profile.
Default lightmap scale
   Sets the default lightmap scale for this profile.
Cordon texture
   This option is not implemented at this time.

General
-------

|image1|

This tab contains general configuration options for the editor to
control its behavior. The paths to the executables in this tab are very
important so CaWE can find them to compile and run edited maps.

Undo Levels
   Sets the maximum number of actions performed by the editor that can
   be revoked.

Allow grouping/ungrouping while Ignore Groups is checked
   Toggles if you can group or ungroup selected objects even if the
   **Ignore Groups** Option from the
   :ref:`Selection Tool <the_selection_tool>` options bar is checked.

Stretch arches to fit original bounding rectangle
   This option stretches a arch to fit the bounding rectangle when
   created. If this options is not selected the size of the arch will be
   determined by the size of the bounding rectangle as well as the
   parameters set in the arch creation dialog (see
   :ref:`The New Brush Tool <the_new_brush_tool>`).

Executables
~~~~~~~~~~~

The path of executables used to compile and run maps can be configured
here in case the user changed their paths or wants to use different
ones.

Engine executable
   Sets the path to the Cafu executable.
CaBSP executable
   Sets the path to the CaBSP executable.
CaPVS executable
   Sets the path to the CaPVS executable.
CaLight executable
   Sets the path to the CaLight executable.

At the bottom of this tab, the path to your configuration data is shown.

.. _d_views:

2D Views
--------

|image2|

This tab holds configuration options that define the appearance of the
editors 2D views.

Options
~~~~~~~

Crosshair cursor
   This option is not implemented at this time.
Default to 15 degree rotations
   If rotating an object this sets the rotation steps to 15 degree
   instead of smooth rotating. Note that you can still rotate objects
   smoothly by pressing ``SHIFT`` while rotating.
Display scrollbars
   Toggles the display of scrollbars in the 2D views.
Draw vertices
   Toggles display of objects vertices in a 2D view.
White-on-Black color scheme
   Switches between white grid on black background and black grid on
   white background modes.
Keep group when done dragging
   When cloning objects from a VisGroup, this option adds the cloned
   objects to the same VisGroup after dragging them.
Center on camera after movement in 3D
   This option is not implemented at this time.
Use Visgroup colors for object lines
   Toggles if the line color defined in the Edit VisGroups dialog is
   used to display the objects that belong to this group in a 2D view.
Arrow keys nudge selected object/vertice
   If activated the arrow keys can be used to move objects or vertices.
Reorient primitives on creation in the active 2D view
   When creating a brush in a 2D view, this brushes top side is per
   default oriented in the direction of the top down 2D view. If this
   option is activated the top side of the brush will be oriented in the
   direction of the 2D view in which the brush has been created.
Automatic infinite selection in 2D windows (no ENTER)
   If dragging a selection box, this option determines if the objects in
   the selection box are selected instantly after the mouse button is
   released.
Selection box selects by center handles only
   Determines if the center handle of objects has to lie within a
   selection box for the object to be selected. Otherwise any object
   that lies partially in the selection box is selected.

Grid
~~~~

Size
   Defines the default grid size.
Intensity
   Adjusts the grids color intensity.
Highlight every 64 units
   If checked, a grid line is highlighted every 64 units.
Highlight every 1024 units
   If checked, a grid line is highlighted every 1024 units.
Highlight every *X* grid lines
   If a grid line is the *X*. grid line in a row, it is highlighted.
Hide grid smaller than 4 pixel
   If the distance between two grid lines gets smaller than 4 pixels in
   a 2D view, the grid lines are no longer displayed.
Dotted Grid
   Toggles between a grid with solid lines or dots.

.. _d_views1:

3D Views
--------

|image3|

This tab holds configuration options that define the appearance of the
editors 3D view.

Performance
~~~~~~~~~~~

Animate models
   With this option activated, models are animated in the editor.
Back clipping plane
   Sets the back clipping plane in the editors 3D view. With high values
   even geometry far away from the camera is rendered. This option has a
   huge impact on the performance of the editor, especially in big maps.
Model render distance
   Determines the distance at which models are rendered as bounding
   boxes instead of the real model meshes.

Navigation
~~~~~~~~~~

Reverse mouse Y axis (aircraft style)
   Toggles reversion of the Y axis of the mouse, when changing the
   cameras view direction in a 3D view with the mouse.
Forward speed
   Sets the maximum speed when moving forward in a 3D view.
Time to top speed (msec)
   Adjusts the time until a movement in a 3D view speeds up to its
   maximum speed.

.. |image0| image:: /images/mapping/cawe/dialogs/dialog_configure_gameconfig.png
   :class: medialeft
.. |image1| image:: /images/mapping/cawe/dialogs/dialog_configure_general.png
   :class: medialeft
.. |image2| image:: /images/mapping/cawe/dialogs/dialog_configure_2dviews.png
   :class: medialeft
.. |image3| image:: /images/mapping/cawe/dialogs/dialog_configure_3dviews.png
   :class: medialeft

