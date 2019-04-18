.. _the_main_window:

The Main Window
===============

Starting the Model Editor
-------------------------

|Start the Model Editor via the File-New or File-Open menu items.| Like
the other asset editors, the Model Editor is a part of CaWE. You start
it via menu items

-  File → Open… (Ctrl+O) *or*
-  File → New → New Model (Ctrl+Shift+N)

With both menu items, you're next presented the **File Open** dialog to
open a previously created model.

| Note that while the Model Editor can load or import models from
  :ref:`many different file formats <the_model_editorintroduction>`, it
  can *save* models only in the ``cmdl`` file format that is specific to
  the Cafu Engine: Unlike any other file format, ``cmdl`` models files
  reflect all the model features that are implemented in the Cafu
  Engine.

Main Window Elements
--------------------

|image1|

3D View
~~~~~~~

The 3D view shows the currently loaded model. The mouse and keyboard
navigation works exactly like in the 3D views of the Map Editor: See

-  :ref:`Navigating the 3D views <navigating_the_3d_views>` and
-  the :ref:`related video <mapping_cawe_views_video>`

for details.

Tip: Especially the ``MMB`` can be helpful for effective navigation in
the 3D view.

Detail Panes
~~~~~~~~~~~~

Model details, scene settings and some dialogs of the Model Editor are
presented in *panes*: Small windows that can be docked to the borders of
the parent frame, or be “floating” freely on the desktop.

If you grab a pane by its title bar and drag it over the screen, a
translucent highlight will indicate where the pane will dock when the
left mouse button is released. Press and hold the ``CTRL`` key while
dragging the pane in order to prevent it from docking at all (it will
remain floating instead).

If you close a pane, use the appropriate item in the **View** menu to
show it again at any time.

Model Element Panes
^^^^^^^^^^^^^^^^^^^

Most of the panes present model elements of a specific type:

-  the skeleton (also called the joints hierarchy),
-  meshes,
-  skins,
-  GUI fixtures,
-  animations,
-  channels,
-  submodels, and
-  level-of-detail models.

Note that for most model elements, there are in fact *two* panes: One
that shows the list of all elements in the model, and one that shows the
details of the currently selected element.

For example, the “Skeleton” pane lists all joints in the model,
hierarchically arranged. Double-click any of them to bring up the “Joint
Inspector” that shows the properties of the currently selected joint.

On the following pages in this manual there is one chapter for each
element type. Each chapter explains the model element type and its
related “list” and “inspector” panes.

The Menu
~~~~~~~~

The program menu provides access to all program features.

Refer to chapter
:ref:`Menu and Toolbar Reference <menu_and_toolbar_reference>` for a
detailed description of each menu item.

The Toolbar
~~~~~~~~~~~

The toolbar provides quick access to the most frequently used menu
items.

Move the mouse over any toolbar button to see a tooltip that describes
the purpose of the button.

.. |Start the Model Editor via the File-New or File-Open menu items.| image:: /images/modeleditor/open_new.png
   :class: mediaright
.. |image1| image:: /images/modeleditor/main_window.png
   :class: mediacenter
   :width: 720px
