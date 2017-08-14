/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_ENTITY_HPP_INCLUDED
#define CAFU_GAMESYS_ENTITY_HPP_INCLUDED

#include "CompBasics.hpp"
#include "CompTransform.hpp"

#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"

#include <climits>


struct lua_State;
struct luaL_Reg;
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }
namespace cf { namespace TypeSys { class MethsDocT; } }


namespace cf
{
    namespace GameSys
    {
        class EntityCreateParamsT;
        class WorldT;


        /// The TypeInfoTs of all EntityT derived classes must register with this TypeInfoManT instance.
        cf::TypeSys::TypeInfoManT& GetGameSysEntityTIM();


        /**
         * This class represents game entities, which are the basic elements of a world.
         *
         * EntityT instances can be created in C++ code or in Lua scripts, using the world:new() function.
         * They can be passed from C++ code to Lua and from Lua to C++ code at will.
         * In C++ code, all EntityT instances are kept in IntrusivePtrT's. Their lifetime is properly managed:
         * An entity is deleted automatically when it is no longer used in Lua \emph{and} in C++.
         * That is, code like
         *     Example 1:    w = world:new("EntityT"); world:SetRootEntity(w); w = nil;
         *     Example 2:    w = world:new("EntityT"); w:AddChild(world:new("EntityT"));
         * works as expected. See the cf::ScriptBinderT class for technical and implementation details.
         */
        class EntityT : public RefCountedT
        {
            public:

            /// The normal constructor.
            /// @param Params   The creation parameters for the entity.
            EntityT(const EntityCreateParamsT& Params);

            /// The copy constructor. It creates a new entity as a copy of another entity.
            /// The parent of the copy is always `NULL`, and it is up to the caller to insert the copy
            /// somewhere into an entity hierarchy.
            ///
            /// @param Entity      The entity to copy-construct this entity from.
            /// @param Recursive   Whether to recursively copy all children as well.
            EntityT(const EntityT& Entity, bool Recursive=false);

            /// The destructor. Destructs this entity and all its children.
            ~EntityT();


            WorldT& GetWorld() const { return m_World; }

            /// Returns the ID of this entity.
            /// The ID is unique in the world, and used to unambiguously identify the entity in network messages
            /// and as entity index number into `.cw` world files.
            unsigned int GetID() const { return m_ID; }

            /// Returns the parent entity of this entity.
            IntrusivePtrT<EntityT> GetParent() const { return m_Parent; }

            /// Returns the immediate children of this entity.
            /// This is analogous to calling GetChildren(Chld, false) with an initially empty Chld array.
            const ArrayT< IntrusivePtrT<EntityT> >& GetChildren() const { return m_Children; }

            /// Returns the children of this entity.
            /// @param Chld      The array to which the children of this entity are appended. Note that Chld gets *not* initially cleared by this function!
            /// @param Recurse   Determines if also the grand-children, grand-grand-children etc. are returned.
            void GetChildren(ArrayT< IntrusivePtrT<EntityT> >& Chld, bool Recurse=false) const;

            /// Traverses the hierarchy (sub-tree) that is rooted at this entity in depth-first order
            /// and records all encountered entities in the given list.
            /// @param List   All nodes in this sub-tree are added to this list in depth-first order.
            void GetAll(ArrayT< IntrusivePtrT<EntityT> >& List);

            /// Adds the given entity to the children of this entity, and sets this entity as the parent of the child.
            ///
            /// This method also makes sure that the name of the Child is unique among its siblings,
            /// modifying it as necessary. See SetName() for more details.
            ///
            /// @param Child   The entity to add to the children of this entity.
            /// @param Pos     The position among the children to insert the child entity at.
            /// @returns true on success, false on failure (Child has a parent already, or is the root of this entity).
            bool AddChild(IntrusivePtrT<EntityT> Child, unsigned long Pos=0xFFFFFFFF);

            /// Removes the given entity from the children of this entity.
            /// @param Child   The entity to remove from the children of this entity.
            /// @returns true on success, false on failure (Child is not a child of this entity).
            bool RemoveChild(IntrusivePtrT<EntityT> Child);

            /// Returns the top-most parent of this entity, that is, the root of the hierarchy that this entity is in.
            IntrusivePtrT<EntityT> GetRoot();     // Method cannot be const because return type is not const -- see implementation.


            /// Returns the application component of this entity.
            /// This component is much like the "Basics" and "Transform" components, but it can be set by the
            /// application (see SetApp()), and is intended for the sole use by the application, e.g. for
            /// implementing a "selection gizmo" in the Map Editor.
            IntrusivePtrT<ComponentBaseT> GetApp() { return m_App; }

            /// The `const` variant of the GetApp() method above. See GetApp() for details.
            IntrusivePtrT<const ComponentBaseT> GetApp() const { return m_App; }

            /// Sets the application component for this entity. See GetApp() for details.
            void SetApp(IntrusivePtrT<ComponentBaseT> App);

