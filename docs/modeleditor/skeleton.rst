.. _modeleditor_skeleton_skeleton:

Skeleton
========

The skeleton forms the basis of a model. Technically, it is built from a
hierarchy of “coordinate systems”: Starting at the root, each coordinate
system is defined in the coordinate system of its parent. Model artists
use these coordinate systems to fix the points (weights and vertices)
that eventually form the meshes of the model.

|image0| If we compare the skeleton of a model to the skeleton of the
human body, the “coordinate systems” closely correspond to “joints”: the
coordinate origin is in the center of the joint, and the coordinate axes
correspond to the bones that extend from the joint. In this analogy, a
“bone” is the line segment between a coordinate origin and the
coordinate origin of its parent system.

You can inspect the skeleton in 3D view in the
:ref:`Scene Setup <scene_setup>`:

-  set **Show Mesh** to “no”
-  set **Show Skeleton** to “yes”

The joints hierarchy
--------------------

|image1| The **Skeleton / Joints Hierarchy** pane shows all the joints
in the skeleton, hierarchically arranged. Items in the hierarchy tree
can be expanded or collapsed with the nearby symbols.

A single click on a joint selects it, a double click opens the **Joint
Inspector** pane as well.

| Pressing the **F2** key or a single-click on an already selected joint
  allows you to rename the joint in place.

Context menu
~~~~~~~~~~~~

|image2| An RMB click in the **Skeleton / Joints Hierarchy** pane opens
the context menu:

Inspect/Edit
   opens the **Joint Inspector** pane,
Rename
   allows to rename the joint,
Expand all
   expands all items in the joints hierarchy tree.

| 

The joint inspector
-------------------

|image3| The **Joint Inspector** pane shows the details of the currently
selected joint.

General
~~~~~~~

The General section shows the name of the joint (it's another option to
rename it), and the index number of the parent joint.

The parent index number is a technical detail that cannot be changed.
You need it only if you :ref:`edit the model file <cmdl_model_files>` in
a text editor or work with the Cafu `model C++ data
structures <http://api.cafu.de/c++/classCafuModelT.html>`__.

Bind pose
~~~~~~~~~

This section shows the position, orientation and scale of the joint
(i.e., the coordinate system).

Note that the details presented here only affect the *bind pose* of the
model. The *bind pose* is the default pose of the model: it is used
whenever no animation sequences are loaded or none are actively used.

That is, the **Joint Inspector** is great for manipulating *the
individual bones of static models*.

It is however useless for animated models, because technically, each
animation sequence brings its own set of (animated) joints that cannot
be manipulated individually. If you want to do that, refer to
:ref:`Transforms: translate, rotate and scale <transformstranslate_rotate_and_scale>`
instead.

The currently selected joint of the bind pose is described by these
parameters:

Pos
   defines the position of the coordinate origin relative to the parent
   system.
Qtr
   defines the first three values of the quaternion that describes the
   orientation of the coordinate system. You should *not* attempt to
   manipulate these values if you don't know what quaternions are and
   why it only shows three instead of four values.
Scale
   expresses the scale (or relative length) of the three coordinate
   axes.

| 

.. |image0| image:: /images/model-editor-6.png
   :class: mediaright
   :width: 252px
.. |image1| image:: /images/modeleditor/joints-hierarchy.png
   :class: mediaright
.. |image2| image:: /images/modeleditor/joints-hierarchy-context-menu.png
   :class: mediaright
.. |image3| image:: /images/modeleditor/joint-inspector.png
   :class: mediaright

