.. _the_model_editorintroduction:

The Model Editor: Introduction
==============================

The Cafu Model Editor answers the question:

=====================================
*“How do I get my models into Cafu?”*
=====================================

|image0| Thus, its purpose is to:

-  load and import model files,
-  support many file formats (see below for a list),
-  let you inspect the loaded model both graphically and structurally,
-  enable you to tweak and adapt the model (e.g. import additional
   animations, remove unused skeleton nodes, etc.)
-  set or adjust all Cafu-specific settings (such as animation
   properties, GUI panels, collision details, etc.),
-  assign and edit render materials,
-  save the resulting model in Cafu's own file format.

Summarized, the Cafu Model Editor makes your models ready for use in the
Map Editor and the Cafu Engine.

What the Model Editor is not
----------------------------

The Cafu Model Editor is *not*, despite its name, a general-purpose
editor that lets you shape meshes or create a model from scratch: There
are many commercial and free programs for this purpose already that we
don't intend to rival. In fact, model artists use one or more of these
programs early in the work pipeline to create and shape the model, then
use the Cafu Model Editor as described above.

Also, with the Model Editor we don't attempt to provide a graphical user
interface that saves you from editing script code:
:ref:`Many models files are scripts <model_files_explained>`, and
accessing some features is only possible by editing the script code in
these files. Our goal is to employ the Model Editor for everything that
is hard or impossible to accomplish with model scripts or otherwise.
Tasks that are best solved in script code are left in script code, for
which we provide thorough documentation in order to make them easy to
understand and master.

With these key ideas, we can achieve the most effective solutions for
all tasks about models.

Starting the Model Editor
-------------------------

|Start the Model Editor via the File-New or File-Open menu items.| Like
the other asset editors, the Model Editor is a part of CaWE. You start
it via menu items

-  File → Open… (Ctrl+O) *or*
-  File → New → New Model (Ctrl+Shift+N)

With both menu items, you're next presented the **File Open** dialog to
open a previously created model.

Check out the :ref:`Main Window <the_main_window>` and the
:ref:`How-Tos <modeleditor_howtos_how-tos>` for the next steps.

.. _modeleditor_introduction_supported_file_formats:

Supported File Formats
----------------------

At this time, the following file formats are supported:

========= =====================================
Extension Description
========= =====================================
``.cmdl`` Cafu Engine native model file format
``.3ds``  3D Studio 3DS
``.ase``  Ascii Scene Export, 3D Studio Max ASE
``.dxf``  Autodesk AutoCAD DXF
``.dae``  Collada DAE
``.fbx``  Autodesk FBX
``.lwo``  Lightwave Object LWO
``.md5``  Doom3 / Quake4 MD5
``.mdl``  Half-Life1 MDL
``.obj``  Alias OBJ
========= =====================================

.. |image0| image:: /images/modeleditor/teaser.png
   :class: mediaright
   :width: 256px
.. |Start the Model Editor via the File-New or File-Open menu items.| image:: /images/modeleditor/open_new.png
   :class: mediaright

