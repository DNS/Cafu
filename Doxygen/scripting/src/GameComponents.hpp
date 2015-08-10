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

    /// This method is called for each component of each entity as the last step of
    /// initializing a newly loaded map.
    OnInit();

    /// This method is called for each component of each entity before the client renders the
    /// next frame.
    ///
    /// The method is a great opportunity to add eye candy effects to a game world that are
    /// not synchronized over the network. (Such effects are not necessarily the same for every
    /// player and thus must not be relevant for the gameplay.)
    ///
    /// As it is called for each component of each entity with the client's framerate, the
    /// implementation must be very careful to keep performance implications light.
    ///
    /// While the implementation can generally modify any variable of any component, it is
    /// important to note that for any such variable, InitClientApprox() should be called
    /// (once beforehand).
    /// Even if the implementation is not interested in the so activated interpolation, at this
    /// time we rely on InitClientApprox() to restore the previous, original value after the
    /// client frame has been rendered. The calls to InitClientApprox() make sure that any
    /// changes that the implementation of OnClientFrame() made are properly cleaned up so that
    /// the *next* call to OnClientFrame() re-starts with the original, unmodified (but
    /// properly predicted and possibly interpolated) values.
    OnClientFrame(number t);

    /** @} */
};


/// This component adds the basic details of the entity (its name, the "is static?" flag, Map Editor data).
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

    /// The name of the entity. Entity names must be valid Lua script identifiers and unique among their siblings.
    /// @cppType{std::string}
    string Name;

    /// Are the map primitives of this entity fixed and immovable, never moving around in the game world?
    /// @cppType{bool}
    boolean Static;
};


/// This component represents a weapon that a player can pick up and use.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentCarriedWeaponT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentCarriedWeaponT}
///
/// @see @ref CarriedWeaponsOverview
class ComponentCarriedWeaponT : public ComponentBaseT
{
    public:


    public:

    /** @name Event Handlers (Callbacks)
     *
     * See the \ref eventhandlers overview page for additional information about the methods in this group.
     *
     * @{
     */

    /// This method is called in order to learn if this weapon is currently idle.
    /// The implementation usually determines the idle state of the weapon by the animation sequence
    /// that the related 1st-person weapon model is currently playing.
    ///
    /// @see @ref CarriedWeaponsOverview
    boolean IsIdle();

    /// This method is called in order to let this weapon know that it is drawn.
    /// It is reponsible for setting up the entity's 1st-person weapon model
    /// for drawing this weapon. This possibly includes setting up the 1st-person weapon model's
    /// ComponentModelT::OnSequenceWrap_Sv() callback, which must be implemented to "operate"
    /// this weapon.
    ///
    /// @see @ref CarriedWeaponsOverview
    Draw();

    /// This method is called in order to have this weapon holstered.
    /// It is reponsible for setting up the entity's 1st-person weapon model
    /// for holstering this weapon. It must return `true` if successful, or `false` to indicate
    /// that holstering is not possible, e.g. because a holstering sequence is not available.
    ///
    /// @see @ref CarriedWeaponsOverview
    boolean Holster();

    /// This method is called in order to have this weapon emit primary fire.
    FirePrimary(boolean ThinkingOnServerSide);

    /// This method is called in order to have this weapon emit secondary fire.
    FireSecondary(boolean ThinkingOnServerSide);

    /// This method is called in order to have the weapon pre-cache its resources.
    PreCache();

    /// This method is called when the player walks over an instance of this weapon in the world.
    /// Its implementation is reponsible for picking up the weapon and for re-stocking the weapon
    /// and the player's inventory with the related ammunition.
    /// (This method is usually not called directly from Cafu's C++ code, but rather from other
    /// script code.)
    ///
    /// @see @ref CarriedWeaponsOverview
    bool PickedUp();

    /// This method is called by the HUD GUI in order to learn which cross-hair should currently be shown.
    tuple GetCrosshairInfo();

    /// This method is called by the HUD GUI in order to learn which info should currently be shown in the "ammo" field.
    string GetAmmoString();

    /** @} */


    public:

    /// A short informational name for this weapon. Used for reference e.g. in the Map Editor, in log output, or in script code (therefore, changing it for existing weapons may require a review of the related script code).
    /// @cppType{std::string}
    string Label;

    /// Is this weapon available to the player? Normally `false` when the player spawns. Switched to `true` when the player picks up the weapon for the first time, whereupon it can be selected and drawn.
    /// @cppType{bool}
    boolean IsAvail;

    /// The filename of the script that implements the behaviour of this weapon.
    /// @cppType{std::string}
    string Script;

