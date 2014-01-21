namespace GameSys
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

    /// Returns the entity that this component is a part of (or `nil` if the component is currently "stand-alone", not a part of any entity).
    EntityT GetEntity();

    /// Registers the given attribute (a member variable) of this class for interpolation over client frames in order
    /// to bridge the larger intervals between server frames.
    /// This method only works with variables whose related C++ type is `float`, `double`, `Vector2fT` or `Vector3fT`,
    /// and is typically used with ComponentTransform::Origin and ComponentTransform::Orientation. For example:
    /// \code{.lua}
    ///     Butterfly.Trafo:InitClientApprox("Origin")
    ///     Butterfly.Trafo:InitClientApprox("Orientation")
    /// \endcode
    ///
    InitClientApprox(string VarName);


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */
};


/// This component adds the basics of the entity (its name and the "is shown?" and "is static?" flags).
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
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

    /// Are the map primitives of this entity fixed and immovable, never moving around in the game world?
    /// @cppType{bool}
    boolean Static;
};


/// This component adds a collision model to its entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentCollisionModelT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentCollisionModelT}
class ComponentCollisionModelT : public ComponentBaseT
{
    public:

    // WARNING: Mismatching method documentation strings encountered!
    SetBoundingBox();


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */


    public:

    /// The file name of the collision model.
    /// @cppType{std::string}
    string Name;
};


/// The common base class for light source components.
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentLightT}
class ComponentLightT : public ComponentBaseT
{
    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */
};


/// This component adds a 3D model to its entity.
/// Models can be used to add geometric detail to a map. Some models also have ready-made
/// "GUI fixtures" where scripted GUIs can be attached that players can interact with.
/// Use the CaWE Model Editor in order to import mesh and animation data for models, and
/// to prepare them for use in game maps.
///
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
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
    SetAnim(number anim, number blend_time=0.0, boolean force_loop=false);

    /// Returns the number of skins in this model.
    number GetNumSkins();

    /// Returns the (first) GUI of this model.
    GuiImplT GetGui();


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */


    public:

    /// Whether the model is currently shown (useful with scripts).
    /// @cppType{bool}
    boolean Show;

    /// The file name of the model.
    /// @cppType{std::string}
    string Name;

    /// The animation sequence number of the model.
    /// @cppType{int}
    number Animation;

    /// The skin used for rendering the model.
    /// @cppType{int}
    number Skin;

    /// The scale factor applied to the model coordinates when converted to world space.
    /// @cppType{float}
    number Scale;

    /// The file name of the GUI to be used with the models GUI fixtures (if there are any).
    /// @cppType{std::string}
    string Gui;
};


/// This component adds a particle system to its entity.
/// The particle system is obsolete though: This is just a quick and dirty port
/// of the particle system in the old game system to the new component system.
/// Both its interface and its implementation need a thorough overhaul.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentParticleSystemOldT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentParticleSystemOldT}
class ComponentParticleSystemOldT : public ComponentBaseT
{
    public:

    /// Emits a new particle in this particle system.
    EmitParticle();


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */


    public:

    /// The type of the particles emitted by this system.
    /// @cppType{std::string}
    string Type;
};


/// This component implements human player physics for its entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentPlayerPhysicsT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentPlayerPhysicsT}
class ComponentPlayerPhysicsT : public ComponentBaseT
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

    /// The velocity of the entity.
    /// @cppType{Vector3dT}
    tuple Velocity;

    /// The bounding box of the entity (relative to the origin).
    /// @cppType{BoundingBox3dT}
    tuple Dimensions;
};


/// This component adds a dynamic point light source to its entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentPointLightT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentPointLightT}
class ComponentPointLightT : public ComponentLightT
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

    /// Switch the light source on or off.
    /// @cppType{bool}
    boolean On;

    /// The color of the emitted light.
    /// @cppType{Vector3fT}
    tuple Color;

    /// The distance up to which the light may be perceived.
    /// @cppType{float}
    number Radius;

    /// The type of the shadows that are cast by this light source.
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>0</td><td>none</td></tr>
    /// <tr><td>1</td><td>Stencil (hard)</td></tr>
    /// </table>
    ///
    /// @cppType{int}
    number ShadowType;
};


