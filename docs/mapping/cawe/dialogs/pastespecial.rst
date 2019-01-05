.. _the_paste_special_dialog:

The Paste Special Dialog
========================

This dialog is opened by clicking on **Paste Special…** in
:ref:`The Edit Menu <mapping_cawe_menureference_edit_the_edit_menu>`.
This option is only available if an object has been copied to the
clipboard before.

Paste Special allows you to paste more than one copy of a selection into
the map. You can for example paste a row of the same object into the
map, whereat the object is pasted multiple times and each copy is
shifted in position to form a row with the other objects.

|image0|

Number of copies to paste
   The number of copies from the objects in the clipboard that will be
   pasted.
Start at center of original
   If this is activated, pasting starts at the center of the original
   object/s. Otherwise it starts at the center of the last two selected
   2D views.
Group copies
   Objects are combined into a group after pasting them.
Translation (per copy)
   Sets the offset in x/y/z direction of each pasted object. This offset
   is accumulative, so the offset of an object is its own offset and the
   sum of the offsets off all previous objects.
Rotation (per copy)
   Sets the rotation offset of each pasted object. This offset is
   accumulative, so the offset of an object is its own offset and the
   sum of the offsets off all previous objects.

.. |image0| image:: /images/mapping/cawe/dialogs/dialog_pastespecial.png
   :class: medialeft