            /// Returns the "Basics" component of this entity.
            /// The "Basics" component defines the name and the "show" flag of the entity.
            IntrusivePtrT<ComponentBasicsT> GetBasics() const { return m_Basics; }

            /// Returns the "Transform" component of this entity.
            /// The "Transform" component defines the position, size and orientation of the entity.
            IntrusivePtrT<ComponentTransformT> GetTransform() const { return m_Transform; }


            /// Returns the components that this entity is composed of.
            /// Only the "custom" components are returned, does *not* include the application component,
            /// "Basics" or "Transform".
            const ArrayT< IntrusivePtrT<ComponentBaseT> >& GetComponents() const { return m_Components; }

            /// Returns the (`n`-th) component of the given (type) name.
            /// Covers both the "custom" as well as the fixed components (application, "Basics" and "Transform").
            /// That is, `GetComponent("Basics") == GetBasics()` and `GetComponent("Transform") == GetTransform()`.
            IntrusivePtrT<ComponentBaseT> GetComponent(const std::string& TypeName, unsigned int n=0) const;

            /// Returns the `n`-th component of this entity, covering both the "custom" as well
            /// as the fixed components (application, "Basics" and "Transform").
            /// This method facilitates looping over all of the entity's components, especially
            /// when neither their concrete type nor their concrete order are paramount.
            IntrusivePtrT<ComponentBaseT> GetComponent(unsigned int n) const;

            /// Adds the given component to this entity.
            ///
            /// @param Comp    The component to add to this entity.
            /// @param Index   The position among the other components to insert `Comp` at.
            /// @returns `true` on success, `false` on failure (if `Comp` is part of an entity already).
            bool AddComponent(IntrusivePtrT<ComponentBaseT> Comp, unsigned long Index=ULONG_MAX);

            /// Deletes the component at the given index from this entity.
            void DeleteComponent(unsigned long CompNr);


            /// Finds the entity with the given ID in the hierachy tree of this entity.
            /// Use `GetRoot()->Find(xy)` in order to search the entire world for the entity with ID `xy`.
            ///
            /// Note that the method cannot be `const` because the return type is not `const` (see the implementation
            /// for details).
            ///
            /// @param WantedID   The ID of the entity that is to be found.
            /// @returns The pointer to the desired entity, or `NULL` if no entity with this ID exists.
            IntrusivePtrT<EntityT> FindID(unsigned int WantedID);

            /// Finds the entity with the name WantedName in the hierachy tree of this entity.
            /// Use `GetRoot()->Find("xy")` in order to search the entire world for the entity with name `xy`.
            ///
            /// Note that the method cannot be `const` because the return type is not `const` (see the implementation
            /// for details).
            ///
            /// @param WantedName   The name of the entity that is to be found.
            /// @returns The pointer to the desired entity, or `NULL` if no entity with this name exists.
            IntrusivePtrT<EntityT> Find(const std::string& WantedName);

            /// Finds all entities in the hierachy tree of this entity that have at least one component of the given
            /// (type) name. Use `GetRoot()->FindByComponent("xy")` in order to search the entire world for entities
            /// with component `xy`.
            ///
            /// Note that the method cannot be `const` because the return type is not `const` (see the implementation
            /// for details).
            ///
            /// @param TypeName   The type name of the component that found entities must have.
            /// @param Result     All found entities are appended to this array.
            void FindByComponent(const std::string& TypeName, ArrayT< IntrusivePtrT<EntityT> >& Result);

            /// Returns whether the given entity is in the hierarchy (sub-tree) of this entity.
            bool Has(IntrusivePtrT<EntityT> Ent) const;

            /// Returns a bounding-box that encloses the visual representation of this entity.
            /// It is used to determine if the entity is in the view-frustum of a camera, how large a region must be
            /// updated in the 2D views of a Map Editor, if the entity is in the potentially-visibility-set (PVS) of
            /// another entity, and similar purposes.
            ///
            /// This method does *not* recurse: The returned bounding-box covers this entity, but not its children.
            ///
            /// Note that many details are up to the entity's components, whose GetCullingBB() method this method's
            /// implementation calls. For example, it is up to the Map Editor's "App" component to decide whether
            /// it includes selection gizmo handles and/or the map primitives in the returned bounding-box.
            ///
            /// @param WorldSpace   If `true`, the bounding-box is returned in world-space coordinates.
            ///                     If `false`, the bounding-box is returned in local entity-space.
            ///                     Note that due to the transformation, the volume of the bounding-box in world-space
            ///                     may be larger than the volume of the bounding-box in entity-space.
            ///
            /// @return The bounding-box that encloses the visual representation of this entity. The returned
            ///     bounding-box is always valid, but possibly not inited (`!IsInited()`), indicating that the entity
            ///     doesn't have a visual representation.
            BoundingBox3fT GetCullingBB(bool WorldSpace) const;

