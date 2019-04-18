.. _gui_fixtures:

GUI Fixtures
============

| GUI fixtures are used to attach GUIs to models.

|image0| GUIs are virtual computer desktops that the player can interact
with to unlock doors, call lifts, obtain information, and much more.

| They are created in the Cafu Engine Gui Editor and use scripts for
  their implementation and for handling events when interacting with the
  player.

The GUI fixtures list
---------------------

|image1| The **GUI Fixtures** pane lists all GUI fixtures in the model.

For each GUI fixture,

-  its name and
-  the GUI fixture number

is shown.

A single click on a GUI fixture selects it, a double click opens the
**GUI Fixture Inspector** pane as well.

Pressing the **F2** key or a single-click on an already selected GUI
fixture allows you to rename the GUI fixture in place.

| The “\ **+**\ ” button creates a new GUI fixture and adds it to the
  list. The “\ **-**\ ” button deletes the currently selected GUI
  fixtures.

Context menu
~~~~~~~~~~~~

|image2| An RMB click in the **GUI Fixtures** pane opens the context
menu:

Inspect/Edit
   opens the **Gui Fixture Inspector** pane.
Rename
   allows to rename the GUI fixture.
Add/create new
   , like the “\ **+**\ ” button, creates a new GUI fixture and adds it
   to the list.

| 

The GUI fixture inspector
-------------------------

|image3| The **GUI Fixture Inspector** pane shows the details of the
currently selected GUI fixture.

Name
~~~~

Shows the name of the currently selected GUI fixture. The name can be
edited in order to rename the GUI fixture.

Origin and endpoints
~~~~~~~~~~~~~~~~~~~~

The origin and the endpoints of the x- and y-axes determine the position
and orientation of the GUI.

You can enter and edit the mesh and vertex numbers manually, but it is
much easier and faster to right-click directly on the model in the 3D
view:

|image4|

Selecting one of the three **GUI fixture** menu items will fill in the
related numbers in the **GUI Fixture Inspector** automatically.

Translation and scale
~~~~~~~~~~~~~~~~~~~~~

Normally, the GUI rectangle is exactly aligned to the origin and the
endpoints of the axes. Using the translation and scale, you can move the
GUI rectangle from its original position and adjust its overall size:

-  The **Scale** values set the relative lengths of the axes.
-  The **Translation** values move the origin in multiples of the scaled
   axes.

| 

Example video
-------------

`This video <https://youtu.be/dNCf82E90TA>`__ shows a GUI that is
attached to an animated model and uses custom translation and scale
settings.

.. |image0| image:: /images/model-editor-3.png
   :class: mediaright
   :width: 252px
.. |image1| image:: /images/modeleditor/gui-fixtures-list.png
   :class: mediaright
.. |image2| image:: /images/modeleditor/gui-fixtures-list-context-menu.png
   :class: mediaright
.. |image3| image:: /images/modeleditor/gui-fixture-inspector.png
   :class: mediaright
.. |image4| image:: /images/modeleditor/3d-view-context-menu.png
   :class: mediacenter

