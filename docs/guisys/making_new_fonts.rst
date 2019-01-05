.. _the_font_wizardcreating_new_fonts:

The Font Wizard: Creating new fonts
===================================

Preparing new fonts for rendering text within a GUI is very easy. The
key idea is to convert a font file (e.g. one of those installed with
your OS, or downloaded from the Internet) to a set of graphics files
that the Cafu GuiSys can use:

| |Open the Font Wizard| Start CaWE and select the related menu item to
  open the Font Wizard.

| |Step 1| Step 1: The welcome page.

|Step 2| Step 2: Click the “\ ``…``\ ” browse button to select the
desired font, or enter the filename directly into the related field.

**Microsoft Windows** users, please note that for some versions of
Windows, most notably **Windows 7**, Microsoft has chosen to
special-case the related “File Open” dialog so that the system folder
``C:\Windows\Fonts`` cannot be browsed. (The problem is neither related
to access rights nor is it specific to CaWE.)

In order to overcome the problem, i.e. if you want to use a system font
from ``C:\Windows\Fonts`` with the Font Wizard, we recommend to either

-  type the path and file name of the desired font manually into the
   input field, or
-  at the command prompt copy the desired font files from
   ``C:\Windows\Fonts`` into any other directory, then use the
   “\ ``…``\ ” browse button to select the related file.

| |Step 3| Step 3: Check the preview of the generated font texture.
  Optionally click the button to see a preview of all generated texture
  images.

| |Step 4| Step 4: Enter the name under which you would like to save the
  font. A new directory is created with the given name and all files
  related to the font are saved into this directory.

| |Step 5| Step 5: Use the indicated name in the GUI Editor or a GUI
  script to activate the new font (see below for details).

Creating fonts at the command prompt
------------------------------------

As an alternative to the above, fonts can also be created with the
``MakeFont`` program at the command prompt. Here is the sequence of
commands that creates the same font as above:

.. code:: doscon

   d:\Dev\Cafu> cd Fonts
   d:\Dev\Cafu\Fonts> mkdir Segoe
   d:\Dev\Cafu\Fonts> cd Segoe
   d:\Dev\Cafu\Fonts\Segoe>..\..\MakeFont.exe C:\Windows\Fonts\segoe.ttf -m=Segoe
   The Cafu Font Maker, version July 01 2009.
    
   Portions of this software are copyright (c) 2006 The FreeType Project
   (www.freetype.org). All rights reserved.

``MakeFont`` takes two parameters: the name of the font file whose font
you want to use with Cafu GUIs, and the base name of the font materials.
The base name is specified as ``-m=<base_name>``. Although ``MakeFont``
will also work if you omit the ``-m=…`` part, your fonts will not work
properly without it, so please make sure to include it. (The Cafu MatSys
needs this option in order to be able to render the fonts properly.)

Using the newly created font
----------------------------

|Activate the new font in the GUI Editor| Your font is now ready to be
used in a GUI. Its name always begins with ``Fonts/``, followed by the
previously chosen font name. In our example above, the font name is
``Fonts/Segoe``.

In the GUI Editor, enter the font name into the appropriate field of the
window that should use the font.

Alternatively, if you edit the GUI script manually, just add

.. code:: lua

       self:set("font", "Fonts/Segoe");

to the appropriate ``OnInit()`` function to use the new font.

.. |Open the Font Wizard| image:: /images/fontwizard/fontwizard_0.png
   :class: mediaright
.. |Step 1| image:: /images/fontwizard/fontwizard_1.png
   :class: mediaright
.. |Step 2| image:: /images/fontwizard/fontwizard_2.png
   :class: mediaright
.. |Step 3| image:: /images/fontwizard/fontwizard_3.png
   :class: mediaright
.. |Step 4| image:: /images/fontwizard/fontwizard_4.png
   :class: mediaright
.. |Step 5| image:: /images/fontwizard/fontwizard_5.png
   :class: mediaright
.. |Activate the new font in the GUI Editor| image:: /images/fontwizard/new_font_gui_editor.png
   :class: mediaright

