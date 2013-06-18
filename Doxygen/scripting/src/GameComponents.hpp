namespace Game
{


/// This is the base class for the components that a game entity is composed/aggregated of.
/// Components are the basic building blocks of an entity: their composition defines
/// the properties, the behaviour, and thus virtually every aspect of the entity.
///
/// Components of this type are never instantiated directly, but always indirectly through one of the derived classes.
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentBaseT}
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


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */
};


/// This component adds the basics of the entity (its name and the "is shown?" flag).
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE %Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentBasicsT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentBasicsT}
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

    /** @} */


    public:

    /// The name of the entity. Entity names must be valid Lua script identifiers and unique among their siblings.
    /// @cppType{std::string}
    string Name;

    /// Is this entity currently shown?
    /// @cppType{bool}
    boolean Show;
};


/// This component adds a 3D model to its entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE %Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentModelT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentModelT}
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

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */


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


/// This component adds information about the position and orientation of the entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE %Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentTransformT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentTransformT}
class ComponentTransformT : public ComponentBaseT
{
    public:


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */


    public:

    /// The origin of the entity (in the coordinate system of its parent).
    /// @cppType{Vector3fT}
    tuple Origin;

    /// The orientation of the entity (in the coordinate system of its parent).
    /// @cppType{Vector3fT}
    tuple Orientation;
};


}   // namespace Game
