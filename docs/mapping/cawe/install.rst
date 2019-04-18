.. _installation_initial_configuration_and_de-installation:

Installation, Initial Configuration and De-Installation
=======================================================

.. _mapping_cawe_install_installation:

Installation
------------

The CaWE editor is included with the regular Cafu releases, and as such
requires no explicit installation: After unzipping the packed file,
simply change into the ``Cafu-x.y`` directory. Under Windows, then
double-click on ``RunCaWE.bat``. Under Linux, click ``CaWE-run`` in the
``Projects/Cafu/`` sub-directory.

First-Time Configuration
------------------------

This section is relevant to CaWE releases before February 2008 (version
8.02) only. If you have version 8.02 or newer, the initial setup is
fully automatic and you can skip this section safely. You may still
check out the
:ref:`Configure CaWE Options <the_configure_cawe_options_dialog>` dialog
anytime later.

CaWE requires some basic configuration for normal use, and thus asks you
for providing at least one game (or “MOD”) configuration when it is
first run:

#. |image0| After clicking on **OK**, CaWE will automatically show you
   the related configuration dialog.
#. |image1| Here, click on **Add New** for adding a new game
   configuration. You will then be asked for the desired name of the new
   config.
#. |image2| You can enter anything here, i.e. normally something that
   reminds you what MOD this config is related to, like “Cafu
   DeathMatch”.
#. Next you have to fill-in where the Game Data File is located. This is
   a file with ``.fgd`` suffix. One such file is already provided with
   the Cafu package: the ``DeathMatch.fgd`` file in the
   ``Projects/Cafu/Games/DeathMatch`` subdirectory. Click on the related
   **Browse…** button and locate the file.
#. |image3| Finally, fill-in the rest of the dialog appropriately such
   that it eventually looks like this. The paths will of course be
   different for you, depending on where you installed the Cafu package
   and its version. See the next step however for an alternative setup.
#. |image4| Alternatively to the previous step, you can also fill-in
   relative paths as shown in this image. This is the preferred method,
   because it works better with future versions of CaWE, makes upgrading
   to a newer version of Cafu and CaWE easier, solves some problems with
   map compilation some people were reporting, and will become the
   default with future releases of CaWE!
   Please note that under Linux, you have to use forward-slashes as the
   path separator (e.g. ``Games/DeathMatch``) and executable files must
   be referred to like e.g. ``./CaLight``.

..

   .. container:: no

      All settings in this dialog can be revised *later at any time* via
      the **File → Configure CaWE…** menu.
      Adding new or additional game configurations at that time may
      however require to restart CaWE for the settings to take effect.

.. _mapping_cawe_install_de-installation:

De-Installation
---------------

CaWE, as all other Cafu related software, has been designed to interfere
with your computer and operating system as little as possible. In
general, it is enough to delete the directory that the packed file was
unzipped to during installation, and all traces of CaWE are gone.

There is one exception though: When run, CaWE stores your configuration
settings in a newly created directory. This directory is located in the
“home” directory of the current user. The exact place on your harddisk
depends on your OS and even on its version, but CaWE states the full
path to it both in the **Configuration** dialog (**General** tab), and
in the **About** dialog box that is accessible via the **Help → About**
menu.

|image5| |image6|

.. |image0| image:: /images/mapping/cawe/firstrun_nogameconfigs.png
   :class: medialeft
.. |image1| image:: /images/mapping/cawe/firstrun_emptyconfigdialog.png
   :class: medialeft
   :width: 300px
.. |image2| image:: /images/mapping/cawe/firstrun_addnewgame.png
   :class: medialeft
.. |image3| image:: /images/mapping/cawe/firstrun_filledin.png
   :class: medialeft
.. |image4| image:: /images/mapping/cawe/firstrun_filledin_rel.png
   :class: medialeft
.. |image5| image:: /images/mapping/cawe/deinst_pathtocfg_configure.png
   :class: media
   :width: 300px
.. |image6| image:: /images/mapping/cawe/deinst_pathtocfg_about.png
   :class: media
   :width: 300px
