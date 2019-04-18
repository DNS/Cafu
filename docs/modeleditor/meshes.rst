.. _modeleditor_meshes_meshes:

Meshes
======

The meshes define the shape of the model.

A model can have several meshes. Each mesh is made of triangles, whose
vertices are composed of one or more weights, that in turn are attached
to the :ref:`Skeleton <modeleditor_skeleton_skeleton>` of the model.

Despite its name, it's not the purpose of the Model Editor to modify the
positions of weights and vertices, or to provide means to generally
shape the meshes. Please see the
:ref:`The Model Editor: Introduction <the_model_editorintroduction>` for
more details. Instead, the Model Editor focuses on setting details that
are specific to the Cafu Engine.

The meshes list
---------------

|image0| The **Meshes** pane lists all meshes in the model.

For each mesh,

-  its name,
-  the mesh number, and
-  the mesh material (in the currently selected
   :ref:`skin <modeleditor_skins_skins>`)

is shown.

A single click on a mesh selects it, a double click opens the **Mesh
Inspector** pane as well.

Pressing the **F2** key or a single-click on an already selected mesh
allows you to rename the mesh in place.

| The “\ **-**\ ” button at the top of the list deletes the currently
  selected meshes.

Context menu
~~~~~~~~~~~~

|image1| An RMB click in the **Meshes** pane opens the context menu:

Inspect/Edit
   opens the **Mesh Inspector** pane.
Rename
   allows to rename the mesh.
Project new UV-coords…
   is a tool for hot-fixing models that did not bring proper
   UV-coordinates, or none at all:

| |image2|

The mesh inspector
------------------

|image3| The **Mesh Inspector** pane shows the details of the currently
selected mesh.

Name
~~~~

Shows the name of the currently selected mesh. The name can be edited in
order to rename the mesh.

Material
~~~~~~~~

The material that is used for rendering the mesh is set here: Press the
“…” button in order to open the **Material Browser**.

|image4| The Material Browser is the same as the one used in the Map
Editor, but it only shows the materials that are specific to the
currently loaded model. It also has some basic capabilities to *edit*
materials and to set their properties, but is currently not very
advanced.

At this time, we recommend that you use the Material Browser for
assigning materials to meshes, but in order to create and edit material
definitions, best use a programmers editor for editing the
:ref:`related cmat material files <model_files_explained>` directly:
Material scripting is documented in chapter
:ref:`The Cafu Material System <the_material_system>`.

(It is our goal to turn the Material Browser into a full-featured
Material Editor in the near future, but we currently work on improving
other parts of the Cafu code. If you want to lend a helping hand, please
let us know!)

Tangent-space method
~~~~~~~~~~~~~~~~~~~~

The tangent-space method determines the algorithm that is used for
computing the normal, tangent and bi-tangent vectors at the vertices of
the meshes.

Ideally, the algorithm that is set here should match the algorithm that
was used by the external program that was used for creating the *normal
map* for this mesh. This is important so that the Cafu model code can
accurately reproduce the subtle details involved in generating the axes
of tangent-space, as only then the lighting results will exactly match
those in the original program. Unfortunately, the involved algorithms
are not standardized at this time (and often not even documented!), so
that we can only try a best guess.

Refer to
`CafuModelT::MeshT::TangentSpaceMethodT <http://api.cafu.de/c++/structCafuModelT_1_1MeshT.html#a02f1d4ddd0c2dce4062a1b279ccd130d>`__
for the currently available tangent-space methods. More tangent-space
methods will be added in the future.

If in doubt, there is a very simple receipt for choosing the right
method: simply pick the one that looks best.

Cast shadows
~~~~~~~~~~~~

This setting determines whether the mesh casts shadows when lit by a
dynamic light source.

This can be useful for performance tuning, e.g. in order to not have
shadows casts for very small meshes that implement model details, or
generally for models that are very far away, or never seen in a context
where shadows play a role.

Statistics
~~~~~~~~~~

The last three numbers show the number of triangles, vertices and
weights in the mesh. They cannot be changed.

| 

.. |image0| image:: /images/modeleditor/meshes-list.png
   :class: mediaright
.. |image1| image:: /images/modeleditor/meshes-list-context-menu.png
   :class: mediaright
.. |image2| image:: /images/modeleditor/meshes-project-uv.png
   :class: mediacenter
.. |image3| image:: /images/modeleditor/mesh-inspector.png
   :class: mediaright
.. |image4| image:: /images/mapping/cawe/material_browser_2.png
   :class: medialeft
   :width: 200px
