.. _getting_started_with_the_cafu_source_code:

Getting Started with the Cafu Source Code
=========================================

This text explains how you get and compile the Cafu source code.

Most steps that are explained below are only necessary once, when you
get and compile the Cafu source code for the first time.

Getting the Source Code
-----------------------

This section explains how you obtain the Cafu source code by checking it
out from its `Git repository <https://bitbucket.org/cafu/cafu>`__.

Git is a version control system that allows the Cafu developers to work
on their copy of the source code independently of each other, and to
distribute the latest version of the source code to other developers at
any time. Moreover, many developers prefer to manage their project
independently in their own version control system, but also wish to
integrate the latest changes (e.g. bug-fixes and new features) from the
official Cafu repository into theirs from time to time: Git makes the
synchronization of the projects code with the original Cafu source code
as convenient and automatic as possible. If you don't know Git yet, have
a look at the `Git website <http://git-scm.com>`__ and the `Git
documentation <http://git-scm.com/documentation>`__. The “Pro Git” book
is very well written and available for free in several languages.

For Windows and many other systems, you can get Git from the `Git
downloads <http://git-scm.com/downloads>`__ page. Popular Git clients
with a graphical user interface are available at the Git downloads page
as well.

With most Linux distributions, installing Git via the systems package
manager is usually preferred. For example, under Ubuntu:

.. code:: bash

   > sudo apt-get install git gitk

Then check out the source code at the command-line under Windows or
Linux with this command:

.. code:: bash

   > git clone --recursive https://bitbucket.org/cafu/cafu.git Cafu

Note the ``--recursive`` option in the command above: it makes sure that
submodules are automatically checked out as well. As we keep the texture
images for the DeathMatch example game in a submodule, it is convenient
to have them checked out along with the main repository. Be prepared
though that this adds approximately 180 MiB to the download volume.

Our Git project page is https://bitbucket.org/cafu/cafu, where you can
browse the repository online, create forks, post pull requests, and find
additional help about Git.

Python and SCons
----------------

Cafu uses `SCons <http://www.scons.org/>`__ as its software build system
on all supported platforms. SCons is a modern replacement for ``make``
and ``Makefiles``. SCons requires the `Python scripting
language <http://www.python.org>`__ to be installed, so that you have to
get and install both Python and SCons on your computer.

Windows
~~~~~~~

Under Windows,

-  first get and install Python **2.7** (*not* one of the newer, but
   backwards-incompatible 3.x editions!) from
   http://www.python.org/download/,
-  then get and install SCons **2.3** (or any later version) from
   http://www.scons.org/download.php.

If in doubt, pick the Windows installers for both Python 2.7 and SCons
2.3: they are easy to use, lightweight, and automatically setup the
proper environment variables.

Important notes:

-  Pick the **32-bit** edition of Python even on 64-bit systems!
   (Unfortunately, SCons does not yet work with the 64-bit builds of
   Python.) That is,
   `python-2.7.12.msi <http://www.python.org/ftp/python/2.7.12/python-2.7.12.msi>`__
   is the right file for all Windows systems.
-  On Windows 7, 8 and 10, run the SCons installer via right-click, then
   select **“Run as administrator”**.

That is normally all, but you may wish to check if the Python installer
added Python's ``Scripts`` directory to the ``PATH`` environment
variable of your system. For example, if Python was installed in
``C:\Python27``, then ``PATH`` should contain both ``C:\Python27`` and
``C:\Python27\Scripts``. Otherwise, you have to add the proper
directories manually: See http://support.microsoft.com/kb/310519 and
http://vlaurie.com/computers2/Articles/environment.htm for additional
information.

In order to verify that everything is working correctly, open a new
command prompt and enter ``scons -v``:

::

   > scons -v
   SCons by Steven Knight et al.:
           script: v2.3.0, 2013/03/03 09:48:35, by garyo on reepicheep
           engine: v2.3.0, 2013/03/03 09:48:35, by garyo on reepicheep
           engine path: ['C:\\Python27\\Scripts\\..\\Lib\\site-packages\\scons-2.3.0\\SCons']
   Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 The SCons Foundation

The report of the SCons version indicates that both Python and SCons are
ready for use. Please make sure that you use SCons version **2.3 or
newer**, older versions don't work!

Linux
~~~~~

Under Linux, just use the systems package manager in order to install
SCons. The package manager will automatically install SCons and all
software that SCons depends on, such as Python. For example, under
Ubuntu:

.. code:: bash

   > sudo apt-get install scons

