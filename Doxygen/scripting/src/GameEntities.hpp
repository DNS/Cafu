namespace GameSys
{


/// An entity is the basic element in a game world.
///
/// Entity can be hierarchically arranged in parent/child relationships, e.g. a player that rides a car.
///
/// An entity is a separate unit that is self-contained and has its own identity, but has very little own features.
/// Instead, an entity contains a set of components, each of which implements a specific feature for the entity.
///
/// If you would like to create a new entity explicitly (those defined in the CaWE Map Editor are instantiated automatically), use WorldT::new():
/// \code{.lua}
///     local entity = world:new("EntityT", "my_entity")
/// \endcode
///
/// @nosubgrouping
/// @cppName{cf,GameSys,EntityT}
class EntityT
{
    public:

    /// Returns the ID of this entity.
    /// The ID is unique in the world, and is used (in C++ code) to unambiguously identify
    /// the entity in network messages and as entity index number into `.cw` world files.
    ///
    number GetID();

    /// This method adds the given entity to the children of this entity.
    AddChild(entity child);

    /// This method removes the given entity from the children of this entity.
    /// @param child   The entity that is to be removed from the children of this entity.
    RemoveChild(entity child);

    /// This method returns the parent of this entity (or `nil` if there is no parent).
    EntityT GetParent();

    /// Returns the top-most parent of this entity, that is, the root of the hierarchy that this entity is in.
    EntityT GetRoot();

    /// This method returns an array of the children of this entity.
    table GetChildren();

    /// Finds the entity with the given ID in the hierachy tree of this entity.
    /// Use `GetRoot():Find(xy)` in order to search the entire world for the entity with ID `xy`.
    /// @param ID   The ID of the entity that is to be found.
    /// @returns The entity with the desired ID, or `nil` if no entity with this ID exists.
    ///
    EntityT FindByID(number ID);

    /// Finds the entity with the given name in the hierachy tree of this entity.
    /// Use `GetRoot():Find("xy")` in order to search the entire world for the entity with name `xy`.
    /// @param Name   The name of the entity that is to be found.
    /// @returns The entity with the desired name, or `nil` if no entity with this name exists.
    ///
    EntityT FindByName(string Name);

    /// Finds all entities in the hierachy tree of this entity that have at least one component of the given (type) name.
    /// Use `GetRoot():FindByComponent("xy")` in order to search the entire world for entities with component `xy`.
    ///
    /// @param TypeName   The type name of the component that found entities must have
    /// @returns The array of entities that have a component of the desired type.
    ///
    table FindByComponent(string TypeName);

    /// This method returns the "Basics" component of this entity.
    ComponentBasicsT GetBasics();

    /// This method returns the "Transform" component of this entity.
    ComponentTransformT GetTransform();

    /// This method adds a component to this entity.
    AddComponent(ComponentBaseT component);

    /// This method removes a component from this entity.
    RemoveComponent(ComponentBaseT component);

    /// This method returns an array of the components of this entity.
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

    /// This method is called for each entity when a new world is loaded.
    /// Note that these OnInit() methods are automatically written by the Cafu Map Editor
    /// into a world's `.cent` files, you normally don't write them yourself.
    /// Use the ComponentBaseT::OnInit() methods instead for custom dynamic initialization.
    OnInit();

    /** @} */
};


}   // namespace GameSys