            /// Writes the current state of this entity into the given stream.
            /// This method is called to send the state of the entity over the network, to save it to disk,
            /// or to store it in the clipboard.
            ///
            /// @param Stream
            ///   The stream to write the state data to.
            ///
            /// @param WithChildren
            ///   Should the children be recursively serialized as well?
            void Serialize(cf::Network::OutStreamT& Stream, bool WithChildren=false) const;

            /// Reads the state of this entity from the given stream, and updates the entity accordingly.
            /// This method is called after the state of the entity has been received over the network,
            /// has been loaded from disk, has been read from the clipboard, or must be "reset" for the purpose
            /// of (re-)prediction.
            ///
            /// @param Stream
            ///   The stream to read the state data from.
            ///
            /// @param IsIniting
            ///   Used to indicate that the call is part of the construction / first-time initialization of the entity.
            ///   The implementation will use this to not wrongly process the event counters, interpolation, etc.
            void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting);

            /// Renders the components of this entity.
            /// Note that this method does *not* recurse into the children, and it does *not* setup any of the
            /// MatSys's model, view or projection matrices: it's up to the caller to do that.
            /// @param FirstPersonView   If the world is rendered from the perspective of this entity.
            /// @param LodDist           The distance of the viewer entity to this entity.
            /// @returns `true` if "something" was rendered, `false` otherwise (in this case the Map Editor may choose
            ///     to render another visual representation of this component's entity).
            bool RenderComponents(bool FirstPersonView, float LodDist) const;

            /// Advances the entity one frame (one "clock-tick") on the server.
            /// It typically updates all game-relevant state that is sync'ed over the network to all
            /// connected game clients.
            /// ComponentBaseT::OnServerFrame() is called by this method for all components of this entity.
            ///
            /// @param t   The time in seconds since the last server frame.
            void OnServerFrame(float t);

            /// Advances the entity one frame (one "clock-tick") on the client.
            /// It typically updates eye-candy that is *not* sync'ed over the network.
            /// ComponentBaseT::OnClientFrame() is called by this method for all components of this entity.
            ///
            /// @param t   The time in seconds since the last client frame.
            void OnClientFrame(float t);

            /// Calls the Lua method with name `MethodName` of this entity.
            /// This method is analogous to UniScriptStateT::CallMethod(), see there for details.
            /// @param MethodName     The name of the Lua method to call.
            /// @param NumExtraArgs   The number of extra arguments that have been pushed on the stack.
            /// @param Signature      See UniScriptStateT::Call() for details.
            bool CallLuaMethod(const char* MethodName, int NumExtraArgs, const char* Signature="", ...);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // Methods called from Lua scripts on cf::GameSys::EntityT instances.
            // They are protected so that derived entity classes can access them when implementing overloads.
            static int GetID(lua_State* LuaState);
            static int AddChild(lua_State* LuaState);
            static int RemoveChild(lua_State* LuaState);
            static int GetParent(lua_State* LuaState);
            static int GetRoot(lua_State* LuaState);
            static int GetChildren(lua_State* LuaState);
            static int FindByID(lua_State* LuaState);
            static int FindByName(lua_State* LuaState);
            static int FindByComponent(lua_State* LuaState);
            static int GetBasics(lua_State* LuaState);
            static int GetTransform(lua_State* LuaState);
            static int AddComponent(lua_State* LuaState);
            static int RmvComponent(lua_State* LuaState);
            static int GetComponents(lua_State* LuaState);
            static int GetComponent(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< List of methods registered with Lua.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::MethsDocT DocCallbacks[];


            private:

            void UpdateAllDependencies();       ///< Updates the dependencies of all components.
            void operator = (const EntityT&);   ///< Use of the Assignment Operator is not allowed.

            WorldT&                                 m_World;        ///< The world instance in which this entity was created and exists. Useful in many regards, but especially for access to the commonly used resources, the script state, etc.
            const unsigned int                      m_ID;           ///< The ID of this entity. It is unique in the `m_World`, and used to unambiguously identify the entity in network messages and as entity index number into `.cw` world files.
            EntityT*                                m_Parent;       ///< The parent of this entity. May be NULL if there is no parent. In order to not create cycles of IntrusivePtrT's, the type is intentionally a raw pointer only.
            ArrayT< IntrusivePtrT<EntityT> >        m_Children;     ///< The list of children of this entity.
            IntrusivePtrT<ComponentBaseT>           m_App;          ///< A component for the sole use by the application / implementation.
            IntrusivePtrT<ComponentBasicsT>         m_Basics;       ///< The component that defines the name and the "show" flag of this entity.
            IntrusivePtrT<ComponentTransformT>      m_Transform;    ///< The component that defines the position and orientation of this entity.
            ArrayT< IntrusivePtrT<ComponentBaseT> > m_Components;   ///< The components that this entity is composed of.
        };
    }
}

#endif
