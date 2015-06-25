namespace GameSys
{


/// This class holds the hierarchy of game entities that populate a game world.
/// The root of the hierarchy is the map entity, all other entities are direct or indirect children of it.
///
/// This class represents the world that is defined in a `.cent` script file as a whole.
/// Its methods affect the entire world, not just a single entity of its entity hierarchy.
/// (The new() method described below is an exception: It is used to create new entities and new components.)
///
/// Note that you never create WorldT instances yourself:
/// Instead, each game script accesses the global `world` variable that is automatically predefined.
/// Thus, the methods of the WorldT class are always used like this:
/// @code
///     -- The "world" object is a predefined global variable.
///     world:someMethod(true)
/// @endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,WorldT}
class WorldT
{
    public:

    /// This method creates new game entities or new entity components.
    ///
    /// @par Example
    /// \code{.lua}
    ///     --// Create a new EntityT instance and set its object name.
    ///     local soldier1 = gui:new("EntityT", "Alex")
    ///
    ///     --// Create another entity, but set its name in a separate step.
    ///     local soldier2 = gui:new("EntityT")
    ///     soldier2:GetBasics():set("Name", "Bob")
    ///
    ///     --// Create some components and add them to soldier1.
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
    ///     soldier1:AddComponent(c1, c2, c3)
    /// \endcode
    ///
    /// @param ClassName
    ///     The name of the class of which an object should be created.
    ///     Use `"EntityT"` in order to create a new EntityT.
    ///     Use any class name from the ComponentBaseT hierarchy in order to create a new component,
    ///     for example `"ComponentModelT"` in order to create a new model component.
    ///
    /// @param InstanceName
    ///     The name that the newly created entity instance is assigned.
    ///     Specifying a name for an entity is equivalent to setting the `Name` attribute of its Basics component;
    ///     see the example above and ComponentBasicsT.Name for details.
    ///     Setting a proper entity instance name is important so that other script code can unambiguously find
    ///     and identify the entity by name later. The CaWE Map Editor also uses it in order to automatically create
    ///     the initialization script code in the `.cent` files.
    ///     This parameter is not used (and in fact ignored) for components, which have no individual object names.
    ///
    /// @returns The newly created object.
    object new(string ClassName, string InstanceName="");

    /// Returns the root entity of this world as previously set by SetRootEntity().
    EntityT GetRootEntity();

    /// Sets the root entity for this world.
    /// If you use the Map Editor that is part of the CaWE application, a proper call to this method is automatically included in the generated files.
    /// @param ent   The entity that is set as the root entity of this world.
    SetRootEntity(EntityT ent);

    /// Employs m_ClipWorld->Trace() to trace a ray through the (clip) world.
    table TraceRay(table Start, table Ray);

    /// Employs m_PhysicsWorld->TraceBoundingBox() to trace a bounding-box through the (physics) world.
    /// Note that this method is only useful with entities that do *not* have a collision model of their own,
    /// because it currently is not capable to ignore specific collision models (the entity's) for the trace,
    /// which was a necessity in this case.
    table Phys_TraceBB(table BB, table Start, table Ray);
};


}   // namespace GameSys
