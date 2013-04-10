/**
 * This class represents the GUI that is defined in a <tt>.cgui</tt> script file as a whole.
 * It is quasi the top-level "desktop" object that contains all the windows that together form the user interface.
 * Its methods affect the entire GUI, not just a single window of its window hierarchy.
 * (The new() method described below is an exception: It is used to create new windows.)
 *
 * Note that you never create GuiT instances yourself:
 * Instead, each GUI script accesses the global @c gui variable that is automatically predefined.
 * Thus, the methods of the GuiT class are always used like this:
 * @code
 *     -- The "gui" object is a predefined global variable.
 *     gui:showMouse(true)
 * @endcode
 *
 * @ingroup GUI
 */
class GuiT
{
    public:

    /// Sets whether the GUI is "active" or not.
    /// Active GUIs are drawn and receive device (mouse and keyboard) events and clock-tick events.
    /// Inactive GUIs aren't drawn and don't receive any events.
    /// (A newly created GuiT instance is active by default.)
    ///
    /// A GUI that receives events forwards them also to its windows,
    /// that is, the event handler methods such as WindowT::OnKeyPress(), WindowT::OnFrame() etc. are called.
    ///
    /// \note
    /// In order for a GUI to receive device events, more requirements must be met (just being active is not enough).
    /// See setInteractive() for details.
    ///
    /// \par Example:
    /// See method close() for an example.
    ///
    /// @param b   Whether the GUI is to be activated (\c true) or deactivated (\c false).
    activate(boolean b);

    /// Same as calling activate() with parameter \c false.
    ///
    /// \par Example:
    /// \code
    ///     -- The next two lines are equivalent:
    ///     gui:close();
    ///     gui:activate(false);
    /// \endcode
    close();

    /// Sets whether the GUI is "interactive" or not.
    /// Only the top-most, interactive GUI receives device (mouse and keyboard) events.
    /// For example, if the games main menu is shown on screen and the console (a separate, independent GUI) is opened "on top" of it,
    /// the (interactive) console will receive all device events while it is in the foreground.
    /// (A newly created GuiT instance is interactive by default.)
    ///
    /// A GUI that receives events forwards them also to its windows,
    /// that is, the event handler methods such as WindowT::OnKeyPress(), WindowT::OnFrame() etc. are called.
    ///
    /// \note
    /// In order for a GUI to receive events, it must also be active, see activate() for details.
    ///
    /// \par Example:
    /// A typical example for a \emph{non-interactive} GUI is the local players HUD,
    /// or any static 3D world GUI that just displays some information.
    ///
    /// @param b   \c true if the GUI is to be set interactive, \c false otherwise.
    setInteractive(boolean b);

    /// Sets whether this GUI is fullscreen and fully opaque, that is, whether this GUI covers everything underneath it.
    ///
    /// If the given boolean \c b is \c true, the Cafu GuiSys skips the rendering of the GUIs "below" this one, otherwise it doesn't.
    /// This can improve the GUI performance significantly if e.g. the player is at a point in the game where the world rendering FPS is low.
    /// In this case, if setFullCover() has been called with parameter \c false, the world FPS also drags the GUI FPS down.
    /// If however setFullCover() has been called with parameter \c true, the GuiSys omits the slow drawing of the world beneath this GUI,
    /// so that the GUIs FPS can be much higher.
    ///
    /// For newly created GUIs, the default value is \c false (other GUIs beneath this one are rendered).
    ///
    /// @param b   \c true if this GUI is fullscreen and fully opaque, \c false otherwise.
    setFullCover(boolean b);

    /// Sets the position of the mouse cursor to the point at <tt>(x, y)</tt> (in virtual screen coordinates).
    ///
    /// @param x   The x-coordinate in virtual screen coordinates (640*480) to set the mouse cursor to.
    /// @param y   The y-coordinate in virtual screen coordinates (640*480) to set the mouse cursor to.
    setMousePos(number x, number y);

