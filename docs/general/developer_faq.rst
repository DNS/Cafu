.. _developer_faq:

Developer FAQ
=============

Is Cafu right for our game of genre "..."?
------------------------------------------

Variants of this question include:

-  Can we use Cafu to make a
   `MMORPG <https://en.wikipedia.org/wiki/Massively_multiplayer_online_role-playing_game>`__?
-  Is Cafu fit for our 3D strategy game?
-  Will we able to implement our car racing game with Cafu?

While we would love to provide you with a clear and authoritative “yes”
or “no” answer to these questions, that is easier said than done:

When you ask one of these questions, you usually have a clear mental
image already on how your game should look like – an image that we don't
have, because often we know way too little about your project: your
plans, your team, your skills (modelling, mapping, scripting,
programming, …), your deadlines, etc. Not knowing what you *really* want
and if you're ready to – for example – writing material shaders or
making changes to the C++ source code is what makes this question
difficult to answer.

However, there is an alternative that is much better than any statement
we could make: Familiarize yourself with Cafu, try it yourself, and
evaluate if it is right for your needs.

For example, download and run the latest demo. Explore the TechDemo map.
Then try making a small map of your own, or edit one of the included
maps in the editor. Browse the scripts and source code. If there are
problems or questions, `ask <http://www.cafu.de/developers>`__.

Not only will you become familiar with the Cafu details, you will also
be able to form a very competent opinion about Cafu's suitability for
your project.

.. _where_are_the_visual_studio_project_files:

Where are the Visual Studio project files?
------------------------------------------

In the development of Cafu, an important consideration is the build
process that turns the source code into executable programs. This build
process is a challenge in its own right, because it must work

-  on all supported platforms (currently Windows and Linux, soon MacOS,
   too),
-  with several compilers (and several versions of each),
-  in several variants (32- and 64-bit; debug, profile and release),
-  possibly with multiple combinations of the above, simultaneously on
   the same computer,
-  with all external libraries,
-  in changing environments (e.g. variable number of game libraries),
-  flexibly, robustly and easily configurable for our fellow developers
   and users.

Achieving these goals with Visual Studio solution files on Windows (and
possibly Makefiles on Linux) is a maintenance nightmare that borders on
the impossible. For these reasons, we have chosen to use
`SCons <http://www.scons.org/>`__ as the build system for the Cafu
Engine. SCons meets the above requirements, and it can still be used
automatically and conveniently from most text editors.

Alas, we realize that having true Visual Studio project and solution
files (and similar native files for other IDEs) would still be nice to
have. We therefore started looking into these programs:

-  `Premake <http://industriousone.com/premake>`__
-  `Bakefile <https://github.com/vslavik/bakefile>`__
-  `CMake <http://www.cmake.org>`__

These all look very promising, but each has problems of its own and
until today nobody ever finished a complete, working solution. If you
would like to help improving the situation, it would be very welcome.

What is the maximum number of network players?
----------------------------------------------

There is no hard limit built into Cafu, but the general rule is that in
the worst case, updates of everyone must be distributed over the network
to everyone else. That means that the overall network load (on the
server) roughly climbs with the number of players squared.

Fortunately, the Cafu network code is able to cut the network load
significantly in many cases:

-  Each network message is very small from the beginning, and
   additionally compressed.
-  The Cafu server knows “who can (potentially) see who”, and optimizes
   all cases where updates would be sent that cannot be observed by the
   receiving client.

With these features, Cafu can handle a large number of players in a
single level all at the same time. We have demonstrated that connecting
8 clients over the internet is possible without problems (and with some
bandwidth left) – and that was several years ago over a slow ISDN (64
kbit/s) Internet connection.

How can I clone the Cafu source code repository?
------------------------------------------------

You can clone the Cafu source code repository like this:

.. code:: bash

   > git clone --recursive https://bitbucket.org/cafu/cafu.git Cafu

Also see
:ref:`Getting Started with the Cafu Source Code <getting_started_with_the_cafu_source_code>`
for more details and https://bitbucket.org/cafu/cafu, where you can
browse the repository online, create forks, submit pull requests, and
find additional help texts.

How do I start a new game (MOD) project?
----------------------------------------

This is now explained in its own section in the documentation: See
:ref:`Starting your own Game <starting_your_own_game>` for details.