    /// The current amount of ammo for the primary fire of this weapon.
    /// @cppType{uint16_t}
    number PrimaryAmmo;

    /// The maximum amount of ammo for the primary fire of this weapon.
    /// @cppType{uint16_t}
    number MaxPrimaryAmmo;

    /// The current amount of ammo for the secondary fire of this weapon.
    /// @cppType{uint16_t}
    number SecondaryAmmo;

    /// The maximum amount of ammo for the secondary fire of this weapon.
    /// @cppType{uint16_t}
    number MaxSecondaryAmmo;
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

    /// Sets the given bounding-box as the collision model.
    /// Instead of loading a collision model from a file, a script can call this method
    /// to set a bounding-box with the given dimensions as the collision model.
    SetBoundingBox(number min_x, number min_y, number min_z, number max_x, number max_y, number max_z, string MatName);


    public:

    /// The file name of the collision model.
    /// @cppType{std::string}
    string Name;

    /// If true, the orientation of the entity does not affect the orientation of the collision model.
    /// This is used with players, monsters and other NPCs whose world-space collision model must not
    /// change when they rotate (in order to not get them accidentally stuck in nearby walls).
    /// @cppType{bool}
    boolean IgnoreOrient;
};


/// Entities with this component are associated with a client connection
/// at whose end is a human player who provides input to control the entity.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentHumanPlayerT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentHumanPlayerT}
class ComponentHumanPlayerT : public ComponentBaseT
{
    public:

    /// Returns the ComponentCarriedWeaponT component of the currently active weapon,
    /// or `nil` if currently no weapon is active.
    GetActiveWeapon();

    /// This method initiates the holstering of the currently active weapon and the subsequent drawing
    /// of the given weapon.
    ///
    /// If the current weapon is unknown or not available to the player (e.g. because it has never been picked up),
    /// or if it turns out that the weapon does not support holstering (e.g. because there is no holstering
    /// sequence available), the holstering is skipped and the next weapon is drawn immediately.
    /// If the current weapon is fine but is not idle at the time that this method is called (e.g. reloading
    /// or firing), the call is *ignored*, that is, the weapon is *not* changed.
    ///
    /// @param NextWeapon   This can be the index number into the CarriedWeapon components of this entity, starting at 1.
    ///                     Use 0 to select "no" weapon.
    ///                     Alternatively, pass an instance of the carried weapon that is to be selected next.
    /// @param Force        If `true`, forces the drawing of the next weapon immediately, ignoring the idle
    ///                     state and holstering sequence of the current weapon. This is normally only used
    ///                     if, for example, the last hand grenade has been thrown and bare-handed animation
    ///                     sequences for idle and holster are not available.
    ///
    /// @see @ref CarriedWeaponsOverview
    SelectWeapon(any NextWeapon, bool Force);

    /// This method draws the next weapon as previously prepared by SelectWeapon().
    ///
    /// It is intended to be called at the end of the holstering sequence of the previous weapon, either
    /// directly from SelectWeapon() when it found that holstering can entirely be skipped, or indirectly
    /// when SelectWeapon() calls the previous weapon's `Holster()` callback, the end of the holster
    /// sequence is detected in the `OnSequenceWrap_Sv()` script callback, and its implementation in turn
    /// calls this method.
    ///
    /// @see @ref CarriedWeaponsOverview
    SelectNextWeapon();

    /// Traces a ray through the world and causes damage to the hit entity (if any).
    ///
    /// This method can be used to implement the "fire" action of weapons that cause instantaneous damage,
    /// such as pistols, guns, rifles, etc.
    /// The ray is traced from the camera's origin along the camera's view vector, which can be randomly
    /// scattered (used to simulate inaccurate human aiming) by the given parameter `Random`.
    /// If an entity is hit, its TakeDamage() method is called with the human player as the originator and
    /// the amount of damage as given by parameter `Damage`.
    /// @param Damage   The damage to inflict to a possibly hit entity.
    /// @param Random   The maximum amount of random scatter to apply to the traced ray.
    FireRay(number Damage, number Random = 0.0);

    /// Returns a pseudo-random number.
    ///
    /// If `n` is 0, 1, or absent (`nil`), this method returns a pseudo-random number in range `[0.0, 1.0]` (inclusive).
    /// Otherwise, a pseudo-random *integer* in range `0 ... n-1` is returned.
    ///
    /// The important aspect of this method is that it returns pseudo-random numbers that are reproducible in the
    /// context of the "client prediction" feature of the Cafu Engine. All random numbers that are used in human
    /// player code must be obtained from this method.
    GetRandom(number n);

