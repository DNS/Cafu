.. _matsys_cmat_manual_overview:

Overview
========

First of all, here is a simple example for a material definition script.
It was taken from the ``Cafu-9.06/Games/DeathMatch/Materials/Kai.cmat``
file, which also contains several other, very similar material
definitions:

::

       Textures/Kai/3r_metpan01        // Material definitions start with the material name.
       {
           diffusemap  Textures/Kai/3r_metpan01_diff.png     // This line says which texture is used as diffuse-map.
           normalmap   Textures/Kai/3r_metpan01_norm.png
           specularmap Textures/Kai/3r_metpan01_spec.png
           lightmap    $lightmap
       }

Before we dig into the details about keywords and structure of such
material definitions, here are some general properties of cmat files:

-  cmat files are simple ASCII text files, containing material
   definition scripts. Their file name ends with ``.cmat``.
-  All statements in such files are *case-sensitive*. That means that
   ``text`` and ``Text`` are *not* the same. This is also true for
   filenames, like the ``Textures/Kai/3r_metpan01_diff.png`` filename
   above, because some operating systems like Linux have a
   case-sensitive file-system.
-  C++ style comments are allowed in material scripts:
   ``// This is a comment.``
-  The cmat files for MOD “MyMOD” are all stored in the
   ``Cafu-9.06/Games/MyMOD/Materials/`` directory and its
   subdirectories. This is necessary because the Cafu engine
   automatically scans this directory for material scripts whenever MOD
   “MyMOD” is run.
-  Tokens in cmat files are separated by white-space and these
   individual characters: ``( { [ ] } ) ,``
-  Quoted tokens are recognized. That is, everything between two
   ``"..."`` is considered as one statement, even if white-space or one
   of the above characters is inside it. That means, if for example you
   *really* want to have a material name like
   ``my(new and cool)material``, then you have to enclose it in
   quotation marks like this: ``"my(new and cool)material"`` in order to
   account for both the white-space and the brackets. Using quotation
   marks is not recommended, though! They're mostly useful if somebody
   created textures with weird file names like for example
   ``"{_SomeFile.bmp"``. Write ``MyNewAndCoolMaterial`` or
   ``My/New/And/Cool/Material`` or something similar for your material
   names instead.

Material definitions always start with the **material name**. In the
example above, that's ``Textures/Kai/3r_metpan01``. You can name
materials almost anything you like. If you want to use white-space,
commas or brackets in their name, you'll have to put the name into
quotation marks as mentioned above. However, it is important that the
name is *unique*. If the same material name appears again in any other
cmat file of the same MOD, the engine will use only the first occurance,
so the chances are 50:50 that your material wins over the other. Most
materials that come with Cafu have a filename that roughly resembles the
name of its texture image files. That is often a helpful hint for
conveniently working with the material, but by no means a requirement.

The **body** of the material definition is enclosed in a pair of
``{ … }``. In many cases, it will only contain a few texture map
specification statements like in the example above.