/// This component adds a radiosity point light source to its entity.
/// Radiosity lights are preprocessed and their effects are baked into lightmaps.
/// Consequently, radiosity lights are static, and script-updating them at run-time has no effect.
/// However, their results look very natural and their performance at run-time is very good.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentRadiosityLightT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentRadiosityLightT}
class ComponentRadiosityLightT : public ComponentLightT
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

    /// The color of the emitted light.
    /// @cppType{Vector3fT}
    tuple Color;

    /// The intensity (per color-component) of the light source in watt per steradian (W/sr).
    /// @cppType{float}
    number Intensity;

    /// The size of the cone in which light is emitted (in degrees; use 360.0 for a spherical light).
    /// @cppType{float}
    number ConeAngle;
};


/// This component runs custom Lua script code, implementing the behaviour of the entity in the game world.
/// The script code can be loaded from a separate file, or it can be entered and kept directly in the component.
///
/// Keeping the script code in a separate file is useful when it is re-used with several entity instances
/// or in several maps.
/// Keeping the script code directly in the component is useful for short scripts that are unique to a single
/// map and entity instance.
/// Note that both options can also be combined: The script code from a file is loaded first, and immediate
/// code can be used to augment it (for example to "configure" it).
///
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentScriptT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentScriptT}
class ComponentScriptT : public ComponentBaseT
{
    public:

    /// This functions sets the number of event types that can be used with PostEvent().
    InitEventTypes(number n);

    /// This function is used for posting an event of the given type.
    /// The event is automatically sent from the entity instance on the server to the entity instances
    /// on the clients, and causes a matching call to the ProcessEvent() callback there.
    /// The meaning of the event type is up to the script code that implements ProcessEvent().
    /// Note that events are fully predictable: they work well even in the presence of client prediction.
    PostEvent(number EventType);

    /// Inflicts damage to nearby entities.
    ///
    /// This function finds all entities that are close to this (within a distance of `OuterRadius`,
    /// excluding this entity itself) and have a ComponentScriptT component.
    /// It then calls that script component's TakeDamage() method in order to apply the damage accordingly.
    ///
    /// @param Damage        The maximum damage to apply to nearby entities.
    /// @param InnerRadius   Entities that are closer than `InnerRaduis` are damaged by the full amount of `Damage`.
    /// @param OuterRadius   Entities that are farther than `OuterRadius` are not damaged at all.
    ///
    /// The damage that is inflicted to entities that are between `InnerRaduis` and `OuterRadius`
    /// is linearly scaled from `Damage` to 0. Entities must implement the TakeDamage() script callback
    /// method in order to actually process the inflicted damage.
    DamageAll(number Damage, number InnerRadius, number OuterRadius);


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */


    public:

    /// The file to load the Lua script code from.
    /// @cppType{std::string}
    string Name;

    /// Immediate Lua script code to use with this entity.
    /// @cppType{std::string}
    string ScriptCode;
};


/// This component adds 3D sound output to its entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentSoundT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentSoundT}
class ComponentSoundT : public ComponentBaseT
{
    public:

    // WARNING: Mismatching method documentation strings encountered!
    Play();


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /** @} */


    public:

    /// The name of the sound shader or sound file to play.
    /// @cppType{std::string}
    string Name;

    /// Whether the sound is played automatically in interval-spaced loops.
    /// If `false`, playbacks of the sound must be triggered by explicit calls to the Play() method.
    /// @cppType{bool}
    boolean AutoPlay;

    /// If `m_AutoPlay` is `true`, this is the time in seconds between successive playbacks of the sound.
    /// @cppType{float}
    number Interval;
};


/// This component adds information about the position and orientation of the entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentTransformT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentTransformT}
class ComponentTransformT : public ComponentBaseT
{
    public:

    /// Returns the orientation of this entity as a tuple of three angles, measured in degrees:
    ///   - heading (yaw),
    ///   - pitch,
    ///   - bank (roll).
    tuple GetAngles();

    /// Sets the orientation of this entity from a set of three angles, measured in degrees:
    ///   - heading (yaw),
    ///   - pitch,
    ///   - bank (roll).
    SetAngles(number heading, number pitch=0.0, number bank=0.0);

    /// Returns the x-axis of the local coordinate system of this entity.
    /// The local coordinate system expresses the orientation of the entity.
    /// It is relative to the entity's parent.
    tuple GetAxisX();

    /// Returns the y-axis of the local coordinate system of this entity.
    /// The local coordinate system expresses the orientation of the entity.
    /// It is relative to the entity's parent.
    tuple GetAxisY();

    /// Returns the z-axis of the local coordinate system of this entity.
    /// The local coordinate system expresses the orientation of the entity.
    /// It is relative to the entity's parent.
    tuple GetAxisZ();


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


}   // namespace GameSys
