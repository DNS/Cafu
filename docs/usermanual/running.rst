.. _running_cafu:

Running Cafu
============

In order to start Cafu, simply double-click on the ``Cafu.exe``
executable file. This file is in the ``Cafu-x.y`` folder that was
created during installation.

.. _the_main_menu:

The Main Menu
-------------

|mainmenu.jpg|

| After starting, the *Main Menu* appears as shown in the figure. From
  here you can configure Cafu's graphic, audio and control options,
  start new games, join multiplayer games and even host your own
  multiplayer server. *To start playing immediately just click* New
  Game\ *, select a map to play and click* Go! *to play this map on your
  local machine.* For hosting multiplayer games or joining them please
  read the following sections.

New Game
~~~~~~~~

|newgame.jpg|

| This dialog allows you to quickly start a new game. You can choose a
  map from the list on the left side of the screen. If available a
  preview screenshot of the map is shown on the right side. To start the
  game you just have to click on Go! and the the engine will
  automatically host a local game server with the chosen map and connect
  you as a client to it.

Join Online Game
~~~~~~~~~~~~~~~~

|joingame.jpg|

This dialog lets you join a multiplayer game using the given parameters:

-  Player name Your player name. Other players see this name when you
   talk to them.
-  Model Your appearance in the game world. Clicking on this control
   cycles through all available player models (the currently chosen
   model is shown in the preview windows to the right)
-  Remote Host Name The name or the IP of the server that you wish to
   connect to.
-  Remote Port The port number at which the server is listening on the
   remote host.

| Clicking Connect will attempt to connect you to the chosen server and
  the game will start if connecting was successful.

Host Game Server
~~~~~~~~~~~~~~~~

|hostgame.jpg| This dialog lets you start and configure a multiplayer
server.

-  Map Name The map hosted on the server. Clicking on this box cycles
   through all available maps (a preview of the currently chosen map is
   shown on the right).
-  Local Server Port The port number at which your server will listening
   for incoming connections.
-  Play on this machine This box lets you choose whether you want to
   play on the server yourself or just start a stand alone server.

The following two options are only relevant if you have chosen Yes in
Play on this machine.

-  Player Name Your player name. Other players see this name when you
   talk to them.
-  Model Your appearance in the game world. Clicking on this control
   cycles through all available player models (the currently chosen
   model is shown in the preview windows to the right).

Clicking Go! will start the game server and optionally connect you to
it.

.. _keyboard_layout:

Keyboard Layout
---------------

In order to control the player and operate the game, you can use the
keys as shown in the table below. The demonstrated layout reflects both
*keyboard-only* player control layout and the popular *mouse+keyboard*
player control layout. The keyboard layout will be made user
configurable in future releases.

=================== =============== =============
Action              Key             Alternative
=================== =============== =============
Move forward        (Arrow up)      ``W``
Move back           (Arrow down)    ``S``
Turn left           ← (Arrow left)  (Mouse left)
Turn right          → (Arrow right) (Mouse right)
Strafe left         ``A``           ``,``
Strafe right        ``D``           ``.``
\                                  
Jump                ``SPACE``      
Walk                ``R_SHIFT``     ``L_SHIFT``
Run (even faster)   ``R_CTRL``     
\                                  
Look up             ``PAGE UP``     (Mouse up)
Look down           ``PAGE DOWN``   (Mouse down)
Look banked CW      ``HOME``       
Look banked CCW     ``INSERT``     
Look straight ahead ``END``        
\                                  
Talk / Chat         ``T``          
Toggle Console      ``F1``         
Screen-shot         ``F5``         
Quit program        ``ESC``        
=================== =============== =============

Keyboard layout for player control and game operations.

.. _the_command_console:

The Command Console
-------------------

In order to activate or deactivate the in-game console, press the ``F1``
key.

The large upper area of the window shows the console output of any
component or subsystem of Cafu. The “Close” button in the lower right
corner closes the console window again, bringing you back to the game.
Other options to close the console window include pressing the ``F1`` or
``ESC`` key. The “Clear” button clears the contents of the console
output window.

The text input field in the lower left is used for entering console
commands. Note that the ``TAB`` key command-completion feature is
available to facilitate text entry. The up and down arrow keys navigate
through the command history for repeating previously entered commands.

Technically, the console interpreter is actually an instance of a
`Lua <http://www.lua.org/>`__ program in interactive mode. The Cafu
engine binds its console variables and functions to that program so that
they can be accessed like any other native Lua value. You may want to
refer to the `Lua documentation <http://www.lua.org/docs.html>`__ to
learn more about the language Lua and its possibilities. The fact that
you can write entire Lua programs at the Cafu in-game console provides
great customizability and flexibility for both game developers and
users.

Nonetheless, the Cafu console can also be used without any prior Lua
knowledge, or any programming knowledge at all. Here are some
getting-started examples that demonstrate the basic usage:

Entering the command

.. code:: lua

       help()

prints out a short help text with instructions on how to obtain more
help and the list of all available commands. Note that ``help()`` is
actually a function call and therefore the brackets are mandatory.

.. code:: lua

       list()

lists all available Cafu console functions, console variables, and
global Lua values. As such, the ``list()`` function is useful to look-up
the available commands. Note that the ``TAB`` key for command-completion
serves a similar purpose, but the ``list()`` function provides more
information about the values and the output is formatted.

.. code:: lua

       help("quit")
       list("cl")

The ``help(“quit”)`` function call provides help specific for the
``quit`` console variable, including a description of its purpose. This
works analogously with any other console command, too. The
``list(“cl”)`` call works like the normal ``list()``, but restricts the
output to values that begin with the string ``“cl”``.

.. code:: lua

       screenSuffix="png"

sets the value of the console variable ``screenSuffix`` to ``png``. Use
this when you want to save your screenshots in ``png`` image file format
instead of the default ``jpg``. Enter ``help(“screenSuffix”)`` for more
information about the ``screenSuffix`` console variable.

.. code:: lua

       rcon("changeLevel('filename')")

``rcon`` is the abbreviation for “remote console”. It means that the
rest of the line is not processed by the local client, but rather sent
to the remote server console. Therefore, this line instructs the server
to initiate a level (world) change.

Pressing the ``F1`` key deactivates the console and brings you back to
the game.

The config.lua File
~~~~~~~~~~~~~~~~~~~

When ``Cafu.exe`` starts up, it automatically processes the
``config.lua`` file. The ``config.lua`` file is located in the Cafu base
directory. It is run in the context of the in-game console and can
therefore contain any statement that you could also enter interactively
as described above. Because the ``config.lua`` file is processed early
during engine initialization, it is the ideal place to set default
values and to keep any console statements that you find yourself
entering over and over again. You can also define new functions or
convenient abbreviations, and in fact, a complete, full-featured Lua
program can be written into this file, either by the user, the game
developer, or both. Just have a look into the ``config.lua`` file in the
Cafu base directory to see some examples.

.. |mainmenu.jpg| image:: /images/usermanual/mainmenu.jpg
   :class: mediaright
   :width: 320px
.. |newgame.jpg| image:: /images/usermanual/newgame.jpg
   :class: mediaright
   :width: 320px
.. |joingame.jpg| image:: /images/usermanual/joingame.jpg
   :class: mediaright
   :width: 320px
.. |hostgame.jpg| image:: /images/usermanual/hostgame.jpg
   :class: mediaright
   :width: 320px
