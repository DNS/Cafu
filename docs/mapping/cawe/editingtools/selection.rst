.. _the_selection_tool:

The Selection Tool
==================

|image0|

The Selection tool allows you to select objects like brushes, entire
entities or bezier patches in your map and move, scale, rotate or shear
them. Apart from these functions directly activated by the Selection
tool, CaWE provides a lot more functions that are related to selected
objects. Since these functions are only usable with one or more objects,
the Selection tool is a very important instrument to create maps.

You can activate the Selection tool by clicking on the related icon
marked in the left screen or by pressing the ``SHIFT+S`` keyboard
shortcut.

Selecting objects
-----------------

To select an object, click the object once. In the 3D view, click on any
surface that is part of the object to select it. In the 2D view click
either on an edge that is part of the object or on its center marker.

In the 3D view, the surface of the object will be changed in color and
its edges are colored yellow to mark it as selected. In the 2D view, the
edges of the selected object are colored red.

|image1| |image2|

To select one or more objects you can also drag a selection box in a 2D
view window. Releasing the mouse button opens the selection box, but
does not select the objects yet. You can move and scale this selection
box until it covers all objects you want selected. To eventually select
the objects, press the ``ENTER`` key. This way all objects that are
covered whole or partially by this selection box are selected. Note that
you can also scale the selection box in another 2D view to create a
spatial selection box and narrow down the selected objects.

|image3| |image4|

You can change the behavior of a selection box in the
:ref:`Configure CaWE Options <the_configure_cawe_options_dialog>` dialog
at the **2D Views** tab. **Automatic infinite selection in 2D windows
(no ENTER)** enables you to drag a selection box and instantly select
all objects by releasing the mouse button instead of pressing the
``ENTER`` key. This way however you loose the ability to open a spatial
selection box. If **Selection box selects by center handle only** is
activated, only objects whose center handle is covered by a selection
box count as selected objects.

Another way to select more than one object is to select objects while
pressing the ``CTRL`` key. This way you can choose as many objects as
you want and perform an action on all these objects simultaneously.

You can select all objects in a map by clicking the **Select All**
option from the
:ref:`edit menu <mapping_cawe_menureference_edit_the_edit_menu>`. To
deselect all objects, use the **Select None** option from the
:ref:`edit menu <mapping_cawe_menureference_edit_the_edit_menu>`.

You can also deselect all selected objects by pressing the ``ESC`` key,
which clears the selection.

Working with selected objects
-----------------------------

Now that one ore more objects have been selected you can perform actions
on them.

Moving and Cloning objects
~~~~~~~~~~~~~~~~~~~~~~~~~~

|image5|

By clicking inside the selection box of an object, you can drag it in a
2D view and move its position to a new location. If grid snapping is
activated, you can ignore it while moving the object by pressing the
``ALT``-key. This also works with the transformations described below.

You can also move objects with the “arrow” keys, if you select the
**Arrow keys nudge selected object/vertex** option in the
:ref:`Configure CaWE Options <the_configure_cawe_options_dialog>` dialog
under **2D Views**.

An easy way to duplicate objects is using the **Clone** function while
moving an object. Simply move your object as described above, but before
releasing the left mouse button to move the object, press and hold the
``SHIFT`` key. In this way a clone of the object is created at its new
location. Note that cloning an object while moving it with the “arrow”
keys is not supported.

Tranformation modes
~~~~~~~~~~~~~~~~~~~

To resize, rotate or shear an object you have to select the appropriate
transformation mode. You can switch between the 3 modes by clicking
inside the selection box of the object you want to transform. Different
transformation modes are marked by handles at the edges and vertices of
the selection box.

Resizing an object
~~~~~~~~~~~~~~~~~~

|image6| |image7|

