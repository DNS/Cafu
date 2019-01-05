.. _transformstranslate_rotate_and_scale:

Transforms: translate, rotate and scale
=======================================

|image0| The **Model Transform** dialog is used to translate, rotate or
scale the entire model.

It is opened

-  via menu item **Model → Transform… (Ctrl+T)**,
-  or the “Transform” button in the
   :ref:`application toolbar <menu_and_toolbar_reference>`.

Contrary to the :ref:`Joint Inspector <modeleditor_skeleton_skeleton>`,
the **Model Transform** dialog can *not* be used to transform individual
joints in a skeleton, a feature that is only useful for static,
non-animated models.

The virtue of the **Model Transform** dialog is that it affects the
entire model (the whole skeleton), and that the transformation is
applied to all currently selected animation sequences. If no animation
sequence is selected at all, the default pose (also called the “bind
pose”) is transformed.

Purpose
-------

The **Model Transform** dialog's main area of application is to adjust
newly imported models to the Cafu coordinate system:

An authoring program might have exported a model in a scale or default
orientation that does not agree well with the Cafu coordinate system, or
just is different from other models that you already have.

Use the **Model Transform** dialog to fix such and similar cases easily.

.. |image0| image:: /images/modeleditor/transform.png
   :class: mediaright