.. _how_do_i_dynamically_reload_the_map_script_in-game:

How do I dynamically reload the map script in-game?
---------------------------------------------------

While you're developing a script for one of your maps, it can be a very
helpful shortcut to reload the map script while the game is running,
without interrupting it. This avoids the normally required cycle of
leaving and re-starting the map.

As a preparatory step, add a function like this to your map script:

.. code:: lua

   -- This function reloads and runs this script again.
   -- It is useful for working with and testing the script "online",
   -- i.e. while the game is running and without reloading the map!
   function reloadScript()
       -- Adjust the path in the next line as required!
       dofile("Games/DeathMatch/Worlds/BPWxBeta.lua");
       Console.Print("Reloaded the map script.\n");
   end

Then, at the :ref:`Cafu in-game console <the_command_console>`, enter

.. code:: lua

   runMapCmd('reloadScript()')

to dynamically reload the script.

In theory, you could combine these two steps into one, using something
like ``runMapCmd('dofile(“…”)')``, but that is even harder to type than
the version above, and less flexible. (We realize that the entire
``runMapCmd(…)`` business is not optimal. Suggestions for making this
more convenient are welcome.)

Note that if you use this technique, it can be helpful (but is not
required) to understand how it works: The map script “lives” inside a
Lua state that is initialized together with the map. (The in-game
console has an entirely separate Lua state, which is why
``runMapCmd(…)`` is needed.) The above commands essentially run
``dofile(…)`` in the context of the maps Lua state, which in turn
re-runs the map script.

Where can I learn more about 3D programming or game engines?
------------------------------------------------------------

There are many available resources regarding 3D engines and related
their subjects, both online and offline in the form of traditional books
and papers. The following list is neither complete nor up-to-date, but
we hope that it serves as a starting point for your own investigations
into the matter:

-  `NeHe OpenGL Tutorials <http://nehe.gamedev.net>`__
   Excellent OpenGL tutorials for beginners to the advanced.

-  http://www.opengl.org
   The official OpenGL website, and the first place to search for
   information about OpenGL.

-  `"Visibility Computations in Densely Occluded Polyhedral
   Environments"
   (1992) <http://graphics.lcs.mit.edu/~seth/pubs/pubs.html>`__
   Seth Teller's dissertation about Potentially Visibility Sets (PVS): a
   common data structure in modern 3D engines that speeds up rendering.

-  `BSP-FAQs <http://www.faqs.org/faqs/graphics/bsptree-faq>`__
   Introductory information about BSP trees.

-  “Computer Graphics, Principles and Practice” by Foley, vanDam, Feiner
   & Hughes.
   Some say this is the bible of computer graphics – and it's true. It
   covers nearly everything there is to know. An excellent reference,
   but have a look into the table of contents before buying it.

-  “Radiosity and Realistic Image Synthesis”, Michael F. Cohen and John
   R. Wallace. Morgan Kaufmann Publishers, Inc., San Francisco,
   California.

-  “Radiosity. A programmers perspective.”, Ian Ashdown. John Wiley &
   Sons, Inc., New York, N.Y.

-  “Zen of Graphics Programming, 2nd edition” by Michael Abrash.
   Most of this book is about low level DOS graphics programming
   (outdated now, but good nonetheless). The final chapters are a
   collection of Abrash's articles that appeared in the “Dr. Dobbs
   SourceBooks” series. They contain information about BSP trees, span
   sorting, etc.

-  `C++ FAQs <http://www.parashift.com/c++-faq-lite/>`__ by Cline,
   Lomow, and Girou; Addison-Wesley. Available
   `online <http://www.parashift.com/c++-faq-lite/>`__ and `as a
   book <http://www.parashift.com/c++-faq-lite/faq-book.html>`__.

-  “Effective C++, Third Edition” by `Scott
   Meyers <http://www.aristeia.com/books.html>`__.

-  `The C++ Programming
   Language <http://en.wikipedia.org/wiki/The_C%2B%2B_Programming_Language>`__
   by Bjarne Stroustrup.

-  “Programming in Lua, Second Edition” by Roberto Ierusalimschy. The
   first edition of this book is also available
   `online <http://www.lua.org/pil/index.html>`__.