Linux Packages
--------------

Additional software packages must be installed on a Linux system before
the Cafu source code can be compiled.

On a fresh, newly installed Debian or Ubuntu system, you'll need the
following packages. They can be installed at the command prompt with the
``apt-get install`` command, or via the easy to use “Synaptic Package
Manager”. Similar packages and package managers also exist on RedHat
Linux, SuSE, etc., where you can proceed analogously.

-  A graphics driver with 3D hardware acceleration (the right driver is
   usually auto-detected and installed by the “Driver Manager”).
-  **build-essential** – The compiler and basic tools required to
   compile C and C++ programs.
-  **libgtk2.0-dev** – Developer files for GTK 2.0, needed for building
   wxGTK.
-  **libgl1-mesa-dev** and **libglu1-mesa-dev** – OpenGL developer
   files, needed for building wxGTK and the Cafu rendering subsystem.
-  **libxxf86vm-dev** – An interface to the XFree86-VidModeExtension
   extension.
-  **libasound2-dev** – ALSA developer files, needed for building
   OpenAL-Soft.

Make sure that when you're done, all packages from the list and their
dependencies are installed on your Linux system.

Compiler Setup
--------------

Cafu can be compiled as 32-bit and 64-bit software on all platforms. The
following compilers are supported:

=============== ========================================================================================================================================================================
Windows         Remarks
=============== ========================================================================================================================================================================
Visual C++ 2015 You can use the free Express Editions with Cafu. They are available at http://www.microsoft.com/express/vc/ and automatically install the related Microsoft Windows SDK.
Visual C++ 2013
Visual C++ 2012
Visual C++ 2010
\              
Linux           Remarks
GCC 4.4+        The GNU Compiler Collection with its C++ front-end, version 4.4 or any newer version.
=============== ========================================================================================================================================================================

The Cafu-specific compiler setup is fully automatic: No action is
required in this step, and you can directly proceed to the next.

Optionally, if you want to change the default settings now or later,
here is an overview of how it works:

#. When SCons is run in the next step, it first determines if file
   ``Cafu/CompilerSetup.py`` already exists. If it doesn't (e.g. when
   you do this for the first time), it automatically creates the file as
   a copy of ``Cafu/CompilerSetup.py.tmpl``.
#. Alternatively, you can also copy ``Cafu/CompilerSetup.py.tmpl`` to
   ``Cafu/CompilerSetup.py`` manually. This is only required once, and
   only if you want to edit ``Cafu/CompilerSetup.py`` before SCons is
   run for the first time.
#. You can edit ``Cafu/CompilerSetup.py`` in order to set the compiler
   and tools that are used to build Cafu, and to set the target
   architecture (such as ``x86`` or ``x86_64``) that it is build for.
   Each setting is well documented, so you should have no problems to
   make the desired changes.
   The default settings automatically determine the latest installed
   compiler and the current architecture, so in most cases the file
   works out-of-the-box and you have to change nothing at all. Also if
   you have doubts about the right settings, just continue and use the
   file as-is.

If you do this for the first time, we recommend that you don't bother
with ``Cafu/CompilerSetup.py`` at all. Just let the automatism determine
the settings and come back later if desired or required.

64-bit Windows with Visual C++ 2010 or 2012 Express Editions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On 64-bit Windows systems, SCons tries to find a 64-bit compiler, but
Visual C++ 2010 Express Edition comes with a 32-bit compiler only. To
tell SCons to use the 32-bit compiler instead, please edit file
``Cafu/CompilerSetup.py`` as described
`here <http://forum.cafu.de/viewtopic.php?p=4360#p4360>`__.

Even though the Visual C++ 2012 Express Edition comes with a 64-bit
compiler, at this time the `same
edits <http://forum.cafu.de/viewtopic.php?p=4360#p4360>`__ must be done
for it as well.

Compiling the Source Code
-------------------------

You are now ready compile the Cafu source code. At the command prompt,
change into the top-level ``Cafu`` directory (where also the files
``CompilerSetup.py(.tmpl)`` and ``SConstruct`` are), then start the
compilation with:

::

   > scons

When you do this for the first time, be prepared that it will take a
while: Everything is being built from scratch, once for the debug and
once for the release build. When you repeat the same command in the
future, e.g. after source code changes or updates, only the minimum set
of files will be rebuilt and the entire process will complete *much*
faster.

Under Linux, if you have a multi-core system, you can speed this up by
having several build jobs run in parallel:

::

   > scons -j N

where N is the maximum number of jobs to start simultaneously. For
example, try ``-j 4`` on a quad-core machine.

