.. _modeleditor_channels_channels:

Channels
========

Channels are used for grouping joints. They allow animations to play
only on a subset of the model joints, so that multiple animations can
play on different parts of the model at the same time.

For example, you can play a walking animation on the legs, an animation
for swinging the arms on the upper body, and an animation for moving the
eyes on the head.

Animation channels are defined and inspected in the Model Editor as
described below. The *use* of channels is up to the game's script or C++
code.

The channels list
-----------------

|image0| The **Channels** pane lists all animation channels in the
model.

For each channel,

-  its name and
-  the channel number

is shown.

A single click on a channel selects it, a double click opens the
**Channel Inspector** pane as well.

Pressing the **F2** key or a single-click on an already selected channel
allows you to rename the channel in place.

| The “\ **+**\ ” button creates a new channel and adds it to the list.
  The “\ **-**\ ” button deletes the currently selected channels.

Context menu
~~~~~~~~~~~~

|image1| An RMB click in the **Channels** pane opens the context menu:

Inspect/Edit
   opens the **Channel Inspector** pane.
Rename
   allows to rename the animation channel.
Add/create new
   , like the “\ **+**\ ” button, creates a new channel and adds it to
   the list.

| 

The channel inspector
---------------------

|image2| The **Channel Inspector** pane shows the details of the
currently selected channel.

Name
~~~~

Shows the name of the currently selected animation channel. The name can
be edited in order to rename the channel.

Joints
~~~~~~

Select for each individual joint whether it will be a member of the
channel.

| When the game code plays an animation on this channel, the animation
  will only affect joints that are a member of the channel.

.. |image0| image:: /images/modeleditor/channels-list.png
   :class: mediaright
.. |image1| image:: /images/modeleditor/channels-list-context-menu.png
   :class: mediaright
.. |image2| image:: /images/modeleditor/channel-inspector.png
   :class: mediaright