    /// An auxiliary method for spawning entities for thrown hand grenades, thrown face-huggers, launched AR grenades,
    /// or launched rockets (RPGs).
    ///
    /// This is only an auxiliary method -- it should in fact be removed and entirely be implemented in Lua instead!
    SpawnWeaponChild(string EntityName);

    /// An auxiliary method for spawning new particles.
    /// This is only a clumsy auxiliary method -- the entire particle system needs a thorough revision instead!
    RegisterParticle(number Type);


    public:

    /// The name that the player chose for himself.
    /// @cppType{std::string}
    string PlayerName;

    /// Keeps track of the next random number that is returned by the GetRandom() method.
    /// @cppType{uint16_t}
    number RandomCount;

    /// For the player's main state machine, e.g. spectator, dead, alive, ...
    /// @cppType{uint8_t}
    number State;

    /// Health.
    /// @cppType{uint8_t}
    number Health;

    /// Armor.
    /// @cppType{uint8_t}
    number Armor;

    /// Frags.
    /// @cppType{uint8_t}
    number Frags;

    /// The index number into the CarriedWeapon components of this entity, starting at 1, indicating the currently active weapon. The weapon must also be available (have been picked up) before the player can use it. A value of 0 means that "no" weapon is currently active.
    /// @cppType{uint8_t}
    number ActiveWeaponNr;

    /// The next weapon to be drawn by SelectNextWeapon(). Like ActiveWeaponNr, this is an index number into the CarriedWeapon components of this entity, starting at 1. A value of 0 means "none".
    /// @cppType{uint8_t}
    number NextWeaponNr;

    /// The progress of one "head swaying" cycle in state FrozenSpectator.
    /// @cppType{float}
    number HeadSway;
};


/// This component keeps an inventory count for an arbitrary set of items.
/// An item can be anything that can be described with a string.
/// Contrary to other components, an inventory is flexible regarding the "kind" and number
/// of items that it keeps the counts for. However, it is focused on being used by script code;
/// it is not possible to inspect and edit the contained items in the Map Editor at this time.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentInventoryT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentInventoryT}
class ComponentInventoryT : public ComponentBaseT
{
    public:

    /// Returns the inventory count for the specified item.
    /// @param item_name   The name of the item to return the count for.
    any get(string item_name);

    /// Sets the inventory count for the specified item.
    /// @param item_name    The name of the item to set the count for.
    /// @param item_count   The new inventory count to set.
    set(string item_name, number item_count);

    /// Changes the inventory count of the specified item by the given amount.
    /// The amount can be positive to increase the inventory count or negative to decrease it.
    /// The resulting inventory count is clamped to the maximum for the item, if such a maximum is set.
    /// It is also clamped to 0 (for negative amounts).
    /// The function returns `true` if the resulting inventory count was clamped on either boundary,
    /// or `false` if no clamping was applied.
    /// @param item_name   The name of the item whose count is to be changed.
    /// @param amount      The amount by which the inventory count is changed.
    Add(string item_name, number amount);

    /// Checks if the inventory count of the specified item is at the item's maximum.
    /// Returns `true` if the inventory count of the specified item is equal to (or even exceeds) its defined maximum.
    /// Returns `false` if no maximum is defined or if the inventory count is below the defined value.@param item_name   The name of the item to check the count for.
    CheckMax(string item_name);
};


/// The common base class for light source components.
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentLightT}
class ComponentLightT : public ComponentBaseT
{
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

    /// This method is called when a new animation sequence number is set for this model.
    tuple OnAnimationChange(int AnimNr);

    /// This method is called when playing the model's current animation sequence "wraps".
    OnSequenceWrap_Sv();

    /// This method is called when playing the model's current animation sequence "wraps".
    OnSequenceWrap();

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

    /// Is this model a submodel of another model? If set, the pose of this model is aligned with the first "non-submodel" in the entity.
    /// @cppType{bool}
    boolean IsSubmodel;

    /// Is this a 1st-person view model? If `true`, the model is rendered if the world is rendered from *this* entity's perspective. If `false`, the model is rendered when seen from the outside, i.e. in everybody else's view. The default is `false`, because `true` is normally only used with the human player's 1st-person carried weapon models.
    /// @cppType{bool}
    boolean Is1stPerson;
};


/// This component controls the movement of one or more entities and implements the related effects.
///
/// The component can handle a single entity, e.g. a moving platform or a lift, or several entities
/// that act together as a team, e.g. the wings of a door.
///
/// This component works in concert with a Script component, which must be present in the same entity.
/// The Mover component queries the Script component for the desired spatial transformation of each
/// team member, and notifies it whenever the movement is blocked by another entity (e.g. a player).
/// It also implements the appropriate effects on other entities, e.g. their being pushed by moving
/// parts, or their riding on top of them.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentMoverT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentMoverT}
class ComponentMoverT : public ComponentBaseT
{
    public:

    /// This is the main method of this component: [...]
    /// @param FrameTime   The time across which the parts are moved.
    HandleMove(number FrameTime);


    public:

    /// The time in seconds that it takes to move each part from one endpoint to the other.
    /// @cppType{float}
    number moveDuration;

    /// Describes the mover's behavior when it is activated at the "dest" position.
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>0</td><td>move home</td></tr>
    /// <tr><td>1</td><td>reset timeout</td></tr>
    /// <tr><td>2</td><td>ignore</td></tr>
    /// </table>
    ///
    /// @cppType{int}
    number destActivated;

    /// The timeout in seconds after which the parts move back to their "home" position. A negative value to disables the timeout.
    /// @cppType{float}
    number destTimeout;

    /// Describes the mover's behavior regarding other entities.
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>0</td><td>ignore</td></tr>
    /// <tr><td>1</td><td>cannot push</td></tr>
    /// <tr><td>2</td><td>can push</td></tr>
    /// <tr><td>3</td><td>can force-push</td></tr>
    /// </table>
    ///
    /// @cppType{int}
    number otherEntities;

    /// Describes the base function that is used to compute the mover's trajectory.
    ///
    /// @par Typical values:
    /// <table>
    /// <tr><th>Value</th><th>Description</th></tr>
    /// <tr><td>0</td><td>linear</td></tr>
    /// <tr><td>1</td><td>sine</td></tr>
    /// </table>
    ///
    /// @cppType{int}
    number trajFunc;

    /// The exponent that is applied to `trajFunc`.
    /// @cppType{float}
    number trajExp;
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

    /// The type of the particles emitted by this system.
    /// @cppType{std::string}
    string Type;
};


/// This component includes the body of this entity in the dynamic simulation of physics.
///
/// Without this component, the entity is either *static* (it doesn't move at all), *kinematic*
/// (it is moved by script or program code), or it doesn't participate in physics computations
/// at all.
///
/// With this component, the entity's body is subject to gravity, impulses, and generally to
/// the dynamic simulation of physics effects in the game world.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentPhysicsT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentPhysicsT}
class ComponentPhysicsT : public ComponentBaseT
{
    public:

    /// This method applies an impulse at the center of the entity's body.
    /// The impulse is applied at the center of the body, so that it changes the body's
    /// linear velocity, but not its torque.
    ApplyImpulse(Vector3T Impulse);

    /// This method applies an off-center impulse to the entity's body.
    /// The impulse is applied at the center of the body, offset by `rel_pos`,
    /// changing the linear velocity and the body's torque appropriately.
    ApplyImpulse(Vector3T Impulse, Vector3T rel_pos);

    /// This method sets the gravity vector for this object, in m/s^2.
    /// The default gravity vector is `(0, 0, -9.81)`.
    SetGravity(number gx, number gy, number gz);


    public:

    /// The mass of the entity's body, in kilograms [kg].
    /// @cppType{float}
    number Mass;
};


/// This component implements human player physics for its entity.
/// It updates the entity's origin according to the laws of simple physics
/// that are appropriate for player movement.
/// The component does not act on its own in a server's Think() step, but is
/// only a helper to other C++ or script code that must drive it explicitly.
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

    /// This is the main method of this component: It advances the entity's origin
    /// according to the laws of simple physics and the given state and parameters.
    /// Other C++ or script code of the entity typically calls this method on each
    /// clock-tick (frame) of the server.
    /// @param FrameTime       The time across which the entity is to be advanced.
    /// @param WishVelocity    The desired velocity of the entity as per user input.
    /// @param WishVelLadder   The desired velocity on a ladder as per user input.
    /// @param WishJump        Does the user want the entity to jump?
    MoveHuman(number FrameTime, Vector3T WishVelocity, Vector3T WishVelLadder, bool WishJump);


    public:

    /// The current velocity of the entity.
    /// @cppType{Vector3dT}
    tuple Velocity;

    /// The bounding box of the entity (relative to the origin).
    /// @cppType{BoundingBox3dT}
    tuple Dimensions;

    /// The maximum height that the entity can climb in one step.
    /// @cppType{double}
    number StepHeight;

    /// Only jump if the jump key was *not* pressed in the previous frame.
    /// @cppType{bool}
    boolean OldWishJump;
};


/// This component marks its entity as possible spawn point for human players
/// that begin a single-player level or join a multi-player game.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentPlayerStartT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentPlayerStartT}
class ComponentPlayerStartT : public ComponentBaseT
{
    public:


