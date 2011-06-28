/**
 * @defgroup GUI Graphical User Interface
 *
 * This module groups all script classes related to graphical user interfaces (GUIs) in Cafu.
 *
 * Cafu GUIs are like computer desktops: They are made of a hierarchy of windows.
 * Each window has graphical attributes and elements, it can contain text and can be interacted with.
 * GUIs can be used both in 2D, e.g. for the game's Main Menu, as well as in 3D,
 * e.g. for the in-game controls of a teleporter station, to call lifts, etc.
 *
 * GUI scripts are used both to describe the window setup and contents, as well as
 * to react to window events like mouse cursor movement and clicks, keyboard input, etc.
 * In a sense, a GUI is like a mini operating system, and the GUI scripts are the means to program it.
 *
 * For example GUI scripts, see the files in
 *   - http://trac.cafu.de/browser/cafu/trunk/Games/DeathMatch/GUIs
 *
 * GUI scripts have access to our <a href="group__Common.html">Common</a> libraries as well as
 * to (most of) the <a href="http://www.lua.org/manual/5.1/manual.html#5">Lua standard libraries</a>.
 *
 * @{
 */


/// This class represents a basic, rectangular window.
/// Each window has attributes like position, size, border, color, text, animations, effects, and more.
/// The built-in behaviour of a window is generally determined by its \emph{class} (WindowT or one of its subclasses)
/// when the window is created, and can optionally be augmented by custom script code.
/// Windows also have parent and child windows, and the entire window hierarchy forms the GUI.
///
/// <h3>Event Handler Callbacks</h3>
/// The methods that are listed in this group are \emph{event handlers}:
/// They are called automatically by the Cafu GUI system whenever the related event occurs.
///
/// Contrary to "normal" methods, none of these methods is predefined by or implemented by the Cafu GUI system,
/// and you normally don't call these methods yourself (although you can).
///
/// Instead, with these methods, the roles are reversed:
/// If you're interested in handling a specific event, define (write) the related event handler method.
/// The Cafu GUI system will call it when the related event occurs, using the appropriate function parameters
/// for the event (this is similar to overriding virtual methods in C++).
/// Your own implementation code then determines how the event is handled.
///
/// This list documents all callback methods that are available,
/// but you have to provide an implementation only if you wish to handle the related event.
/// Methods for events that you do not require need not be implemented at all.
///
/// @nosubgrouping
/// @cppName{WindowT}
class WindowT
{
    public:

    /**
     * Sets the window attribute with name @c attrib to the given value.
     *
     * Here is a list of all attribute names that can be used for @c attrib, logically grouped.
     * The second column indicates the number and type of the required values:
     *
     * <table>
     *   <tr><th>Name</th><th>Type</th><th>Remarks</th></tr>
     *   <tr><td> show     </td><td> boolean </td><td> If this window is shown. </td></tr>
     *   <tr><td> time     </td><td> number  </td><td> Automatically incremented on each frame, but can freely be modified. </td></tr>
     *   <tr><td> rotAngle </td><td> number  </td><td> The angle of rotation of this window, in degrees (0..360). </td></tr>
     *   <tr><td colspan="3"></td></tr>
     *   <tr><th>Name</th><th>Type</th><th>Remarks</th></tr>
     *   <tr><td> rect   </td><td> number, number, number, number </td><td> The (x, y) position and (width, height) dimensions of this window. The position is relative to the parent window. </td></tr>
     *   <tr><td> pos    </td><td> number, number                 </td><td> Just the first two components of ''rect'': the (x, y) position, relative to the parent window. </td></tr>
     *   <tr><td> size   </td><td> number, number                 </td><td> Just the last two components of ''rect'': The width and height. </td></tr>
     *   <tr><td> pos.x  </td><td> number                         </td><td> The x-position of this window (1st component of ''rect''). </td></tr>
     *   <tr><td> pos.y  </td><td> number                         </td><td> The y-position of this window (2nd component of ''rect''). </td></tr>
     *   <tr><td> size.x </td><td> number                         </td><td> The width of this window (3rd component of ''rect''). </td></tr>
     *   <tr><td> size.y </td><td> number                         </td><td> The height of this window (4th component of ''rect''). </td></tr>
     *   <tr><td colspan="3"></td></tr>
     *   <tr><th>Name</th><th>Type</th><th>Remarks</th></tr>
     *   <tr><td> backMaterial </td><td> string                         </td><td> The name of the MatSys material to be used as the background image of this window. </td></tr>
     *   <tr><td> backColor    </td><td> number, number, number, number </td><td> The red, green, blue and alpha components of the background color. </td></tr>
     *   <tr><td> backColor.r  </td><td> number                         </td><td> The red component of the background color. </td></tr>
     *   <tr><td> backColor.g  </td><td> number                         </td><td> The green component of the background color. </td></tr>
     *   <tr><td> backColor.b  </td><td> number                         </td><td> The blue component of the background color. </td></tr>
     *   <tr><td> backColor.a  </td><td> number                         </td><td> The alpha component of the background color. </td></tr>
     *   <tr><td colspan="3"></td></tr>
     *   <tr><th>Name</th><th>Type</th><th>Remarks</th></tr>
     *   <tr><td> borderWidth   </td><td> number                         </td><td> The width of the border frame. </td></tr>
     *   <tr><td> borderColor   </td><td> number, number, number, number </td><td> The red, green, blue and alpha components of the border color. </td></tr>
     *   <tr><td> borderColor.r </td><td> number                         </td><td> The red component of the border color. </td></tr>
     *   <tr><td> borderColor.g </td><td> number                         </td><td> The green component of the border color. </td></tr>
     *   <tr><td> borderColor.b </td><td> number                         </td><td> The blue component of the border color. </td></tr>
     *   <tr><td> borderColor.a </td><td> number                         </td><td> The alpha component of the border color. </td></tr>
     *   <tr><td colspan="3"></td></tr>
     *   <tr><th>Name</th><th>Type</th><th>Remarks</th></tr>
     *   <tr><td> font         </td><td> string                         </td><td> The name of the font to render the text with. </td></tr>
     *   <tr><td> text         </td><td> string                         </td><td> The text to display in this window. </td></tr>
     *   <tr><td> textColor    </td><td> number, number, number, number </td><td> The red, green, blue and alpha components of the text color. </td></tr>
     *   <tr><td> textColor.r  </td><td> number                         </td><td> The red component of the text color. </td></tr>
     *   <tr><td> textColor.g  </td><td> number                         </td><td> The green component of the text color. </td></tr>
     *   <tr><td> textColor.b  </td><td> number                         </td><td> The blue component of the text color. </td></tr>
     *   <tr><td> textColor.a  </td><td> number                         </td><td> The alpha component of the text color. </td></tr>
     *   <tr><td> textScale    </td><td> number                         </td><td> The relative size of the text. 1 = 48pt, 0.5 = 24pt, etc. </td></tr>
     *   <tr><td> textAlignHor </td><td> number (integer)               </td><td> The horizontal text alignment mode: 0 = left (default), 1 = right, 2 = center. </td></tr>
     *   <tr><td> textAlignVer </td><td> number (integer)               </td><td> The vertical text alignment mode: 0 = top (default), 1 = bottom, 2 = middle. </td></tr>
     * </table>
     *
     * Internally, many window attributes are composed of tuples of 2 or 4 individual values.
     * E.g. all colors have a red, green, blue and alpha component, and positions have an x- and y-component.
     * Therefore, for many attributes there is a form with which you can set all components at once,
     * and several forms with which you can set a subset of the components.
     *
     * All coordinates are given with regards to the "virtual screen coordinate system",
     * which has a resolution of 640 * 480 pixels and is automatically scaled to the true resolution.
     *
     * Color components (red, green, blue and alpha) are always given in percent, that is, as fractional values between 0 and 1.
     *
     * \par Example:
     * @code
     *     function ConsoleFrame:OnInit()
     *         self:set("rect", 20, 20, 600, 440);
     *         self:set("backColor", 0.82, 0.49, 0.17, 0.2);
     *         self:set("borderWidth", 0.7);
     *         self:set("borderColor", 0.82, 0.49, 0.17, 1.0);
     *         self:set("text", "Hello!");
     *     end
     * @endcode
     *
     * @param attrib   The name of the window attribute to set. See the table above for a list of all window attributes.
     * @param value    The value(s) to set the window attribute(s) to.
     */
    set(string attrib, any value);

    /**
     * Returns the value of the specified window attribute.
     *
     * As multiple values can be returned at a time (as a tuple), this method works with all window attributes listed at the set() method,
     * except for @c "backMaterial" and @c "font", which are write-only.
     *
     * \par Example:
     * @code
     *     function RotatingWin:OnFrame()
     *         self:set("rotAngle", self:get("time")*50);
     *     end
     *
     *     function OtherWin:OnAction()
     *         -- Toggle the hor. text alignment of RotatingWin.
     *         local newAlign=RotatingWin:get("textAlignHor")+1;
     *         if (newAlign>2) then newAlign=0; end
     *
     *         RotatingWin:set("textAlignHor", newAlign);
     *     end
     * @endcode
     *
     * @param attrib   The name of the window attribute whose value should be returned. See set() for the list of supported attributes.
     *                 The only exceptions are @c "backMaterial" and @c "font", which are write-only.
     * @returns   The value of the desired window attribute in the appropriate type.
     */
    any get(string attrib);

