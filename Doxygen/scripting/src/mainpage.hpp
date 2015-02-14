/**
 * @mainpage Scripting Documentation
 *
 * @section overview Overview
 *
 * Welcome to the <em>Cafu Engine Scripting Documentation</em>.
 *
 * You can use scripts to program the behaviour of things in the Cafu universe.
 *
 * This reference documentation describes the classes and functions that can occur and be used in Cafu game scripts, especially:
 *
 *   - Map scripts &ndash; Scripts that program and control the entities that live within a game world
 *     (such as human players, monsters, weapons, doors, lifts, ...).
 *   - GUI scripts &ndash; Scripts that control the user interaction with 2D GUIs (e.g. the programs main menu)
 *     and 3D GUIs (e.g. the teleporter controls, unlock door keypads, etc.).
 *
 * Other sub-systems in Cafu are script-controlled as well, such as the in-game console,
 * the program configuration file, the render materials and sound shaders, etc.
 * For these types of script, please refer to the documentation at http://www.cafu.de/wiki/
 *
 *
 * @section gettingstarted How do I get started?
 *
 * For getting started with scripting, the "learning-by-doing" approach often works very well, because it's fast and fun:
 * Start by studying and modifing the example scripts that ship with Cafu (see links below),
 * or take them as starting points for your own work.
 * Experimenting with working scripts will make it easy for you to understand the underlying key ideas and concepts.
 * Then come back here and read the documentation as required.
 * You may also want to have a look at the Lua documentation and/or start asking questions at the Cafu mailing list or forum.
 *
 *
 * @section moreinfo Additional Resources of Information
 *
 *   - For an overview and introduction to Cafu development, see the documentation at http://www.cafu.de/wiki/
 *   - The Developer Resources page is at http://www.cafu.de/developers
 *   - The scripting implementation in Cafu is based on the programming language <a href="http://www.lua.org">Lua</a>.
 *       - Learn more about Lua at http://www.lua.org/about.html
 *       - Check out the very good Lua documentation at http://www.lua.org/docs.html <br />
 *         It provides all the background information about the language and is an indispensable resource for advanced Cafu scripting!
 *   - For example scripts, see
 *       - http://trac.cafu.de/browser/cafu/trunk/Games/DeathMatch/Worlds for map scripts, and
 *       - http://trac.cafu.de/browser/cafu/trunk/Games/DeathMatch/GUIs for GUI scripts.
 *       - These scripts also ship with each Cafu release and are found in the respective subdirectories.
 *   - If you're looking for the \emph{C++} reference documentation instead, see http://docs.cafu.de/c++/
 */


/**
 * @page eventhandlers Event Handlers (Callbacks)
 *
 * Some of our script classes have methods that are called *event handlers* or *event callbacks*.
 * The related documentation groups such methods in a separate section that is labelled "Event Handlers (Callbacks)".
 *
 * The key feature about these methods is that they are called *automatically*.
 * Whenever the related event occurs, the related C++ code (e.g. the game code or the GUI system)
 * calls the appropriate event handler.
 *
 * Contrary to "normal" methods, none of these methods is predefined by or pre-implemented,
 * and you normally don't call these methods yourself (although you can).
 * Instead, with these methods, the roles are reversed:
 * If you're interested in handling a specific event, define (write) the related event handler method.
 *
 * The C++ code (the game code, the GUI system, etc.) will call it when the related event occurs,
 * using the appropriate function parameters for the event (this is similar to overriding virtual methods in C++).
 * Your own implementation code then determines how the event is handled.
 *
 * The "Event Handlers (Callbacks)" documentation of a class lists and describes all callback methods that are available,
 * but you have to provide an implementation only if you wish to handle the related event.
 * Methods for events that you are not interested in need not be implemented at all.
 *
 * Classes that have callback methods include (list may be incomplete!):
 *   - \link GuiSys::WindowT WindowT\endlink,
 *     \link GuiSys::ComponentBaseT ComponentBaseT\endlink,
 *     \link GuiSys::ComponentChoiceT ComponentChoiceT\endlink,
 *     \link GuiSys::ComponentListBoxT ComponentListBoxT\endlink
 */


/* // TODO: Try this out before adding it to the Doxygen documentation!
 * @section List all methods of script object
 *
 * You can list all methods of a script object (e.g. the @c gui object, a GUI window, a map entity, ...)
 * using this code:
 * @code
 *     function listMethods(t)
 *         for k, v in pairs(t)
 *         end
 *     end
 *
 *     listMethods(gui.getmetatable())
 * @endcode
 */