    /// Sets the MatSys material that is used to render the mouse cursor.
    ///
    /// \note
    /// This is not yet implemented at this time. Instead the GuiSys always draws the built-in default cursor. Sorry.
    ///
    /// @param matName   The name of the MatSys material that is to be used for the mouse cursor.
    setMouseMat(string matName);

    /// Sets whether the mouse cursor is shown at all.
    ///
    /// Non-interactive GUIs normally don't show a mouse cursor whereas interactive GUIs do,
    /// but there also are exceptions, e.g. a GUI in which the user can control a 3D camera with the mouse is
    /// interactive but may not need a mouse cursor (the clients main world rendering is an example for this).
    /// Also fade-out or cinematic sequences can temporarily switch off the mouse-cursor with this method.
    ///
    /// (The default value is \c true, a newly created GuiT instance shows the mouse cursor by default.)
    ///
    /// @param b   When \c true, the mouse cursor is shown. When \c false, then not.
    showMouse(boolean b);

    /// Sets the keyboard input focus to the given window.
    ///
    /// By default, in a newly created GUI, no window has the keyboard input focus.
    ///
    /// When the user clicks with the mouse into a new window, the GuiSys automatically calls
    /// the WindowT::OnFocusLose() event handler for the window that loses the focus, and
    /// the WindowT::OnFocusGain() event handler for the window that receives that focus.
    ///
    /// However, when setFocus() is called, no such calls are implied!
    /// That is, if you call setFocus() and want the WindowT::OnFocusLose() and WindowT::OnFocusGain()
    /// event handlers to be called, you must do so yourself as demonstrated in the example.
    ///
    /// \par Example:
    /// \code
    ///     -- previousWin and myWin are some windows (instances of the
    ///     -- WindowT or any derived class) of the GUIs window hierarchy.
    ///     previousWin:OnFocusLose();
    ///     gui:setFocus(myWin);
    ///     myWin:OnFocusGain();
    /// \endcode
    ///
    /// @param win   The window that should receive the keyboard input focus.
    setFocus(WindowT win);

    /// Like setFocus(WindowT), but takes the name of the window to set to focus to.
    ///
    /// \par Example:
    /// \code
    ///     -- myWin is an instance of the WindowT or any derived class.
    ///     gui:setFocus(myWin);
    ///     gui:setFocus(myWin:GetName());   -- Equivalent to the line above.
    /// \endcode
    ///
    /// @param winName   Name of the window to set to focus to.
    setFocus(string winName);

    /// Returns \c true if this GUI is a 3D world GUI (an implicit requirement),
    /// it has been assigned a valid parent entity and the parent entity name is non-empty.
    /// Returns \c false otherwise.
    boolean hasValidEntity();

    /// Returns the non-empty name of the parent entity if hasValidEntity() returns \c true, an empty string otherwise.
    string getEntityName();

    /// Sets the root window for this GUI.
    /// If you use the GUI Editor that is part of the CaWE application, a proper call to this method is automatically included in the generated files.
    /// @param win   The window that is set as the root window of this GUI.
    SetRootWindow(WindowT win);

    /// Creates and returns a new window.
    /// @param Class  The (name of the) class of the newly created window, e.g. "WindowT", "ChoiceT",
    ///               or the name of any other class in the WindowT hierarchy.
    /// @param Name   The name that the newly created window instance is assigned.
    ///               Even though this parameter is optional and can be set with the WindowT::SetName() method later,
    ///               it is recommended that a unique window name is set here, so that other script code can
    ///               unambiguously find and identify the window by name later.
    /// @returns The newly created window.
    WindowT new(string Class, string Name="");

    /// Finds and returns a window by pointer value.
    /// This method is useful for debugging, when an error message referred to a window by pointer value.
    /// @param   ptr   The pointer value as obtained from an error message.
    /// @returns The window for the given pointer value, or nothing if no such window was found.
    WindowT FindWindow(number ptr);

    /// Registers the given function \c f as a new thread (a Lua coroutine).
    /// @param f   The function to be registered as a new Lua coroutine.
    thread(function f);
};