A dashed rectangular box is drawn around a selected object by the editor
(this box is only visible if the object itself isn't rectangular). The
resize mode is marked by white squares at the edges and vertices of this
box. By moving the mouse over one of these squares and dragging this
square by holding down the left mouse button you can resize the object
in the direction of the square you clicked on.

Rotating an object
~~~~~~~~~~~~~~~~~~

|image8| |image9|

The rotate mode is marked by white dots at the vertices of the box.
Moving the mouse over one of these dots and dragging this dot by holding
down the left mouse button rotates the object smoothly. It is also
possible to rotate the object in multiples of 15 degrees by pressing and
holding down the ``SHIFT``-key while rotating an object.

You can change the default behavior to rotating in 15 degree steps if
you select the **Default to 15 degree rotations** option in the
:ref:`Configure CaWE Options <the_configure_cawe_options_dialog>` dialog
under **2D Views**. If this option is select, pressing ``SHIFT`` will
allow you to rotate the object smoothly.

Shearing an object
~~~~~~~~~~~~~~~~~~

|image10| |image11|

White squares at the edges of the selection box mark the shearing mode.
By left clicking on a square and moving the mouse while holding the left
mouse button down this edge can be moved in two directions along its own
axis and in this way the object is deformed.

Deleting an object
~~~~~~~~~~~~~~~~~~

To delete a selected object just press the ``DEL``-key on your keyboard
or choose **Delete** from the
:ref:`Edit menu <mapping_cawe_menureference_edit_the_edit_menu>`.

More
~~~~

There are a lot more things you can do with selected objects. These
additional functions are explained in the documentations of the
:ref:`Edit menu <mapping_cawe_menureference_edit_the_edit_menu>`,
:ref:`Map menu <the_map_menu>`,
:ref:`View menu <mapping_cawe_menureference_view_the_view_menu>` and
:ref:`Tools menu <the_tools_menu>`.

The Tool Options Bar
--------------------

|:mapping:cawe:editingtools:selectiontoolbar.png|

Once the Selection tool is activated, its option bar is visible above
the view windows. This bar contains selection parameters and buttons for
further functions related to selections.

Ignore Groups
   This option specifies if grouped objects are selected as a group
   (selecting one object selects all objects) or if groups are ignored
   and objects are selected normally even if they are part of an object
   group.
Lock Materials
   This option determines if the Material of an object is locked to the
   object itself and therefore doesn't change, even tough the object is
   moved. If this option is not selected the Material is locked onto the
   point of origin of the world coordinate system and therefore the part
   of the texture visible on the object may change when the object is
   moved.
Group
   This button puts two ore more previously selected objects into a
   group. Grouped objects can be selected all at once by clicking on one
   object of the group.
Ungroup
   This button reverts grouped objects back to single objects. This
   function only works if a group of objects has previously been
   selected.
Hide
   Hides all selected objects from the view windows. The objects are not
   deleted, but simply not displayed, which is particularly useful when
   working with big maps that contain a lot of objects.
Hide Other
   Hides all objects except the selected objects.
Apply Material
   Applies the material that is currently selected in the
   :ref:`materials bar <the_main_window_user_interface>` to the selected
   object(s).

Selection tool keyboard shortcuts
---------------------------------

-  ``SHIFT+S``:

   -  Activates the Selection tool.

-  ``ENTER``:

   -  If a box selection is opened → selects all objects that are part
      of the box selection .

-  ``ESC``:

   -  If a box selection is opened → closes the box selection.
   -  If an object is currently resized, rotated, sheared or moved →
      move object back into original position and form.
   -  If objects are selected → deselect objects.

-  ``DEL``:

   -  Deletes the selected object(s) from map.

-  ``CTRL``:

   -  If in selection-mode → add new selected objects to selection list
      instead of removing old selection and only selecting clicked
      object.

-  ``ALT``:

   -  While moving or cloning an object → ignore grid snapping and move
      object smoothly
   -  While tranforming an object → ignore grid snapping and transform
      object smoothly

-  ``SHIFT``:

   -  If Rotating an object → rotate in multiples of 15 degrees.
   -  If moving an object → object is cloned first then moved, so
      original object remains in place.

-  ``Page up``:
-  ``Page down``:

   -  If last select click covered more than one object → cycle trough
      objects.

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_selection.png
   :class: medialeft
   :width: 80px
.. |image1| image:: /images/mapping/cawe/editingtools/selectedcube3d.png
   :class: media
.. |image2| image:: /images/mapping/cawe/editingtools/selectedcube2d.png
   :class: media
.. |image3| image:: /images/mapping/cawe/editingtools/selectionboxdrag.png
   :class: media
.. |image4| image:: /images/mapping/cawe/editingtools/selectionboxopen.png
   :class: media
.. |image5| image:: /images/mapping/cawe/editingtools/selectionmoveclone.png
   :class: media
.. |image6| image:: /images/mapping/cawe/editingtools/selectscale.png
   :class: media
.. |image7| image:: /images/mapping/cawe/editingtools/selectscaleactive.png
   :class: media
.. |image8| image:: /images/mapping/cawe/editingtools/selectrotate.png
   :class: media
.. |image9| image:: /images/mapping/cawe/editingtools/selectrotateactive.png
   :class: media
.. |image10| image:: /images/mapping/cawe/editingtools/selectshear.png
   :class: media
.. |image11| image:: /images/mapping/cawe/editingtools/selectshearactive.png
   :class: media
.. |:mapping:cawe:editingtools:selectiontoolbar.png| image:: /images/mapping/cawe/editingtools/selectionoptionsbar.png
   :class: media

