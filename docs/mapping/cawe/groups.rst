.. _mapping_cawe_groups_groups:

Groups
======

Groups are a great aid for organizing your map and for making many
mapping tasks easier. Map elements that have been put into a group can
be treated as a whole, and thus they can be edited, transformed and
modified as if they were a single object. Groups can also hide their
elements from the views and lock them in order to prevent them from
being edited.

In the latest versions of CaWE, groups are also persistent: Groups are
saved in and loaded from Cafu map files so that they are available from
one session to the next. Moreover, any changes to groups and their
properties are recorded in the edit history and can be undone via the
**Edit → Undo** menu.

In summary, groups act like layers in image processing programs. They
are helpful and easy to use. This section explains the details.

Creating Groups
---------------

In order to create a group, use the
:ref:`Selection tool <the_selection_tool>` in order to select the map
elements that you want to become the members of the new group. Then use
one of the following methods in order to put the selected elements into
a new group:

-  Use the **Tools → Group (Ctrl+G)** menu.
-  Click one of the buttons **Group**, **Hide**, or **Hide Other** in
   the :ref:`Selection tool <the_selection_tool>` toolbar.

Each of these menu items or buttons creates a new group and puts the
selected map elements into it. The **Hide Other** button is an
exception, as it will group everything else. The two **Hide** buttons
additionally mark the newly created group as hidden, so that the
selected map objects will disappear from the views.

The Groups Panel
----------------

|The groups panel| The Groups panel shows a list of all groups in the
map. If the Groups panel is currently not visible, use the **View →
Panels → Groups** menu to show it.

Each group is shown in a line with information about the groups
settings: its visible/hidden status, its name, its color, its
edit/locked status, and whether the group members are selected
individually or as a whole. Each of the icons can be clicked in order to
toggle its status.

Group Visibility
^^^^^^^^^^^^^^^^

| The members of a group can be hidden from the views:
| |image1| the group is visible.
| |image2| the group is hidden.
| This is very useful for getting things “out of the way” when you're
  editing a portion of the map that is usually occluded by other map
  objects. It's also a nice trick to enhance the performance on older
  computer systems: When the rendering is too slow, just hide the parts
  of the map that you're currently not editing, and the rendering of the
  remaining objects of interest will be much smoother.

Group Name and Color
^^^^^^^^^^^^^^^^^^^^

The initial name of a group is automatically assigned when the group is
first created, and can be changed at any time: Press **F2**,
single-click a selected group, or use the context menu (described below)
in order to assign a new name, e.g. one that describes the content or
purpose of the group.

When you double-click the group name, the members of the group are
selected in the map.

The groups color is indicated by the background color of the group name.
When activated in the :ref:`CaWE Options <d_views>` dialog, the member
elements of a group are rendered in the groups color, so that they are
easy to see in the map views.

Locking Groups
^^^^^^^^^^^^^^

| The members of a group can be prevented from being clicked in the map
  views:
| |image3| the members of the group can be selected.
| |image4| the group is locked.
| This icon toggles whether groups can be selected with the
  :ref:`Selection tool <the_selection_tool>`. When the icon shows the
  lock, the members of the groups cannot be selected in the 2D or 3D
  views, and thus they cannot become the subject of inadvertent edits,
  transformations, or other modifications.

Group Selection
^^^^^^^^^^^^^^^

| The grouping nature of a group can be turned on and off:
| |image5| the members of the group are selected individually.
| |image6| the members of the group are selected as a whole.
| This icon toggles the actual grouping behavior of the group. Usually,
  you will want to have this set to “select as a whole”, so that a mouse
  click on a part of a group selects the whole group. However, it can be
  preferable to have this set to “select individually”, for example when
  the primary purpose of the group is to temporarily hide or lock its
  elements.

Editing groups
--------------

|The groups context menu| Groups can be edited via the Groups panels
context menu: Right-click into the Group panel in order to open its
context menu:

Select
   Selects all members of the group in the map, just as if they had been
   selected with the :ref:`Selection tool <the_selection_tool>`. This is
   the same as a double-click on the group name.

Edit
   Using the submenu, you can change the properties of one or more
   groups that are selected in the Groups panel.

Dissolve
   This menu item ungroups the members of the group and then deletes the
   group. Note that it does not delete the map elements itself, they're
   just ungrouped.

Merge groups
   When two or more groups are selected, their members can be merged
   into a single group.

Move up/down
   Use these menu items to change the order of the groups in the list.

It is important to note that *multiple* groups can be edited via the
context menu: Just select one or more groups, and the context menu
operation will affect them all at once.

Any changes to groups and their properties are recorded in the edit
history and can be undone via the **Edit → Undo** menu.

.. |The groups panel| image:: /images/mapping/cawe/groups/panel.png
   :class: mediaright
.. |image1| image:: /images/mapping/cawe/groups/eye.png
   :class: media
.. |image2| image:: /images/mapping/cawe/groups/eye_grey.png
   :class: media
.. |image3| image:: /images/mapping/cawe/groups/wrench.png
   :class: media
.. |image4| image:: /images/mapping/cawe/groups/lock.png
   :class: media
.. |image5| image:: /images/mapping/cawe/groups/groupselect-indiv.png
   :class: media
.. |image6| image:: /images/mapping/cawe/groups/groupselect-asone.png
   :class: media
.. |The groups context menu| image:: /images/mapping/cawe/groups/contextmenu.png
   :class: mediaright

