.. _modeleditor_submodels_submodels:

Submodels
=========

Submodels are models whose skeleton is (partially) aligned to the
skeleton of their parent model.

The most prominent examples for submodels are weapon models that are
combined with player models: Each player model can be combined with any
weapon model, and the animation sequences of either combine naturally
with each other:

|image0|

It's typically the job of the game code to load and combine parent
models with their submodels, but the Model Editor implements this
feature as well so that you can inspect loaded submodels easily:

Press the “\ **+**\ ” button in the **Submodels** dialog in order to
load a new submodel.

In theory, you can load more than one submodel per parent model,
although that only makes sense if you load e.g. different weapon models
for the left and right hands of the player, or child monsters that are
carried in different locations in the parent monster, etc.

The “\ **-**\ ” button unloads the selected submodels again.

.. |image0| image:: /images/model-editor-4.png
   :class: mediacenter
   :width: 720px
