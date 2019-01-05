.. _level-of-detail_models:

Level-of-Detail Models
======================

Level-of-detail models are a kind of “meta” models: They consist of two
or more “normal” models, but only one of the referenced models is shown
at any time, depending on the distance to the viewer.

|image0| The key idea is to render a detailed model only when the player
(the camera) is close to it. When the camera gets farther away, a less
detailed model is rendered. And when the camera gets even farther away,
an even less detailed model is used.

This technique, while not without its downsides, is a common approach
for reducing the computational effort required for rendering a model
relative to the amount of detail that the player can perceive of distant
objects.

Loading LoD models
------------------

|image1| Level-of-detail models have a file suffix of ``.dlod``, and you
open them in the Model Editor like ``.cmdl`` models or any other model
files.

The **Level-of-Detail Models** dialog will show the list of “real”
models that the LoD model is referring to, and up to which viewer
distance each model is used.

Inspecting and saving LoD models
--------------------------------

In the list and inspector panes, the Model Editor will only show the
elements of the *first* (closest, most detailed) referenced model, even
if the camera is moved far enough away so that in the 3D view one of the
lower detailed models is rendered.

If you attempt to save a ``.dlod`` model, the saved file will be the
first referenced model as well. That is, it will be treated like an
individual, independent model, not as the whole set of models in the
``.dlod`` file.

As a result, you should load LoD models only for inspecting them in the
3D view. In order to edit the component models the ``.dlod`` file is
referring to, you should load them directly by their true name, each one
separately from the other.

Creating LoD models
-------------------

``.dlod`` files cannot be created in the Model Editor. They are simple
text files that you have to create yourself, using a programmers text
editor.

Here is an example, taken from
``Games/DeathMatch/Models/Static/TonneTest.dlod``:

.. code:: bash

   TonneTest_LoD1.ase  10000
   TonneTest_LoD2.ase  20000
   TonneTest_LoD3.ase  40000
   TonneTest_LoD4.ase  80000
   TonneTest_LoD5.ase

Each line lists a concrete model and the distance up to which the model
is rendered.

-  The distance for the last, farthest model is optional. If given, it
   is ignored: the last model is used up to infinity.
-  The file names are relative to the path of the parent (``.dlod``)
   file. Valid examples include ``a/x.cmdl`` and ``..\y.ase``, whereas
   ``c:\z.mdl`` is invalid.
-  The numbers are given in Cafu world units (which are currently *not*
   the same as CaWE world units: they are only 1/25.4 of the Cafu
   units!). In Cafu worlds, each unit is 1 millimeter. 1000 units are
   one meter or 3.2808 feet.
-  In many ways, the first model in the list is “responsible” for the
   collective: The Cafu Engine gets the bounding boxes, number of
   animation sequences etc. all from the first model. Ideally though,
   all models should have the same BB, same (logical) animation
   sequences, etc.

An example
----------

The image below shows three models, a simple barrel. The three models
are independent of each other, and each is stored in a separate model
file. They all implement the same logical object though, each at a
different level-of-detail:

|image2|

Combine models like these in ``.dlod`` files as shown above – that's
all! Your LoD model is now immediately available for use in the Map
Editor and the Cafu Engine.

.. |image0| image:: /images/modeleditor/lod-models.png
   :class: mediaright
.. |image1| image:: /images/modeleditor/open_new.png
   :class: mediaright
.. |image2| image:: /images/modelling/lod-tut1.gif
   :class: mediacenter