    /**
     * Interpolates the value of the given window attribute from start value @c s to end value @c e over time @c t.
     *
     * \par Example:
     * @code
     *     function ButtonOK:OnMouseEnter()
     *         self:interpolate("rotAngle", 0, 360, 250);
     *         self:interpolate("textScale", 0.2, 0.25, 500);
     *         self:interpolate("pos.y", 300, 380, 750);
     *     end
     * @endcode
     *
     * @param attrib   The name of the window attribute to interpolate. Only attributes of type @c number are allowed / can be interpolated, see set() for a list.
     * @param s        The start value.
     * @param e        The end value.
     * @param t        The time to interpolate over, given in milliseconds.
     */
    interpolate(string attrib, number s, number e, number t);

    /// Sets the name of this window.
    /// The name of the window is normally specified when the window is created, so there is rarely a reason to call this method.
    /// Note that the window name should be unique among all windows in the GUI, so that the window can unambiguously be found by name.
    /// @param name   The new name for this window.
    SetName(string name);

    /// Returns the name of this window.
    /// The name is a string that identifies the window.
    /// It can be set with SetName(), but is usually assigned when the window is created.
    string GetName();

    /// Adds the given window to the children of this window.
    AddChild(window child);

    /// Removes the given window from the children of this window.
    RemoveChild(window child);


    /** @name Event Handler Callbacks
     *
     * @{
     */

    /// This method is called when the user presses the "action" button inside the window.
    /// This is normally the left mouse button, but could also be the appropriate joystick button etc.
    ///
    ///   - When the GuiSys calls this method, it also calls to the OnFocusLose() and OnFocusGain() methods,
    ///     if appropriate (that is, when the keyboard input focus changes as a result of the mouse click).
    OnAction();

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

    /// This method is called when this window gains the keyboard input focus.
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
    OnInit();

    /// This method is called if a key was pressed and the window has the keyboard input focus.
    ///
    /// @param Key   The integer number that represents the key that has been pressed.
    ///     See <a href="http://api.cafu.de/OpenGLWindow_8hpp_source.html#l00044" style="color: #4665A2;">OpenGLWindow.hpp</a>
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
    /// @returns The method must return @c true if it handled this key press, @c false otherwise.
    bool OnKeyPress(number Key);

    /// Like OnKeyPress(), but for key releases.
    bool OnKeyRelease(number Key);

    /// This method is called when a character event occurred and the window has the keyboard input focus.
    ///
    /// The difference to OnKeyPress() and OnKeyRelease is that these two just deals with plain keys,
    /// which is useful for example to work with the arrow keys.
    /// OnChar() however deals with entered \emph{characters}, which for example take the state of the SHIFT key into account,
    /// language specific keyboard settings and layout, etc. Therefore, OnChar() is the preferred choice for text input.
    ///
    ///   - Characters are represented as ASCII codes. That is, the integer values that @c ch may assume match the ASCII table.
    ///   - You can use the <tt>string.char(ch)</tt> Lua library function in order to obtain a string that consist
    ///     of the character that corresponds to the numeric value of @c ch.
    ///
    /// @param ch   The integer number that represents the character.
    /// @returns The method must return @c true if it handled this key press, @c false otherwise.
    bool OnChar(number ch);

    /// This method is called when the mouse cursor enters the rectangle of the window.
    ///
    /// \note This does currently not take rotation into account, i.e. it acts as if the rotation was always 0,
    /// even if the window rectangle is actually rotating.
    OnMouseEnter();

    /// This method is called when the mouse cursor leaves the rectangle of the window.
    ///
    /// \note This does currently not take rotation into account, i.e. it acts as if the rotation was always 0,
    /// even if the window rectangle is actually rotating.
    OnMouseLeave();

    /** @} */
};


/// @cppName{ModelWindowT}
class ModelWindowT : public WindowT
{
    public:

    SetModel();
    GetModelNrOfSqs();
    SetModelSequNr();
    SetModelPos();
    SetModelScale();
    SetModelAngles();
    SetCameraPos();
};


/// @cppName{ListBoxT}
class ListBoxT : public WindowT
{
    public:

    Clear();
    Append();
    Insert();
    GetNumRows();
    GetRowText();
    SetRowText();
    GetSelection();
    SetSelection();
    GetRowHeight();
    SetRowHeight();
    SetOddRowBgColor();
    SetEvenRowBgColor();
    SetRowTextColor();
    SetSelRowBgColor();
    SetSelRowTextColor();
};


/// @cppName{EditWindowT}
class EditWindowT : public WindowT
{
    public:

    set();
    GetTextCursorPos();
    SetTextCursorPos();
    SetTextCursorType();
    SetTextCursorRate();
    SetTextCursorColor();
};


/// @cppName{ChoiceT}
class ChoiceT : public WindowT
{
    public:

    Clear();
    Append();
    Insert();
    GetNumChoices();
    GetChoice();
    SetChoice();
    GetSelection();
    SetSelection();
};

/** @} */   // End of group GUI.