    public:

    /// If checked, players can be spawned here in single-player games.
    /// @cppType{bool}
    boolean SinglePlayer;

    /// If checked, players can be spawned here in multi-player games.
    /// @cppType{bool}
    boolean MultiPlayer;
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

    /// This method is called when another entity wants to prompt us to become active.
    /// Note that this method is usually not called directly from Cafu's C++ code, but rather
    /// from other script code, e.g. from GUIs whose button has been pressed.
    OnActivate(EntityT Other);

    /// This method is called when another entity moves into this entity's trigger volume.
    OnTrigger(EntityT Other);

    /// This method is called on the client in order to process and react to events.
    ProcessEvent(int EventType, int EventCount);

    /// The server calls this method on each server clock tick, in order to advance the world
    /// to the next server frame.
    Think(number FrameTime);

    /// This method is called when there also is a ComponentMoverT component in the entity.
    /// The mover calls this method in order to learn which of its part to move where over
    /// the given frame time.
    tuple GetMove(int PartNr, number FrameTime);

    /// This method is called when the player has pressed a button to change the weapon.
    /// @param GroupNr   The number of the weapon group from which the next weapon is to be drawn.
    ChangeWeapon(int GroupNr);

    /// This method is called when another entity caused damage to this entity.
    TakeDamage(EntityT Other, number Amount, number DirX, number DirY, number DirZ);

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

    /// This method plays the sound once.
    Play();


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


/// This component connects its entity to another.
/// It is used by Script or GUI (Model) components in order to learn which other entity
/// is related and should possibly be acted upon. For example, Target components are
/// often used to let generic "open door" GUIs know which door they should actually open.
///
/// Note that the variables of this class (also referred to as "Public Attributes" or "Member Data")
/// must be used with the get() and set() methods at this time -- see get() and set() for details.
///
/// If you would like to create a new component of this type explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local comp = world:new("ComponentTargetT")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,ComponentTargetT}
class ComponentTargetT : public ComponentBaseT
{
    public:


    public:

    /// The name of another entity that scripts and GUIs should act upon.
    /// @cppType{std::string}
    string Target;
};


/// This component adds information about the position and orientation of its entity.
/// Positions and orientations can be measured relative to several distinct spaces:
///
/// world-space
///   : The global and "absolute" coordinate space that also exists when nothing else does.
///
/// entity-space
///   : The local coordinate system of the entity. It is defined by the entity's transform component relative
///     to the entity's parent-space. The term "model-space" can be used synonymously with "entity-space".
///
/// parent-space
///   : The entity-space of an entity's parent.
///     If an entity has no parent entity, this is the same as world-space.
///
/// Although transform components can theoretically and technically exist without being attached to an entity,
/// in practice this distinction is not made. Every entity has exactly one built-in transform component, and
/// terms like "the origin of the transform" and "the origin of the entity" are used synonymously.
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

    /// Returns the origin of the transform (in world-space).
    tuple GetOriginWS();

    /// Sets the origin of the transform (in world-space).
    SetOriginWS(number x, number y, number z);

    /// Returns the orientation of this entity as a tuple of three angles, measured in degrees:
    ///   - heading (yaw),
    ///   - pitch,
    ///   - bank (roll).
    /// The angles are relative to the coordinate system of the parent entity.
    tuple GetAngles();

    /// Sets the orientation of this entity from a set of three angles, measured in degrees:
    ///   - heading (yaw),
    ///   - pitch,
    ///   - bank (roll).
    /// The angles are relative to the coordinate system of the parent entity.
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

    /// Sets the orientation of the transform so that it "looks at" the given position.
    /// The new orientation is chosen such that the bank angle is always 0 relative to the xy-plane.
    /// @param PosX      The target position to look at (x-component).
    /// @param PosY      The target position to look at (y-component).
    /// @param PosZ      The target position to look at (z-component).
    /// @param AxisNr    The "look axis", i.e. the number of the axis to orient towards `Pos`:
    ///                  0 for the x-axis, 1 for the y-axis.
    /// @param NoPitch   If `true`, the pitch angle is kept at 0, and the given axis points towards `Pos`
    ///                  only in the XY-Plane and the z-axis points straight up (0, 0, 1).
    LookAt(number PosX, number PosY, number PosZ, integer AxisNr = 0, boolean NoPitch = false);


    public:

    /// The origin of the entity (in the coordinate system of its parent).
    /// @cppType{Vector3fT}
    tuple Origin;

    /// The orientation of the entity (in the coordinate system of its parent).
    /// @cppType{Vector3fT}
    tuple Orientation;
};


}   // namespace GameSys
