.. _usermanual_installation_installation:

Installation
============

.. _minimum_system_requirements:

Minimum System Requirements
---------------------------

Cafu has been designed to run on a very broad range of hardware.
Therefore, meeting or failing a certain set of minimum system
requirements is not necessarily a question of getting all-or-nothing.
Instead, the details of a systems hardware usually just scale the
performance and/or the quality that the Cafu engine can achieve on that
particular system – it even has automatic fall-back capabilities
built-in in order to handle old and very old hardware well.

Your computer system should meet the following *minimum requirements*
for Cafu to run (failing these requirements probably means that Cafu
does not run at all):

-  Intel i386 compatible CPU with 2 GHz
-  1 GB RAM (2 GB with Windows Vista or 7)
-  3D graphics board with OpenGL support and 256 MB graphics RAM
-  Windows (2000, XP, Vista, or 7) or Linux operating system
-  TCP/IP (network drivers)
-  On Windows: DirectX 7.0 or higher

Systems that do not meet these requirements may still work fine with
Cafu, but both the main processor and/or the graphics processor RAM may
easily be exceeded by loading larger maps with many textures, resulting
in very bad performance. You can use the Cafu Options dialog then to
reduce the texture quality and/or load smaller maps.

In addition to the above, here is a list of *recommended system
features* that allow Cafu to run reasonably well (in all cases, more is
better):

-  Intel i386 compatible CPU with 3 GHz
-  2 GB CPU RAM
-  contemporary 3D graphics board with OpenGL support, programmable GPU
   and 256 MB graphics (GPU) RAM

In order to use the advanced networking features of Cafu, the network
environment of your system must be appropriately configured.

In other words, if some of the modern commercial 3D computer games run
on your system, then this is a good indication that Cafu will work, too.

We are developing Cafu under Windows and Ubuntu Linux, and test it on as
many platforms as we can get access to. We have only little experience
with other Linux distributions, but have received only positive reports
up to now, regarding compatibility with all supported operating systems.

.. _installing_cafu:

Installing Cafu
---------------

In order to install Cafu, simply unzip the packed file that you
downloaded from the website. Please make sure that the directory
structure is preserved. This is the common extraction process for nearly
all contemporary decompression software. However, for some older
programs you need to explicitly specify a command line option or check a
checkbox. A new directory like ``Cafu-9.06`` will be created and all
files will be extracted into this directory.

.. _usermanual_installation_upgrading:

Upgrading
---------

In order to upgrade the engine from an older, existing installation,
please delete all the old files and directories and then re-install the
new version as stated above. We are sorry for any inconvenience caused
by this, but this is the best way to avoid many possible upgrade and
version conflict problems.

.. _usermanual_installation_de-installation:

De-Installation
---------------

Because the installation makes no changes to your system, it is
sufficient to just delete the ``Cafu-x.y`` folder for a completely clean
de-installation.
