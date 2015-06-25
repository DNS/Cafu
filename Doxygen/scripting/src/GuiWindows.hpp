namespace GuiSys
{


/// A window is the basic element of a graphical user interface.
///
/// Windows are hierarchically arranged in parent/child relationships to form complex user interfaces.
///
/// Each window essentially represents a rectangular shape, but only has very little features of its own.
/// Instead, a window contains a set of components, each of which implements a specific feature for the window.
///
/// If you would like to create a new window explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local win = gui:new("WindowT", "my_window")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,WindowT}
class WindowT
{
    public:

    /// This method adds the given window to the children of this window.
    AddChild(window child);

    /// This method removes the given window from the children of this window.
    /// @param child   The window that is to be removed from the children of this window.
    RemoveChild(window child);

    /// This method returns the parent of this window (or `nil` if there is no parent).
    window GetParent();

    /// This method returns an array of the children of this window.
    table GetChildren();

    /// This method returns the windows local time (starting from 0.0).
    number GetTime();

    /// This method returns the "Basics" component of this window.
    ComponentBasicsT GetBasics();

    /// This method returns the "Transform" component of this window.
    ComponentTransformT GetTransform();

    /// This method adds a component to this window.
    AddComponent(ComponentBaseT component);

    /// This method removes a component from this window.
    RemoveComponent(ComponentBaseT component);

    /// This method returns an array of the components of this window.
    table GetComponents();

    /// This method returns the (n-th) component of the given (type) name.
    /// Covers the "custom" components as well as the application components, "Basics" and "Transform".
    /// That is, `GetComponent("Basics") == GetBasics()` and `GetComponent("Transform") == GetTransform()`.
    /// @param type_name   The (type) name of the component to get, e.g. "Image".
    /// @param n           This parameter is optional, it defaults to 0 if not given.
    ComponentBaseT GetComponent(string type_name, number n);


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /// Called for each window when the GUI is activated (i.e.\ switched on for rendering).
    /// For example, when the user toggles the in-game console, this method is called every time the console GUI comes up.
    OnActivate();

    /// Called for each window when the GUI is deactivated.
    ///
    ///   - This method mainly exists for symmetry with OnActivate(), it has little practical use.
    OnDeactivate();

    /// This method is called when this window gains the keyboard input focus.
    ///
    ///   - This \emph{not} called when the focus change was triggered by the script, i.e. by a call to gui:setFocus(myWin).
    ///     That is, this method is only called when the user triggered the focus change by a mouse click.
    ///     If you use gui:setFocus(myWin) and want this method called, just call it yourself immediately after the call.
    OnFocusGain();

    /// This method is called when this window loses the keyboard input focus.
    ///
    ///   - This \emph{not} called when the focus change was triggered by the script, i.e. by a call to gui:setFocus(myWin).
    ///     That is, this method is only called when the user triggered the focus change by a mouse click.
    ///   - Currently, losing the focus cannot be vetoed.
    OnFocusLose();

    /// Called on each frame that is rendered by the Cafu engine.
    ///
    ///   - Note that it does not get the frame time passed, but you can always learn the global time by calling get()
    ///     with parameter <tt>"time"</tt>, e.g. \code t = self:get("time") \endcode
    ///   - Obviously, expensive computations cannot be run in this method &mdash; they would directly impact the framerate!
    OnFrame();

    /// This is the first method that is called for each window after a GUI has been loaded.
    /// It is only called once for each window.
    ///
    /// Note that OnInit() callbacks are special: They're automatically written by the CaWE GUI Editor as part of the
    /// `_init.cgui` file whenever it saves a file. That is, you can normally not write an OnInit() callback yourself,
    /// because it has been used already in the normal init code of the GUI. See OnInit2() for a callback with the
    /// same purpose that is available for your customizations.
    OnInit();

    /// This method is very much like OnInit(), but intended for your own custom use in the
    /// `_main.cgui` file. It is only called once for each window.
    OnInit2();

    /// This method is called if a key was pressed and the window has the keyboard input focus.
    ///
    /// @param Key   The integer number that represents the key that has been pressed.
    ///     See <a href="http://docs.cafu.de/c++/OpenGLWindow_8hpp_source.html#l00044" style="color: #4665A2;">OpenGLWindow.hpp</a>
    ///     for a list of possible values for \c Key.
    ///     Here is a list of the most frequently used values and their meaning:
    ///
    /// <table>
    ///   <tr><th> Key Value </th><th> Key Name                  </th></tr>
    ///   <tr><td>         1 </td><td> ESC (Escape)              </td></tr>
    ///   <tr><td>    2...11 </td><td> 1 ... 9, 0                </td></tr>
    ///   <tr><td>        14 </td><td> Backspace                 </td></tr>
    ///   <tr><td>        15 </td><td> TAB (Tabulator)           </td></tr>
    ///   <tr><td>        28 </td><td> Return (on main keyboard) </td></tr>
    ///   <tr><td>        41 </td><td> Accent Grave              </td></tr>
    ///   <tr><td>       156 </td><td> Enter (on numpad)         </td></tr>
    ///   <tr><td>       200 </td><td> Arrow Up                  </td></tr>
    ///   <tr><td>       208 </td><td> Arrow Down                </td></tr>
    /// </table>
    ///
    /// @returns The method should return `true` if it handled ("consumed") this event, `false` otherwise.
    boolean OnKeyPress(number Key);

    /// Like OnKeyPress(), but for key releases.
    boolean OnKeyRelease(number Key);

    /// This method is called when a character event occurred and the window has the keyboard input focus.
    ///
    /// The difference to OnKeyPress() and OnKeyRelease() is that these two just deals with plain keys,
    /// which is useful for example to work with the arrow keys.
    /// OnChar() however deals with entered \emph{characters}, which for example take the state of the SHIFT key into account,
    /// language specific keyboard settings and layout, etc. Therefore, OnChar() is the preferred choice for text input.
    ///
    ///   - Characters are represented as ASCII codes. That is, the integer values that `ch` may assume match the ASCII table.
    ///   - You can use the <tt>string.char(ch)</tt> Lua library function in order to obtain a string that consist
    ///     of the character that corresponds to the numeric value of `ch`.
    ///
    /// @param ch   The integer number that represents the character.
    /// @returns The method should return `true` if it handled ("consumed") this event, `false` otherwise.
    boolean OnChar(number ch);

    /// This method is called when a mouse button went down (was pressed).
    /// @param button   The number of the mouse button that went down. (Typically 0 for the left, 1 for the right, 2 for the middle, and 3 for extra mouse button.)
    /// @returns The method should return `true` if it handled ("consumed") this event, `false` otherwise.
    boolean OnMouseButtonDown(number button);

    /// This method is called when a mouse button went up (was released).
    /// @param button   The number of the mouse button that went up. (Typically 0 for the left, 1 for the right, 2 for the middle, and 3 for extra mouse button.)
    /// @returns The method should return `true` if it handled ("consumed") this event, `false` otherwise.
    boolean OnMouseButtonUp(number button);

    /// This method is called when the mouse cursor enters the rectangle of the window.
    ///
    /// \note This does currently not take rotation into account, i.e. it acts as if the
    /// rotation was always 0, even if the window rectangle is actually rotating.
    OnMouseEnter();

    /// This method is called when the mouse cursor leaves the rectangle of the window.
    ///
    /// \note This does currently not take rotation into account, i.e. it acts as if the
    /// rotation was always 0, even if the window rectangle is actually rotating.
    OnMouseLeave();

    // This method is currently not called (commented out in C++ code).
    // boolean OnMouseMove(number amount_x, number amount_y);

    /** @} */
};


}   // namespace GuiSys
