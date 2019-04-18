.. _the_new_entity_tool:

The New Entity Tool
===================

|image0| The New Entity tool allows you to add both new point-based and
new brush-based entities to your worlds. Please refer to the
:ref:`Introduction to Editing <introduction_to_editing>` for a quick
overview on point-based vs. brush-based entities. The
:ref:`Entity Guide <entity_guide>` lists and describes all entity types
that are available with Cafu.

The New Entity tool is activated by either clicking on its related
button on the tools toolbar, or by pressing the ``Shift+E`` keyboard
shortcut.

| When you activate the tool, the tool options bar shows controls for
  creating new entities:
| |image1|

The left half of this bar is relevant for placing point-based entities,
i.e. at ``New (point) entity type:`` you choose the type of the new
entity that is to be created. The right half of the bar allows you to
turn the currently selected brushes into brush-based entites and back.
That is, the ``Turn into (solid) entity type:`` button turns world
brushes into a brush-based entity of the type that has been chosen from
the list to the right of the button, whereas the ``Back to world``
button reverses the process by turning the currently selected
brush-based entity into regular world brushes again.

Placing Point Entities
----------------------

#. |image2| Select the Entity tool.
#. |Green Box| In one of the 2D windows, click where you want to place
   your entity. A green box will appear where you clicked. This is where
   the entity will be inserted.
#. Drag the green box to where you want the entity in the other views.
#. |Info_player_start| Select the type of entity you want to insert from
   the “New (point) Entity” dropdown box. I'm inserting an
   info_player_start.
#. |Done!| Put your mouse cursor over one of the 2D views, and press
   enter. The entity should appear. You should be able to see it in both
   the 2D and 3D views. *Note that other entities are different colours,
   shapes and sizes.*

Placing Brush-Based Entities
----------------------------

In this example, I will be adding water to a very simple map. The
process is very similar when inserting other brush entities.

#. |image6| Select the :ref:`New Brush <the_new_brush_tool>` tool.
#. In a 2D view, drag out a box the size you want your water to be.
   Adjust the size in the other 2D views.
#. *(This step is not necessary for most other entities)* Click the
   “Browse” button in the texture settings (under the tool icons). When
   the viewer pops up, change the filter to “water” (without quotes).
   Select one of the materials.
   |Texture Properties| → |Filter|
#. With your mouse cursor over one of the 2D views, press Enter. The
   brush should be selected if not, select it with the selection tool.
#. |image9| Click the New Entity tool.
#. |Dropdown Box| On the far right side, select “func_water” from the
   drop down box. Click “Turn into (solid) entity type”.

Now compile your map, and you should see water!

See Also
--------

|Start Tutorial|

| `Flash Tutorial <http://www.cafu.de/flash/Placing_a_Model.htm>`__ – A
  flash tutorial that demonstrates how entities of type
  ``static_detail_model`` are created and how their visual and collision
  models are properly set. The second part of the tutorial examines a
  collision model by example and summarizes how new collision models are
  created.

.. |image0| image:: /images/mapping/cawe/editingtools/cawe_toolbar_newentity.png
   :class: medialeft
   :width: 80px
.. |image1| image:: /images/mapping/cawe/editingtools/cawe_tooloptionsbar_newentity.png
   :class: media
.. |image2| image:: /images/mapping/cawe/reference/tool.png
   :class: medialeft
.. |Green Box| image:: /images/mapping/cawe/reference/green_box.png
   :class: mediaright
.. |Info_player_start| image:: /images/mapping/cawe/reference/dropdown.png
   :class: mediaright
.. |Done!| image:: /images/mapping/cawe/reference/finished_point.png
   :class: mediaright
.. |image6| image:: /images/mapping/cawe/reference/w_newbrush.png
   :class: medialeft
.. |Texture Properties| image:: /images/mapping/cawe/reference/mats.png
   :class: media
.. |Filter| image:: /images/mapping/cawe/reference/filter.png
   :class: media
.. |image9| image:: /images/mapping/cawe/reference/tool.png
   :class: medialeft
.. |Dropdown Box| image:: /images/mapping/cawe/reference/solid.png
   :class: mediaright
.. |Start Tutorial| image:: /images/starttutorial.png
   :class: medialeft
   :target: http://www.cafu.de/flash/Placing_a_Model.htm
