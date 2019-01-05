.. _using_the_autodesk_fbx_sdk:

Using the Autodesk FBX SDK
==========================

|images.autodesk.com_adsk_images_autodesk_fbx_badge_150x265.jpg| When
you build Cafu from source, support for several file formats is provided
by Cafu's own source code so that the Model Editor can import static and
animated models in the most important file formats.

Optionally, it is possible to have the build scripts account for and use
the Autodesk FBX SDK in order enable the Model Editor to import model
files in these additional file formats as well:

-  Autodesk FBX (``.fbx``), version 6.0 to 7.2
-  Autodesk AutoCAD DXF (``.dxf``), version 13 and earlier
-  Collada DAE (``.dae``), version 1.5 and earlier
-  3D Studio 3DS (``.3ds``), all versions
-  Alias OBJ (``.obj``), all versions

The FBX SDK is a separate download from Autodesk that we cannot
distribute together with Cafu: The combined file size for all platforms
is *massive*, Autodesk prefers that software vendors don't redistribute
their SDK and in fact requires prior written permission, and we're happy
to maintain a certain degree of independence, for example when we want
to use Cafu with compilers or on platforms where the FBX SDK is not
available.

Therefore, we made the use of the Autodesk FBX SDK very easy, but
entirely optional. If you don't use it, all that you're missing is that
the Model Editor cannot load the above mentioned file formats. (When we
make official binary releases of Cafu, we always ship with FBX support
enabled.)

Follow these steps in order to activate support for FBX:

#. Download the Autodesk FBX SDK from http://www.autodesk.com/fbx

   -  The download requires a one-time registration, but it's free.
   -  You just need the FBX SDK, *not* the FBX Extensions SDK.
   -  At this time, we use version 2017.1 of the FBX SDK, but any later
      version should work as well.

#. Run the installer to extract the SDK contents into a convenient
   location. Their installer is exceptionally nice, it neither modifies
   the Start Menu nor the Windows Registry, but just extracts the files
   into a directory.

   -  Assuming that your top level Cafu source code directory is
      ``Cafu/``, you can directly install the FBX SDK into
      ``Cafu/ExtLibs/fbx/``.
      Make sure that the installer doesn't automatically append another
      directory for the SDK version, and if it asks if you would like to
      keep a copy of the old FBX installation, answer “No” (otherwise it
      first renames ``Cafu/ExtLibs/fbx/`` to ``Cafu/ExtLibs/fbx.old/``,
      creating an (easily fixed) problem if you use Git).
   -  Alternatively, just install the FBX SDK into a convenient location
      first (e.g. into a temporary directory or the default directory
      suggested by the installer), then copy or move the contents into
      ``Cafu/ExtLibs/fbx/``.
      (Under Linux, we recommend to *not* install into the suggested
      ``/usr`` directory. Better install into a temporary directory in
      your home folder, no root privileges required.)

#. As a result from the previous step, make sure that you now have
   directories ``examples``, ``include`` and ``lib`` (and possibly
   others) immediately below ``Cafu/ExtLibs/fbx/``.
#. (Re-)Run SCons as described at
   :ref:`Getting Started with the Cafu Source Code <getting_started_with_the_cafu_source_code>`
   in order to automatically recompile with FBX support enabled.

.. |images.autodesk.com_adsk_images_autodesk_fbx_badge_150x265.jpg| image:: /images/autodesk_fbx_badge_150x265.jpg
   :class: mediaright

