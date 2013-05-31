/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#ifndef CAFU_GAMESYS_ENTITY_HPP_INCLUDED
#define CAFU_GAMESYS_ENTITY_HPP_INCLUDED

#include "CompBasics.hpp"
#include "CompTransform.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"

#include <climits>


struct CaKeyboardEventT;
struct CaMouseEventT;
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

            /// The virtual copy constructor.
            /// Callers can use this method to create a copy of this entity without knowing its concrete type.
            /// Overrides in derived classes use a covariant return type to facilitate use when the concrete
            /// type is known.
            ///
            /// @param Recursive   Whether to recursively clone all children of this entity as well.
            virtual EntityT* Clone(bool Recursive=false) const;

            /// The virtual destructor. Deletes this entity and all its children.
            virtual ~EntityT();


            WorldT& GetWorld() const { return m_World; }

            /// Returns the parent entity of this entity.
            IntrusivePtrT<EntityT> GetParent() const { return m_Parent; }

            /// Returns the immediate children of this entity.
            /// This is analogous to calling GetChildren(Chld, false) with an initially empty Chld array.
            const ArrayT< IntrusivePtrT<EntityT> >& GetChildren() const { return m_Children; }

            /// Returns the children of this entity.
            /// @param Chld      The array to which the children of this entity are appended. Note that Chld gets *not* initially cleared by this function!
            /// @param Recurse   Determines if also the grand-children, grand-grand-children etc. are returned.
            void GetChildren(ArrayT< IntrusivePtrT<EntityT> >& Chld, bool Recurse=false) const;

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

            /// Returns the (n-th) component of the given (type) name.
            /// Covers the "custom" components as well as the application components, "Basics" and "Transform".
            /// That is, `GetComponent("Basics") == GetBasics()` and `GetComponent("Transform") == GetTransform()`.
            IntrusivePtrT<ComponentBaseT> GetComponent(const std::string& TypeName, unsigned int n=0) const;

            /// Adds the given component to this entity.
            ///
            /// @param Comp    The component to add to this entity.
            /// @param Index   The position among the other components to insert `Comp` at.
            /// @returns `true` on success, `false` on failure (if `Comp` is part of an entity already).
            bool AddComponent(IntrusivePtrT<ComponentBaseT> Comp, unsigned long Index=ULONG_MAX);

            /// Deletes the component at the given index from this entity.
            void DeleteComponent(unsigned long CompNr);


            /// Returns the position of this entity in absolute (vs. relative to the parent) coordinates.
            Vector3fT GetAbsoluteOrigin() const;

            /// Finds the entity with the name WantedName in the hierachy tree of this entity.
            /// Use `GetRoot()->Find("xy")` in order to search the entire world for the entity with name `xy`.
            /// @param WantedName   The name of the entity that is to be found.
            /// @returns The pointer to the desired entity, or `NULL` if no entity with this name exists.
            IntrusivePtrT<EntityT> Find(const std::string& WantedName);   // Method cannot be const because return type is not const -- see implementation.

            /// Renders this entity.
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices:
            /// it's up to the caller to do that.
            virtual void Render() const;

            /// Keyboard input event handler.
            /// @param KE   Keyboard event.
            virtual bool OnInputEvent(const CaKeyboardEventT& KE);

            /// Mouse input event handler.
            /// @param ME     Mouse event.
            /// @param PosX   Mouse position x.
            /// @param PosY   Mouse position y.
            virtual bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY);

            /// The clock-tick event handler.
            /// @param t   The time in seconds since the last clock-tick.
            virtual bool OnClockTickEvent(float t);

            /// Calls the Lua method with name `MethodName` of this entity.
            /// This method is analogous to WorldT::CallLuaFunc(), see there for more details.
            /// @param MethodName   The name of the lua method to call.
            /// @param Signature    DOCTODO
            bool CallLuaMethod(const char* MethodName, const char* Signature="", ...);


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // Methods called from Lua scripts on cf::GameSys::EntityT instances.
            // They are protected so that derived entity classes can access them when implementing overloads.
            static int AddChild(lua_State* LuaState);
            static int RemoveChild(lua_State* LuaState);
            static int GetParent(lua_State* LuaState);
            static int GetChildren(lua_State* LuaState);
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


            private:

            void operator = (const EntityT&);   ///< Use of the Assignment Operator is not allowed.

            WorldT&                                 m_World;        ///< The world instance in which this entity was created and exists. Useful in many regards, but especially for access to the commonly used resources, the script state, etc.
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
