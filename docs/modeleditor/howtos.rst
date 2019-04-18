.. _modeleditor_howtos_how-tos:

How-Tos
=======

How do I ...
------------

.. _get_my_model_into_cafu:

... get my model into Cafu?
~~~~~~~~~~~~~~~~~~~~~~~~~~~

See `this video <https://youtu.be/Q3KJm-Ph_5o>`__ for details.

Additional notes:

-  When watching the video, change the playback quality to “720p HD” if
   possible.
-  There is no audio track in this video.
-  Use the :ref:`New Entity <the_new_entity_tool>` tool for placing the
   newly created ``.cmdl`` model into a map. Watch the `Placing a
   Model <http://www.cafu.de/flash/Placing_a_Model.htm>`__ tutorial for
   related information.
-  If you want to make adjustments later, both the ``.cmdl`` model as
   well as the material definitions can be edited as shown above also
   *after* the model has been placed into a map or world file. The
   changes will take effect when the program is next restarted.
-  In the video, why did we *close* the model window before editing the
   material definitions?
   When you save a model file, the Model Editor *overwrites* the
   ``_editor.cmat`` file as explained at
   :ref:`Model Files Explained <model_files_explained>`. In fact, we
   should never have hand-edited the ``_editor.cmat`` file, but do all
   edits in ``.cmat`` (without the ``_editor`` part) instead, but we
   thought it's too difficult to explain that in the video. 😉
   Additionally, the Model Editor had to re-read the new material
   definitions anyway, so even if our edits were made in the right file,
   we had to close and reopen the model.

... scale my model?
~~~~~~~~~~~~~~~~~~~

There are two main methods for scaling a model:

-  via the Skeleton / the Joint Inspector
-  via the Transform dialog

The :ref:`Joint Inspector <modeleditor_skeleton_skeleton>` can transform
individual joints of the model, but only for the *bind pose*. It is
mostly useful for static, non-animated models, but even then only if
there is a special requirement that the Transform dialog cannot handle.

The :ref:`Transform dialog <transformstranslate_rotate_and_scale>` is
much easier to use, as it transforms the entire model (the whole
skeleton): the transformation is applied to all currently selected
animation sequences. If no animation sequence is selected at all, the
“bind pose” is transformed.

... import animations from other models?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In some cases, importing animations is very easy: Use the “\ **+**\ ”
button in the :ref:`Animations <modeleditor_animations_animations>` pane
as demonstrated in the above video.

However, this works only if the animations are stored in separate files
that are compatible with the already loaded mesh. This is specifically
the case with ``md5anim``\ s for their related ``md5mesh`` file.

We will extend this functionality in the future. Until then and for all
other cases, see the next How-To about generically sharing and re-using
elements.

... share and re-use animations, skins etc. from other models?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Creating variants of a model by sharing or re-using elements of another
model is a straightforward idea, but generally not easy to implement:
For example, it's easy to share meshes and animations when the skeleton
is an exact match, but much harder to do otherwise.

The solution is to manually edit the ``cmdl`` model file, which is a Lua
script as documented at
:ref:`Model Files Explained <model_files_explained>`.

For example, copying and pasting the definition of an animation from one
player model into another (that has a compatible, matching skeleton),
tends to work very well.

It is also possible to use Lua's ``dofile()`` statement to include
models into each other, which we intent to augment in the future to load
globals from one model file into a table of another, as to keep the
global namespace of the current model clean. All variants of such file
inclusion however suffer the problem that it creates a dependency of one
file on another. This may be exactly what you want, or maybe not – where
in the latter case falling back to copy and paste is probably the better
solution.
