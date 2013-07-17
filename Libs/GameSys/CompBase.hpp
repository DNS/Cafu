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

#ifndef CAFU_GAMESYS_COMPONENT_BASE_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_BASE_HPP_INCLUDED

#include "Variables.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Templates/Pointer.hpp"

// This macro is introduced by some header (gtk?) under Linux...
#undef CurrentTime


namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }
namespace cf { namespace TypeSys { class MethsDocT; } }
namespace cf { namespace TypeSys { class VarsDocT; } }

struct CaKeyboardEventT;
struct CaMouseEventT;
struct lua_State;
struct luaL_Reg;


namespace cf
{
    namespace GameSys
    {
        class EntityT;

        /// This is the base class for the components that an entity is composed/aggregated of.
        ///
        /// Components are the basic building blocks of an entity: their composition defines
        /// the properties, the behaviour, and thus virtually every aspect of the entity.
        ///
        /// Components can exist in two invariants:
        ///   - Stand-alone, independent and not a part of any entity.
        ///   - Normally, as an active part of a entity.
        ///
        /// Stand-alone components typically occur when they're newly instantiated, for example
        /// when they are loaded from disk, when they are instantiated in scripts, or when they
        /// are kept in the clipboard or managed in the Undo/Redo system of the Map Editor.
        /// Newly created, copied or cloned components are initially stand-alone.
        ///
        /// A component becomes a part of an entity via the EntityT::AddComponent() method.
        /// The entity then knows the component, because it hosts it, and reversely, the
        /// component then knows the parent entity that it is a component of.
        class ComponentBaseT : public RefCountedT
        {
            public:

            /// The constructor.
            /// The newly created component is initially not a part of any entity.
            ComponentBaseT();

            /// The copy constructor.
            /// The newly copied component is initially not a part of any entity, even if the source component was.
            /// @param Comp   The component to create a copy of.
            ComponentBaseT(const ComponentBaseT& Comp);

            /// The virtual copy constructor.
            /// Callers can use this method to create a copy of this component without knowing its concrete type.
            /// Overrides in derived classes use a covariant return type to facilitate use when the concrete type is known.
            /// The newly cloned component is initially not a part of any entity, even if the source component was.
            virtual ComponentBaseT* Clone() const;

            /// The virtual destructor.
            virtual ~ComponentBaseT() { }


            /// Returns the parent entity that contains this component,
            /// or `NULL` if this component is currently not a part of any entity.
            EntityT* GetEntity() const { return m_Entity; }

            /// Returns the variable manager that keeps generic references to our member variables,
            /// providing a simple kind of "reflection" or "type introspection" feature.
            TypeSys::VarManT& GetMemberVars() { return m_MemberVars; }

            /// Calls the given Lua method of this component.
            /// This method is analogous to UniScriptStateT::CallMethod(), see there for details.
            /// @param MethodName   The name of the Lua method to call.
            /// @param Signature    See UniScriptStateT::Call() for details.
            bool CallLuaMethod(const char* MethodName, const char* Signature="", ...);

            /// Returns the name of this component.
            virtual const char* GetName() const { return "Base"; }

            /// Writes the current state of this component into the given stream.
            /// This method is called to send the state of the component over the network, to save it to disk,
            /// or to store it in the clipboard.
            ///
            /// @param Stream
            ///   The stream to write the state data to.
            void Serialize(cf::Network::OutStreamT& Stream) const;

            /// Reads the state of this component from the given stream, and updates the component accordingly.
            /// This method is called after the state of the component has been received over the network,
            /// has been loaded from disk, has been read from the clipboard, or must be "reset" for the purpose
            /// of (re-)prediction.
            ///
            /// @param Stream
            ///   The stream to read the state data from.
            ///
            /// @param IsIniting
            ///   Used to indicate that the call is part of the construction / first-time initialization of the component.
            ///   The implementation will use this to not wrongly process the event counters, interpolation, etc.
            void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting);


            /// This method is called whenever something "external" to this component has changed:
            ///   - if the parent entity has changed, because this component was added to or removed from it,
            ///   - if other components in the parent entity have changed.
            /// The component can use the opportunity to search the entity for "sibling" components
            /// that it depends on, and store direct pointers to them.
            /// Note however that dependencies among components must not be cyclic, or else the deletion
            /// of an entity will leave a memory leak.
            /// @param Entity   The parent entity that contains this component, or `NULL` to indicate that this component is removed from the entity that it used to be a part of.
            virtual void UpdateDependencies(EntityT* Entity);

            /// This method returns the component's bounding-box (with coordinates in the entity's local system).
            /// It is intended to cover the visual representation of the entity in the Map Editor.
            /// Note that the returned bounding-box can be uninitialized, i.e. `GetEditorBB().IsInited() == false` is possible.
            virtual BoundingBox3fT GetEditorBB() const { return BoundingBox3fT(); }

            /// This method implements the graphical output of this component.
            virtual void Render() const { }

            /// This method is called after all entities and their components have been loaded.
            ///
            /// It is called only once when the static part of world initializatzion is complete, i.e. after the initial
            /// values of all entities and their components have been set.
            /// Components can override this method in order act / do something / add custom behaviour at that time.
            ///
            /// For example, a choice component can use it to set the associated text component to the initial
            /// selection, a script component can forward it to the script by calling a related script function,
            /// a component that for backwards-compatibility supports reading old variables can convert to new ones, etc.
            ///
            /// @param InEditor   `true` if this world is instantiated in the Map Editor. This normally means that
            ///     custom behaviour should *not* run. If `false`, this world is instantiated "live", in-game.
            virtual void OnPostLoad(bool InEditor) { }

            /// This method handles keyboard input events.
            /// @param KE   Keyboard event instance.
            /// @returns Whether the component handled ("consumed") the event.
            virtual bool OnInputEvent(const CaKeyboardEventT& KE) { return false; }

            /// This method handles mouse input events.
            /// @param ME     Mouse event instance.
            /// @param PosX   x-coordinate of the mouse cursor position.
            /// @param PosY   y-coordinate of the mouse cursor position.
            /// @returns Whether the component handled ("consumed") the event.
            virtual bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY) { return false; }

            /// This method handles clock-tick events.
            /// @param t   The time in seconds since the last clock-tick.
            virtual void OnClockTickEvent(float t);


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int Get(lua_State* LuaState);
            static int Set(lua_State* LuaState);
            static int GetExtraMessage(lua_State* LuaState);
            static int Interpolate(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];


            private:

            /// A helper structure for interpolations.
            struct InterpolationT
            {
                cf::TypeSys::VarBaseT* Var;         ///< The variable whose value is being interpolated.
                unsigned int           Suffix;      ///< If the variable is composed of several values, this is the index of the one being interpolated.
                float                  StartValue;  ///< Start value of the interpolation.
                float                  EndValue;    ///< End value of the interpolation.
                float                  CurrentTime; ///< Current time between 0 and TotalTime.
                float                  TotalTime;   ///< Duration of the interpolation.

                float GetCurrentValue() const { return StartValue + (EndValue-StartValue)*CurrentTime/TotalTime; }
            };


            void operator = (const ComponentBaseT&);    ///< Use of the Assignment Operator is not allowed.

            EntityT*                m_Entity;           ///< The parent entity that contains this component, or `NULL` if this component is currently not a part of any entity.
            TypeSys::VarManT        m_MemberVars;       ///< The variable manager that keeps generic references to our member variables.
            ArrayT<InterpolationT*> m_PendingInterp;    ///< The currently pending interpolations.
        };
    }
}

#endif
