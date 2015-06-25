namespace GuiSys
{


/// This class holds the hierarchy of windows that together form a GUI.
///
/// This class represents the GUI that is defined in a `.cgui` script file as a whole.
/// It is quasi the top-level "desktop" object that contains all the windows that together form the user interface.
/// Its methods affect the entire GUI, not just a single window of its window hierarchy.
/// (The new() method described below is an exception: It is used to create new windows and new components.)
///
/// Note that you never create GuiT instances yourself:
/// Instead, each GUI script accesses the global `gui` variable that is automatically predefined.
/// Thus, the methods of the GuiT class are always used like this:
/// @code
///     -- The "gui" object is a predefined global variable.
///     gui:showMouse(true)
/// @endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,GuiImplT}
class GuiImplT
{
    public:

    /// Sets the IsActive flag of this GUI.
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
    /// \par Example
    /// See method close() for an example.
    ///
    /// @param IsActive   Whether the GUI is to be activated (\c true) or deactivated (\c false).
    activate(bool IsActive);

    /// Same as calling `gui:activate(false);`
    /// Same as calling activate() with parameter \c false.
    ///
    /// \par Example
    /// \code{.lua}
    ///     -- The next two lines are equivalent:
    ///     gui:close()
    ///     gui:activate(false)
    /// \endcode
    close();

    /// Sets the IsInteractive flag of this GUI.
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
    /// \par Example
    /// A typical example for a \emph{non-interactive} GUI is the local players HUD,
    /// or any static 3D world GUI that just displays some information.
    ///
    /// @param IsInteractive   \c true if the GUI is to be set interactive, \c false otherwise.
    setInteractive(bool IsInteractive);

    /// Sets the IsFullCover flag of this GUI.
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
    /// @param IsFullCover   \c true if this GUI is fullscreen and fully opaque, \c false otherwise.
    setFullCover(bool IsFullCover);

    /// Sets the position of the mouse cursor.
    /// Sets the position of the mouse cursor to the point at <tt>(x, y)</tt> (in virtual screen coordinates).
    ///
    /// @param x   The x-coordinate in virtual screen coordinates (640*480) to set the mouse cursor to.
    /// @param y   The y-coordinate in virtual screen coordinates (640*480) to set the mouse cursor to.
    setMousePos(number x, number y);

    /// Sets the size of the mouse cursor.
    /// This can be useful especially in 3D world GUIs, where the default size can be too hard to see.
    setMouseCursorSize(number size);

    /// Sets the material that is used to render the mouse cursor.
    /// (This method is not yet implemented.)
    /// Sets the MatSys material that is used to render the mouse cursor.
    ///
    /// \note
    /// This is not yet implemented at this time. Instead the GuiSys always draws the built-in default cursor. Sorry.
    ///
    /// @param MatName   The name of the MatSys material that is to be used for the mouse cursor.
    setMouseMat(string MatName);

    /// Determines whether the mouse cursor is shown at all.
    /// Sets whether the mouse cursor is shown at all.
    ///
    /// Non-interactive GUIs normally don't show a mouse cursor whereas interactive GUIs do,
    /// but there also are exceptions, e.g. a GUI in which the user can control a 3D camera with the mouse is
    /// interactive but may not need a mouse cursor (the clients main world rendering is an example for this).
    /// Also fade-out or cinematic sequences can temporarily switch off the mouse-cursor with this method.
    ///
    /// (The default value is \c true, a newly created GuiT instance shows the mouse cursor by default.)
    ///
    /// @param IsShown   When \c true, the mouse cursor is shown. When \c false, then not.
    showMouse(bool IsShown);

    /// This method creates new GUI windows or new GUI components.
    ///
    /// @par Example
    /// \code{.lua}
    ///     --// Create a new WindowT instance and set its object name.
    ///     local winOK = gui:new("WindowT", "ButtonOK")
    ///
    ///     --// Create another window, but set its name in a separate step.
    ///     local winCancel = gui:new("WindowT")
    ///     winCancel:GetBasics():set("Name", "ButtonCancel")
    ///
    ///     --// Create some components and add them to winOK.
    ///     local c1 = gui:new("ComponentTextT")
    ///     c1:set("Text", "OK")
    ///     c1:set("horAlign", 0)
    ///
    ///     local c2 = gui:new("ComponentBorderT")
    ///     c2:set("Width", 0.6)
    ///
    ///     local c3 = gui:new("ComponentImageT")
    ///     c3:set("Alpha", 0.5)
    ///
    ///     winOK:AddComponent(c1, c2, c3)
    /// \endcode
    ///
    /// @param ClassName
    ///     The name of the class of which an object should be created.
    ///     Use `"WindowT"` in order to create a new WindowT.
    ///     Use any class name from the ComponentBaseT hierarchy in order to create a new component,
    ///     for example `"ComponentImageT"` in order to create a new image component.
    ///
    /// @param InstanceName
    ///     The name that the newly created window instance is assigned.
    ///     Specifying a name for a window is equivalent to setting the `Name` attribute of its Basics component;
    ///     see the example above and ComponentBasicsT.Name for details.
    ///     Setting a proper window instance name is important so that other script code can unambiguously find
    ///     and identify the window by name later. The CaWE GUI Editor also uses it in order to automatically create
    ///     the initialization script code in the `.cgui` files.
    ///     This parameter is not used (and in fact ignored) for components, which have no individual object names.
    ///
    /// @returns The newly created object.
    object new(string ClassName, string InstanceName="");

    /// Sets the keyboard input focus to the given window. Does *not* call the OnFocusLose() or OnFocusGain() callbacks!
    /// Instead of a window instance, it is also possible to pass the name of the window (a string).
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
    /// \par Example
    /// \code{.lua}
    ///     -- previousWin and myWin are some windows (instances of the
    ///     -- WindowT or any derived class) of the GUIs window hierarchy.
    ///     previousWin:OnFocusLose();
    ///     gui:setFocus(myWin);
    ///     myWin:OnFocusGain();
    /// \endcode
    ///
    /// @param Window   The window that should receive the keyboard input focus.
    setFocus(WindowT Window);

    /// Like setFocus(WindowT), but takes the name of the window to set to focus to.
    ///
    /// \par Example
    /// \code{.lua}
    ///     -- myWin is an instance of the WindowT or any derived class.
    ///     gui:setFocus(myWin);
    ///     gui:setFocus(myWin:GetName());   -- Equivalent to the line above.
    /// \endcode
    ///
    /// @param winName   Name of the window to set to focus to.
    setFocus(string winName);

    /// Returns the root window of this GUI as previously set by SetRootWindow().
    WindowT GetRootWindow();

    /// Sets the root window for this GUI.
    /// Sets the root window for this GUI.
    /// If you use the GUI Editor that is part of the CaWE application, a proper call to this method is automatically included in the generated files.
    /// @param Window   The window that is set as the root window of this GUI.
    SetRootWindow(WindowT Window);

    /// Calls the OnInit() script methods of all windows.
    /// This method calls the `OnInit()` script methods of all windows.
    ///
    /// Normally, the `OnInit()` callback is automatically called for each window as soon as the
    /// `_main.cgui` file has been read. That is, there is an automatic implicit call to Init()
    /// at the end of the `_main.cgui` script.
    /// However, sometimes that's not enough, and you need all the window's `OnInit()` methods already
    /// run e.g. at the top of the `_main.cgui` script, e.g. in order to make sure that all components
    /// of all windows have been created, so that in `_main.cgui` you can grab their instances and
    /// attach callback methods to them.
    /// In this case, call Init() manually at the top of `_main.cgui`.
    /// (It automatically makes sure not to initialize things twice when called multiple times.)
    /// See `DeathMatch/GUIs/MainMenu/MainMenu_main.cgui` for an example.
    Init();
};


}   // namespace GuiSys
