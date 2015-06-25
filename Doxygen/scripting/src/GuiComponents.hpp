namespace GuiSys
{


/// This is the base class for the components that a window is composed/aggregated of.
/// Components are the basic building blocks of a window: their composition defines
/// the properties, the behaviour, and thus virtually every aspect of the window.
///
/// Components of this type are never instantiated directly, but always indirectly through one of the derived classes.
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentBaseT}
class ComponentBaseT
{
    public:

    /// Returns the value of an attribute (a member variable) of this class.
    ///
    /// The variables of this class are also referred to as "Public Attributes" or "Member Data",
    /// and include the variables of the concrete, derived component class as well as those of its base classes.
    /// (However, there is currently only one base class, ComponentBaseT, which adds methods such as get() and set(),
    ///  but adds no additional variables to its derived classes.)
    ///
    /// At this time, it is not yet possible to write script code like this:
    /// \code{.lua}
    ///     local w       = border_component.Width    --// We're working on it, but at this time,
    ///     local r, g, b = border_component.Color    --// these two lines are not valid code.
    /// \endcode
    /// but instead, we have to write it like this:
    /// \code{.lua}
    ///     local w       = border_component:get("Width")
    ///     local r, g, b = border_component:get("Color")
    /// \endcode
    ///
    /// @param var_name   The name of the variable whose value is to be retrieved.
    /// @returns The value of the queried variable (possibly a tuple if the value itself is a tuple, see the text above for an example).
    any get(string var_name);

    /// Sets an attribute (a member variable) of this class to a new value.
    ///
    /// The variables of this class are also referred to as "Public Attributes" or "Member Data",
    /// and include the variables of the concrete, derived component class as well as those of its base classes.
    /// (However, there is currently only one base class, ComponentBaseT, which adds methods such as get() and set(),
    ///  but adds no additional variables to its derived classes.)
    ///
    /// At this time, it is not yet possible to write script code like this:
    /// \code{.lua}
    ///     border_component.Width = w          --// We're working on it, but at this time,
    ///     border_component.Color = r, g, b    --// these two lines are not valid code.
    /// \endcode
    /// but instead, we have to write it like this:
    /// \code{.lua}
    ///     border_component:set("Width", w)
    ///     border_component:set("Color", r, g, b)
    /// \endcode
    ///
    /// @param var_name    The name of the variable whose value is to be set.
    /// @param new_value   The new value that is assigned to the variable. Note that this can be a tuple if the value itself is a tuple as shown in the example above.
    set(string var_name, any new_value);

    /// Returns the result of `VarBaseT::GetExtraMessage()` for the given member variable.
    ///
    /// This is currently only used with ComponentModelT components, in order to learn why loading a specific model may have failed:
    /// \code{.lua}
    ///     model_component:set("Name", "Players/Trinity/Trinity.cmdl")
    ///
    ///     local msg = model_component:GetExtraMessage("Name")
    ///
    ///     if (msg != "") then
    ///         print("There was an error when loading this model: " .. msg)
    ///     end
    /// \endcode
    string GetExtraMessage(string var_name);

    /// Schedules a value for interpolation between a start and end value over a given period of time.
    /// Only variables that are floating-point numbers and variables that are tuples whose elements are
    /// floating-point numbers can be interpolated. (These are the variables whose underlying C++ type
    /// is `float`, `double`, `Vector2fT` or `Vector3fT`.)
    /// For variables that are tuples, you must append one of the suffixes `.x`, `.y`, `.z` to determine
    /// the first, second or third element for interpolation. Alternatively, the suffixes `.r`, `.g`, `.b`
    /// are more naturally used with color tuples, and work exactly alike.
    ///
    /// \par Example
    /// \code{.lua}
    ///     function ButtonOK:OnMouseEnter()
    ///         self:GetComponent("Text"):interpolate("Scale", 0.4, 0.45, 500)
    ///         self:GetComponent("Border"):interpolate("Color.r", 0.2, 1.0, 750)
    ///         self:GetComponent("Image"):interpolate("Alpha", 1.0, 0.0, 750)
    ///         self:GetTransform():interpolate("Pos.y", 10, 220, 700)
    ///     end
    /// \endcode
    ///
    /// \param var_name      The name of the window attribute to interpolate. Also see set() for details.
    /// \param start_value   The start value.
    /// \param end_value     The end value.
    /// \param time          The time in milliseconds to interpolate the variable from `start_value` to `end_value`.
    interpolate(string var_name, number start_value, number end_value, number time);
};


