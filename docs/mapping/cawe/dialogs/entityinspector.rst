.. _the_entity_inspector_dialog:

The Entity Inspector Dialog
===========================

This dialog presents detailed information about your map and its
entities. It shows you the hierarchically structured list of entities
that your map is composed of, allows you to inspect the properties of
each entity, and provides a built-in source code editor for the map
script.

The dialog is opened by selecting the **Entity Inspector** item from the
:ref:`Edit <mapping_cawe_menureference_edit_the_edit_menu>` menu.

The following text describes the three “tabs” that implement the main
features of the dialog.

Entity Report
-------------

|image0|

The Entity Report keeps a list of all entities that exist in the map. It
is used to easily manage, find and select individual entities, and thus
provides a convenient alternative to locating entities in your map
manually.

The entities are listed by the name of their entity class and their
instance name, if one has given, e.g.
“\ ``static_detail_model (TreasureChest)``\ ”. Clicking on a list entry
selects the related entity in the 2D and 3D views of the map as well.
You can select multiple entities at the same time by holding the
``CTRL`` or ``SHIFT`` key while clicking on an entry: this works just
like in Windows Explorer. The presented list is sorted in alphabetical
order and can be filtered by several options as described below.

Go to
   Centers the 2D views on the selected entities.
Delete
   Deletes the selected entities (both from the list and from the map).

Filter
   The controls in the filter box determine which entities are included
   in the list above.

   Brush Entities
      When checked, the brush-based entities are included in the list.
   Point Entities
      When checked, the point-based entities are included in the list.
   Include hidden Entities
      When checked, entities are included even though they are normally
      hidden (as part of a VisGroup) in the 2D and 3D views.
   By Key/Value
      You can reduce the list of entities to those whose properties
      match the given key/value pattern.
   Exact
      Toggles if the key/value pattern has to be an exact match, or if
      partial match counts as well.
   By Entity Class
      If checked, only entities of the specified entity class are shown.

Properties
----------

|image1|

The Properties tab is one of the most important dialogs in CaWE, because
it allows you to inspect, set and modify all of the advanced attributes
of the currently selected entity.

The properties are presented in tabular form as pairs of *keys* (on the
left) and their related *values* (on the right): The *set of keys* that
are available for an entity is defined by the entity's class. It can
therefore not be changed directly within CaWE. Adding, removing or
renaming keys is only possible via the game's ``EntityClassDefs.lua``
script, which contains the definitions of all entity classes for a game.
Updating this script possibly affects all maps of a game, and is
consequently normally never done for an individual map, but in the early
planning stages of a MOD.

Changing the *values* however is the daily bread and butter of a mapper.
Therefore, doing so is not only easy, but it has also been made as
convenient as possible. In most cases, you just click into the desired
value field and enter a new value directly. For each variable (that is,
a key/value pair), also the variables *type* is known to CaWE, which is
uses to assist you entering correct values:

-  keys that only accept integer values do not accept commas, periods or
   letters,
-  keys that accept boolean values or a choice from an enumeration act
   accordingly,
-  keys that expect colors will provide you a color chooser and an
   automatic preview,
-  keys whose values are multi-line strings come with an appropriate
   editor,
-  keys that expect file names (e.g. for sounds, models, etc.) will
   provide you with a three-dots “…” browse button (which is *very
   helpful* **(!)**),
-  and so on.

We're currently working on improving this feature further, e.g. with
“Play” buttons for listening to sound files, a preview facility also for
model files, etc.

Note that at the top of the dialog, there is a summary about the number
of selected entities and their classes. At the bottom of the dialog, a
short help text for the currently selected key is provided. Help is also
available in form of tool-tips for the key the mouse pointer is
currently over.

Classname
~~~~~~~~~

Changing the class of an entity is a special action, because it
corresponds to deleting the entity and re-instantiating it under a
different class but with the same properties. Nonetheless, you can use
the special ``classname`` property to change an entity's class like any
other property: You're provided with a choice box that contains all
possible new classes for that entity, and just making the selection
reincarnates the entity as an object of the new class.

