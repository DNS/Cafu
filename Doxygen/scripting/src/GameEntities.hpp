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

    /// This method returns an array of the children of this entity.
    table GetChildren();

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

    /** @} */
};


}   // namespace GameSys