Adding Binary Assets
--------------------

You may use the time while the build process is running to download some
supplemental files. These files are not strictly required, but will help
you with trying out the newly compiled programs as described below.

We recommend that you download and extract the following files:

======================================================== ==================================
Download                                                 Extract into
======================================================== ==================================
`Textures.zip <http://www.cafu.de/files/Textures.zip>`__ ``Cafu/Games/DeathMatch/Textures``
`Worlds.zip <http://www.cafu.de/files/Worlds.zip>`__     ``Cafu/Games/DeathMatch/Worlds``
======================================================== ==================================

Note that although these files are zip archives, they are for use on all
platforms (Windows and Linux). With these files, you can immediately run
the Cafu engine and see some example worlds when the compilation has
finished.

Running the new Programs
------------------------

When the SCons script has finished, all Cafu programs have been built in
several variants:

-  The ``debug`` variant contains additional program information and no
   code optimizations, and is often used during development and for
   finding and fixing bugs.
-  The ``release`` variant has program optimizations enabled in order to
   maximize the execution speed and contains no debug information. This
   variant is usually shipped to end users.
-  The ``profile`` variant is not built by default. It is similar to the
   ``release`` variant, but also has debug information in order to make
   it suitable for use with a performance profiler.

As all variants can be built for each compiler on each platform, the
build process organizes the binaries in a directory hierarchy of the
form

::

    build/$platform/$compiler/$variant 

That is, if your compiler is Visual C++ 2015 (abbreviated as ``vc14``)
and your platform is Windows (abbreviated as ``win32``), and you want to
run the release variant of the new programs, then the related binaries
are relative to the path

::

    build/win32/vc14/release/ 

Another important consideration is that all programs assume the
top-level ``Cafu`` directory as their working directory.

Combining these facts provides you with several options on how you can
run the new programs:

#. Copy the programs of the platform, compiler and build variant of your
   choice from ``Cafu/build/$platform/$compiler/$variant/…`` directly
   into the ``Cafu`` directory. This is what we do for the prepackaged
   binary releases, as end-users can then simply and directly
   double-click each executable in the Windows Explorer.
#. Create a Windows link (``.lnk``) or batch (``.bat``) file in the
   proper working directory that calls the desired program in its
   original location.
#. For developers, the most flexible and recommended approach is to run
   the program directly from the command prompt: First change the
   current working directory to ``Cafu``, then type the relative path to
   the desired program (`TAB
   completion <https://en.wikipedia.org/wiki/Tab_completion>`__ – also
   on Windows – proves to be a great help here!).
   For example, running the debug version of CaWE under Windows looks
   like this (note that the example also shows the current working
   directory at the left, the actual command follows the ``>``
   character):
   ::

       D:\Cafu> build\win32\vc14\debug\CaWE\CaWE.exe 

   Running the release version of Cafu under Linux looks like this:

   ::

       ~/Cafu> ./build/linux2/g++/release/Ca3DE/Cafu 

If you have downloaded the binary assets as instructed above, you can
now run your self-compiled Cafu engine e.g. at
``build\win32\vc14\release\Ca3DE\Cafu.exe`` and try one of the included
demo levels! 😃

What next?
----------

Congratulations! When you get here, you have managed to successfully
compile the Cafu source code and run the resulting binaries. You may now
wish to familiarize yourself more completely with the world of Cafu.

One way to do this is to approach Cafu from a map designers perspective,
for example by exploring the Cafu World Editor CaWE. CaWE is not only a
map editor, it also contains the GUI editor, a font conversion tool, a
materials viewer and a model editor. CaWE can also act as a bridge
towards scripting: map scripting, GUI scripting, materials scripting and
vegetation scripting are powerful features in Cafu, and it doesn't stop
there!

Programmers should definitively have a look at the other sections in
this chapter
(:ref:`At the Core: The Cafu Source Code <at_the_corethe_cafu_source_code>`),
such as
:ref:`Starting your own Game <starting_your_own_game>`. If instead you
prefer to dig into the source code right away – just pick your favorite
piece of code and start reading and hacking.

Consider :ref:`Using the Autodesk FBX SDK <using_the_autodesk_fbx_sdk>`
in order to add support for several additional file formats in the Model
Editor.

Or you just browse the files and folders in the ``Cafu`` directory, and
start with whatever you feel most attracted to.

In any case, if you have questions or comments, or if you need help,
post a message at the `support forums <http://forum.cafu.de>`__ at any
time!