Please note that currently there is a restriction that brush-based
(solid) entities can only be converted into entity's of classes that are
brush-based, too. Similarly, entities that are point-based can only be
converted into entity's that are point-based, too.

Undefined Properties
~~~~~~~~~~~~~~~~~~~~

Sometimes entities come with key/value pairs that have no corresponding
definition in their entity class. Such key/value pairs can occur
occasionally e.g. when an entity was converted to a different class (see
below), an entity class got a variable definition removed after a map
has already been made with it, or when an entity has been imported from
an entirely different game. The entity might then bring in a key named
e.g. ``light_pulse_frq`` that is utterly unknown in the context of the
current games entity definitions.

Another but similar case occurs when the entity's class name is unknown
among all entity classes of the current game. This typically happens
when you try to import for example an entity of class “stuffed_animal”
of a children's game into a game about aliens or robots, or in any other
case when the current game just knows nothing about the imported entity
class.

In these cases, there is no corresponding definition for such keys in
the game definition. CaWE therefore collects such occurrences separately
and keeps them under the special category “Undefined Properties”.

Because undefined properties are not dealt with by the game in any way,
you're free to change and modify them at will. You can even right-click
them to open a context menu that allows you to rename them, to add new
ones or to delete them.

In most practical cases, you'll want to get rid of undefined keys
though, which can be achieved by renaming them to something meaningful,
by deleting them, by changing the entire entity class to something else,
or a combination thereof.

Background Colors
~~~~~~~~~~~~~~~~~

The property grid employs several background colors in order to indicate
certain conditions of the values:

-  White background indicates that this is just a normal value.
-  Light blue background also indicates a normal value that also happens
   to be the entity class's default value for this variable.
-  Light orange background is used whenever multiple entities are
   selected that have *different* values for the *same* key. You can
   still enter a new value for such keys: The key of each selected
   entity will then be set to the same new value and the background
   color will change back to white accordingly.
-  Light red background is currently used at only one occasion: When you
   set a ``model`` key and have not yet specified a value for a
   ``collisionModel``, the ``collisionModel`` variable is temporarily
   highlighted with this color in order to remind you to also set a
   collision model.

Working with multiple entities
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Properties tab works also very well when *more* than a single entity
is selected, or in fact, any number of them is selected at the same
time. In order to present you the properties of multiple entities, CaWE
“overlays” them for you, so that even great amounts of simultaneously
selected entities remain easy to understand and manage.

In order to achieve this, CaWE makes sure that properties that are
candidates for layering, but otherwise incompatible with each other for
any reason (e.g. because the property is known only to some but not all
of the entities, or defined multiply with different types), are treated
specially as “mixed” properties. A property that is marked as “mixed”
has attributes in one entity that are non-existent or different in
another, which generally makes them incompatible to each other.

Such mixed properties are therefore listed in another separate category,
and they are protected from editing (“read-only”). This makes sure that
you can see such affected properties, but not inadvertently get
nonsensical values written into lots of entities. Note that this is not
a restriction, because it's just another way of saying: *“Stop! If you
assign a value to this property, there is at least one entity affected
for which doing this makes no sense at all.”*

If you still want to edit a property that is listed as “mixed”, there is
an easy way to overcome the problem: Just select fewer entities until
the entity that was the reason for the property being listed as “mixed”
is gone. When no conflicts remain, the property gets re-listed among the
normal properties, and you can edit it freely.

Map Script
----------

In the Map Script tab, you can view and edit the source code of the maps
current script.

**(FIXME!)** This feature is not yet implemented – I'm very sorry. Both
the functionality and the documentation will be provided later. However,
please note that the game code and the Cafu engine already fully
implement and support map scripting! Use any text editor of your choice
in order to program your map scripts independently and outside of CaWE.
In fact, all the example scripts that come with the Cafu releases have
been written that way. 😉

.. |image0| image:: /images/mapping/cawe/dialogs/dialog_entity_inspector_page1.png
   :class: medialeft
.. |image1| image:: /images/mapping/cawe/dialogs/dialog_entity_inspector_page2.png
   :class: medialeft

