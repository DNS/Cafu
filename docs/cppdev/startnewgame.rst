.. _starting_your_own_game:

Starting your own Game
======================

After you took
:ref:`the first steps with the Cafu source code <getting_started_with_the_cafu_source_code>`,
you're ready to use it to develop your own game or application.

This section lists the most commonly used methods for starting new
projects and explains the required steps to give you a good start.

Cloning DeathMatch
------------------

The “normal” and recommended way to start a new game is to create a copy
of the example game DeathMatch under a different name. This is useful
because it immediately provides you with the proper directory structure,
working code, and example files.

Creating the clone
~~~~~~~~~~~~~~~~~~

Creating the copy is straightforward:

#. In ``Games``, duplicate the directory ``DeathMatch`` and name the
   resulting copy as desired, e.g. ``MyGame``.
#. Delete the directory ``build`` in ``Games/MyGame/Code``.

The second step is necessary because previous builds of DeathMatch may
have created files whose copy can confuse the linker when it is creating
the program library for ``MyGame`` for the first time.

To implement these steps, you can use any method you like, for example a
file manager like Windows Explorer, or the command-line. This example
shows how you can do it under Linux:

.. code:: bash

   $ cp -r Games/DeathMatch/ Games/MyGame/
   $ rm -rf Games/MyGame/Code/build

Using your new game
~~~~~~~~~~~~~~~~~~~

Technically, the above steps are all that is required to create the
basis for your new game:

-  When you re-run SCons as documented at
   :ref:`Getting Started with the Cafu Source Code <getting_started_with_the_cafu_source_code>`,
   the code of your new game will automatically be picked up, compiled
   and linked.
-  When you start CaWE, it will automatically recognize the new game and
   show it in the “Configure”, “New Map”, “New GUI”, … etc. dialogs. If
   “Start Engine” is checked in the “Compile” menu, it will also start
   the Cafu Engine with the new game.
-  If you run the Cafu Engine from the command prompt, make sure to add
   parameter
   ::

      -svGame MyGame

   to the command-line (see below how to get rid of it again).

Tips and Tricks
~~~~~~~~~~~~~~~

-  If you want to run your game by default (without ``-svGame``), see
   the section about the variable ``GameLibs`` in file
   ``CompilerSetup.py``:
   .. code:: python

      # When you're developing your own game, you might want to keep game DeathMatch
      # for reference. Cafu should run your game by default (first in list), whereas
      # DeathMatch would be readily available via the -svGame command-line option:
      GameLibs = ["MyGame", "DeathMatch"]

   Note that you have to re-run SCons in order for changes to take
   effect.

-  In ``MyGame``, some paths in the code and scripts still point to
   ``Games/DeathMatch/…``. For the beginning, that's ok – your game will
   find some of its resources in game DeathMatch, and you can update
   these occurrences step by step at any later time (or even
   intentionally keep things like this).
-  Check out
   :ref:`Game Development Overview <game_development_overview>` for
   details about the files and subdirectories in the new game directory.

Alternatives
------------

Here is a list of alternative courses of action that you may want to
consider in order to start a new game:

Modifying DeathMatch
~~~~~~~~~~~~~~~~~~~~

Working directly with the DeathMatch game “in situ” may seem like a
totally silly idea to you, but in some cases it can be a worthwhile
consideration nevertheless:

If you want to quickly try something out (e.g. a new entity class), or
if you work on something that is not directly or entirely a completely
new game (for example, any changes to the core engine), the game
DeathMatch might be a welcome testbed.

Also, if you wish to communicate and collaborate with people who are not
directly involved with your game code, the DeathMatch code can be a good
common ground.

Renaming DeathMatch
~~~~~~~~~~~~~~~~~~~

Renaming DeathMatch, instead of properly copying it, would principally
work, if it was not for these factors:

-  It does not agree well with version control systems (Git or
   Subversion). If you follow our recommendation to checkout the Cafu
   source code from our Git or Subversion repository, so that you can
   keep track of our latest developments while developing your own game
   in parallel, then renaming the DeathMatch directory is (indeed not
   impossible, but) somewhat incompatible with the way how these
   repositories work.
-  Some file paths in code and scripts still point to
   ``Games/DeathMatch/…``. If you rename DeathMatch, all these paths are
   broken, and your game will not work without fixing them first. If you
   instead copy DeathMatch, your game will find some of its resources in
   game DeathMatch, and you can update these occurrences step by step at
   any later time (or even intentionally keep things like this).

Overall, we recommend *not* to rename DeathMatch. Better clone it as
explained above.

Starting from scratch
~~~~~~~~~~~~~~~~~~~~~

Creating a new, empty directory in ``Games/`` in order to start from
scratch, then fill it with contents as required, is certainly possible.

Doing this can be a very good choice if your main goal is to learn and
understand the technical details from the ground up, or if you only want
to have original content in your game right from the start.

In fact, we're considering complementing the DeathMatch example game
with another “Minimal” example game that comes closer to these goals.

If you start with an empty directory,

-  your progress will be slower,
-  despite the support forums are always available, you should be
   technically versed enough to cope with most problems and errors,
-  be prepared to copy pieces of DeathMatch example code anyway.

Using VSWM
~~~~~~~~~~

The VSWM MOD is old, obsolete, dysfunctional, and a candidate for
removal. Not a good basis for a new game. Don't use it.
