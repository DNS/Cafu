.. _model_files_explained:

Model Files Explained
=====================

Each model comprises several files on disk, such as the model file,
material definition files, and texture images.

When you save a model in the Model Editor, the editor creates or
re-creates some of these files, updates others, and leaves alone the
rest. The result is normally exactly what you want and expect, but
sometimes you may wish to hand-tune some details (such as the material
definitions) without and independently of the Model Editor. In such
cases, it is very helpful to understand the files that belong to a
model.

This section explains the files that together form a model and how they
relate to each other.

One directory per model
-----------------------

Although not a technical requirement and not enforced by the Cafu model
code, it is highly recommended to save each model in a directory of its
own. This

-  explicitly groups all files that logically form and belong to the
   model, and
-  makes packaging a complete model in a ``my_model.zip`` archive
   possible, so that the model can easily and safely be distributed,
   shipped and handled.

The name of the directory should match the file name of the model. That
is, if your model's name is ``Trinity.cmdl``, it should be stored in a
directory with the same name ``Trinity/`` (or in a zip archive with the
same base name ``Trinity.zip``).

Note that when you are saving a new model that does not yet have a
separate directory, you can use the “New Folder” button (or right-click
context menu) of the “Save” dialog to create such new directories as
required.

Example:

.. code:: doscon

   d:\Dev\Cafu\Games\DeathMatch\Models\Players> dir Trinity
   ChromeBuckle_diff.png
   ChromeGlass_diff.png
   Pants_diff.png
   Skin_diff.png
   Trinity.cmat
   Trinity.cmdl
   Trinity.cmdl_bak
   Trinity_editor.cmat

.. _cmdl_model_files:

cmdl model files
----------------

The ``cmdl`` file is the main file of the model: It contains the
definitions for the skeleton, the meshes and the animation sequences of
the model.

The Cafu Engine and the Model Editor load ``cmdl`` files as
`Lua <http://www.lua.org/>`__ scripts, and as such they can be inspected
or edited in a text editor if desired.

Besides the main ``cmdl`` file, the Model Editor also creates a
``cmdl_bak`` backup file that contains the contents of the ``cmdl`` file
before it was last saved. (If you ship your model as a ``zip`` archive,
the ``cmdl_bak`` file is typically redundant and can be omitted from the
archive for space efficiency.)

Example:

.. code:: doscon

   d:\Dev\Cafu\Games\DeathMatch\Models\Players> dir Trinity\*.cmdl*
   Trinity.cmdl
   Trinity.cmdl_bak

.. _cmat_material_definition_files:

cmat material definition files
------------------------------

The ``cmat`` files contain the material definitions for the meshes of
this model.

The main ``cmat`` file must have the same base name as the model
(“Trinity”), so that the full name is for example ``Trinity.cmat``. This
file is for your hand-written material script code, if any, and is never
touched or overwritten by the Model Editor (with one exception, see
below).

The Model Editor also writes a secondary ``cmat`` file whose name ends
with ``_editor.cmat``. This file is written anew each time the model is
saved, and contains material definitions whose script code was not
hand-crafted, but who were created or edited in the Model Editor.

Example:

.. code:: doscon

   d:\Dev\Cafu\Games\DeathMatch\Models\Players> dir Trinity\*.cmat
   Trinity.cmat
   Trinity_editor.cmat

The two ``cmat`` files are linked as follows: When the Cafu model code
loads the materials of a model, it opens the main ``cmat`` file
(``Trinity.cmat``). This file contains a statement like

.. code:: lua

   dofile("Trinity_editor.cmat");

in order to include and process the secondary ``_editor.cmat`` along
with the main file.

The only exception when the Model Editor touches the main
``Trinity.cmat`` file is when the file does not yet exist, or doesn't
contain the ``dofile()`` reference to the editor file. In this case, the
``_editor.cmat`` would not be loaded at all, and thus the Model Editor
inserts the ``dofile()`` line into the main ``cmat`` file.

In summary, the goal of keeping two separate ``cmat`` files that are
linked as described above is to keep your hand-crafted material script
code and the Model Editor edited material definitions cleanly separated,
without any danger of one overwriting the other.

Texture images
--------------

The texture images are referenced from the material definition scripts.
See the documentation about `the Cafu Material
System </start#the_material_system>`__ for more details.

Example:

.. code:: doscon

   d:\Dev\Cafu\Games\DeathMatch\Models\Players> dir Trinity\*.png Trinity\*.jpg
   ChromeBuckle_diff.png
   ChromeGlass_diff.png
   Pants_diff.png
   Skin_diff.png