/// This component adds the basics of the window (its name and the "is shown?" flag).
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentBasicsT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentBasicsT}
class ComponentBasicsT : public ComponentBaseT
{
    public:


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /// This method is called when the value of the component's `Show` member has changed.
    /// The method has no parameters -- use `self:get("Show")` in order to learn whether the window
    /// has been shown or hidden.
    OnShow();

    /** @} */


    public:

    /// The name of the window. Window names must be valid Lua script identifiers and unique among their siblings.
    /// @cppType{std::string}
    string Name;

    /// Is this window currently shown?
    /// @cppType{bool}
    boolean Show;
};


/// This components adds a border to its window.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentBorderT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentBorderT}
class ComponentBorderT : public ComponentBaseT
{
    public:


    public:

    /// The width of the border.
    /// @cppType{float}
    number Width;

    /// The border color.
    /// @cppType{Vector3fT}
    tuple Color;

    /// The alpha component of the color.
    /// @cppType{float}
    number Alpha;
};


/// This components add the behaviour of a choice field to its window.
/// It requires that the window also has a text component, whose value it
/// updates according to user interaction to one of the available choices.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentChoiceT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentChoiceT}
class ComponentChoiceT : public ComponentBaseT
{
    public:

    /// An override of the base class method that also calls Sync().
    set(string var_name, any);

    /// Returns the currently selected item (or `nil` if no item is selected).
    string GetSelItem();


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /// This method is called when the choice's selection has changed.
    ///
    /// The selection may have changed as the result of a mouse click or a key press,
    /// whereas calling `self:set("Selection", ...)` does *not* trigger this event.
    ///
    /// Use `self:GetSelItem()` or `self:get("Selection")` in order to learn which item has recently been selected.
    OnSelectionChanged();

    /** @} */


    public:

    /// The list of available choices.
    /// @cppType{ArrayT<std::string>}
    table Choices;

    /// The index number of the currently selected choice, where 1 corresponds to the first choice (as per Lua convention). Use 0 for "no selection".
    /// @cppType{unsigned int}
    number Selection;
};


/// This component adds an image to its window.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentImageT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentImageT}
class ComponentImageT : public ComponentBaseT
{
    public:


    public:

    /// The name of the image material.
    /// @cppType{std::string}
    string Material;

    /// The color with which the image is tinted.
    /// @cppType{Vector3fT}
    tuple Color;

    /// The alpha component of the color.
    /// @cppType{float}
    number Alpha;
};


/// This components turns its window into a list-box control.
/// It requires that in the same window a text component is available where the aspects of text rendering are
/// configured (but that has empty text contents itself).
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentListBoxT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentListBoxT}
class ComponentListBoxT : public ComponentBaseT
{
    public:

    /// Returns the currently selected item (or nil if no item is selected).
    string GetSelItem();


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /// This method is called when the list box's selection has changed.
    ///
    /// The selection may have changed as the result of a mouse click or a key press,
    /// whereas calling `self:set("Selection", ...)` does *not* trigger this event.
    ///
    /// Use `self:GetSelItem()` or `self:get("Selection")` in order to learn which item has recently been selected.
    OnSelectionChanged();

    /** @} */


    public:

    /// The list of available items.
    /// @cppType{ArrayT<std::string>}
    table Items;

    /// The index number of the currently selected item, where 1 corresponds to the first item (as per Lua convention). Use 0 for "no selection".
    /// @cppType{unsigned int}
    number Selection;

    /// The background color for odd rows.
    /// @cppType{Vector3fT}
    tuple BgColorOdd;

    /// The background alpha for odd rows.
    /// @cppType{float}
    number BgAlphaOdd;

    /// The background color for even rows.
    /// @cppType{Vector3fT}
    tuple BgColorEven;

    /// The background alpha for even rows.
    /// @cppType{float}
    number BgAlphaEven;

    /// The background color for selected rows.
    /// @cppType{Vector3fT}
    tuple BgColorSel;

    /// The background alpha for selected rows.
    /// @cppType{float}
    number BgAlphaSel;

    /// The foreground color for selected rows.
    /// @cppType{Vector3fT}
    tuple TextColorSel;

    /// The foreground alpha for selected rows.
    /// @cppType{float}
    number TextAlphaSel;
};


/// This component adds a 3D model to its window.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentModelT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentModelT}
class ComponentModelT : public ComponentBaseT
{
    public:

    /// Returns the number of animation sequences in this model.
    number GetNumAnims();

    /// Sets a new animation sequence for the pose of this model.
    /// Optionally, there is a blending from the previous sequence over a given time.
    /// Also optionally, the "force loop" flag for the new sequence can be set.
    /// For example: `SetAnim(8, 3.0, true)`
    number SetAnim(number anim, number blend_time=0.0, boolean force_loop=false);

    /// Returns the number of skins in this model.
    number GetNumSkins();


    public:

    /// The file name of the model.
    /// @cppType{std::string}
    string Name;

    /// The animation sequence number of the model.
    /// @cppType{int}
    number Animation;

    /// The skin used for rendering the model.
    /// @cppType{int}
    number Skin;

    /// The position of the model in world space.
    /// @cppType{Vector3fT}
    tuple Pos;

    /// The scale factor applied to the model coordinates when converted to world space.
    /// @cppType{float}
    number Scale;

    /// The angles around the axes that determine the orientation of the model in world space.
    /// @cppType{Vector3fT}
    tuple Angles;

    /// The position of the camera in world space.
    /// @cppType{Vector3fT}
    tuple CameraPos;
};


/// With this component, the user can edit the text in a sibling text component.
/// The component requires that the window also has a text component, whose value it updates according to
/// user edits.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentTextEditT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentTextEditT}
class ComponentTextEditT : public ComponentBaseT
{
    public:

    /// Sets the given text in the related Text sibling component and moves the cursor position to its end.
    SetText(string text);


    public:

    /// The character position of the text cursor in the text. Valid values are 0 to Text.length().
    /// @cppType{unsigned int}
    number CursorPos;

    /// The type of the text cursor. 0 is a vertical bar cursor '|', 1 is an underline cursor '_'. Any other values default to the '|' cursor type.
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>0</td><td> | </td></tr>
    /// <tr><td>1</td><td> _ </td></tr>
    /// </table>
    ///
    /// @cppType{int}
    number CursorType;

    /// The rate in seconds at which the text cursor completes one blink cycle (on/off).
    /// @cppType{float}
    number CursorRate;

    /// The color of the text cursor.
    /// @cppType{Vector3fT}
    tuple CursorColor;

    /// The alpha component of the color.
    /// @cppType{float}
    number CursorAlpha;
};


/// This components adds text to its window.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentTextT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentTextT}
class ComponentTextT : public ComponentBaseT
{
    public:


    public:

    /// The text to show in this window.
    /// @cppType{std::string}
    string Text;

    /// The name of the font.
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>Fonts/Arial</td><td>Fonts/Arial</td></tr>
    /// <tr><td>Fonts/Impact</td><td>Fonts/Impact</td></tr>
    /// </table>
    ///
    /// @cppType{std::string}
    string Font;

    /// The scale that is applied for rendering the text.
    /// @cppType{float}
    number Scale;

    /// Padding between text and window rectangle.
    /// @cppType{Vector2fT}
    tuple Padding;

    /// The text color.
    /// @cppType{Vector3fT}
    tuple Color;

    /// The alpha component of the color.
    /// @cppType{float}
    number Alpha;

    /// How the text is aligned horizontally (left, center, right).
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>-1</td><td>left</td></tr>
    /// <tr><td>0</td><td>center</td></tr>
    /// <tr><td>1</td><td>right</td></tr>
    /// </table>
    ///
    /// @cppType{int}
    number horAlign;

    /// How the text is aligned vertically (top, middle, bottom).
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>-1</td><td>top</td></tr>
    /// <tr><td>0</td><td>middle</td></tr>
    /// <tr><td>1</td><td>bottom</td></tr>
    /// </table>
    ///
    /// @cppType{int}
    number verAlign;
};


/// This component adds information about the position and size of the window.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE GUI Editor are instantiated automatically), use GuiT::new():
/// \code{.lua}
///     local comp = gui:new("ComponentTransformT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GuiSys,ComponentTransformT}
class ComponentTransformT : public ComponentBaseT
{
    public:


    public:

    /// The position of the top-left corner of the window, relative to its parent.
    /// @cppType{Vector2fT}
    tuple Pos;

    /// The size of the window.
    /// @cppType{Vector2fT}
    tuple Size;

    /// The angle in degrees by how much this entire window is rotated.
    /// @cppType{float}
    number Rotation;
};


}   // namespace GuiSys
