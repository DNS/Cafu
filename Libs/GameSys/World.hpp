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

#ifndef CAFU_GAMESYS_WORLD_HPP_INCLUDED
#define CAFU_GAMESYS_WORLD_HPP_INCLUDED

#include "UniScriptState.hpp"

#include <stdexcept>


namespace cf { namespace GuiSys { class GuiResourcesT; } }
class ModelManagerT;
struct CaKeyboardEventT;
struct CaMouseEventT;


namespace cf
{
    namespace GameSys
    {
        class EntityT;


        /// This class holds the hierarchy of game entities that populate a game world.
        /// The root of the hierarchy is the map entity, all other entities are direct or indirect children of it.
        /// The world also holds shared resources that all entities commonly use, such as the script state and the
        /// model manager.
        class WorldT
        {
            public:

            class InitErrorT;

            /// Flags for initializing a world, used in the WorldT constructor.
            enum InitFlagsT
            {
                InitFlag_InlineCode  = 1,   ///< Normally, the `ScriptName` parameter to the WorldT ctor is a filename. If this is set, it is treated as inline script code.
                InitFlag_InMapEditor = 2    ///< Whether the world is instantiated in the Map Editor. If set, only the static data will be loaded, initial behaviour is *not* run.
            };


            /// Constructor for creating an entity hierarchy (== "a world") from the given script file.
            /// @param ScriptName   The file name of the script to load.
            /// @param ModelMan     The manager for all models that are used in this world.
            /// @param GuiRes       The provider for resources (fonts and models) for all GUIs in this world.
            /// @param Flags        A combination of the flags in InitFlagsT.
            /// @throws Throws an InitErrorT object on problems initializing the world.
            WorldT(const std::string& ScriptName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, int Flags = 0);

            /// The destructor.
            ~WorldT();

            /// Returns the name of the script file of this world.
            const std::string& GetScriptName() const { return m_ScriptName; }

            /// Returns the script state of this world.
            UniScriptStateT& GetScriptState() { return m_ScriptState; }

            /// Returns how many millimeters one world unit is large.
            /// Used whenever we have to deal with concrete units of measurement such as millimeters or meters
            /// (e.g. for physics computations, the acoustic Doppler effect, etc.).
            /// FIXME: The same constant is currently also defined (as `const double METERS_PER_WORLD_UNIT = 0.0254`)
            ///        in CollisionModelStaticT::BulletAdapterT, PhysicsWorldT::TraceBoundingBox(), and the Sound Systems.
            float GetMillimetersPerWorldUnit() const { return 25.4f; }

            /// Returns the root entity of this world.
            IntrusivePtrT<EntityT> GetRootEntity() const { return m_RootEntity; }

            /// Returns the ID that the next newly created entity should get.
            unsigned int GetNextEntityID(unsigned int ForcedID = UINT_MAX);

            /// Returns the manager for all models that are used in this world.
            ModelManagerT& GetModelMan() const { return m_ModelMan; }

            /// Returns the resource provider for commonly used GUI fonts and models.
            /// All GUIs that are created in this world share their font and model resources via the returned GuiResourcesT instance.
            cf::GuiSys::GuiResourcesT& GetGuiResources() const { return m_GuiResources; }

            /// Renders this world.
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices:
            /// it's up to the caller to do that.
            void Render() const;

            /// Processes a keyboard event by forwarding it to the entity that currently has the input focus.
            /// @param KE   The keyboard event to process.
            /// @returns `true` if the device has been successfully processed, `false` otherwise.
            bool ProcessDeviceEvent(const CaKeyboardEventT& KE);

            /// Processes a mouse event by forwarding it to the entity that currently has the input focus.
            /// @param ME   The mouse event to process.
            /// @returns `true` if the device has been successfully processed, `false` otherwise.
            bool ProcessDeviceEvent(const CaMouseEventT& ME);

            /// Calls the OnClockTickEvent() method of each entity in the world.
            /// @param t   The time in seconds since the last clock tick.
            void DistributeClockTickEvents(float t);


            private:

            WorldT(const WorldT&);              ///< Use of the Copy Constructor    is not allowed.
            void operator = (const WorldT&);    ///< Use of the Assignment Operator is not allowed.

            void Init();    ///< Calls the OnInit() script methods of all entities.


            const std::string          m_ScriptName;    ///< The name of the script file that this world instance was loaded from.
            UniScriptStateT            m_ScriptState;   ///< The script state of this world.
            IntrusivePtrT<EntityT>     m_RootEntity;    ///< The root of the entity hierarchy that forms this world.
            bool                       m_IsInited;      ///< Has the Init() method already been called?
            unsigned int               m_NextEntID;     ///< The ID that the next newly created entity should get.
            ModelManagerT&             m_ModelMan;      ///< The manager for all models that are used in this world.
            cf::GuiSys::GuiResourcesT& m_GuiResources;  ///< The provider for resources (fonts and models) for all GUIs in this world.


            // Methods called from Lua scripts on cf::GameSys::WorldT instances.
            static int SetRootEntity(lua_State* LuaState);  ///< Sets the root entity for this world.
            static int CreateNew(lua_State* LuaState);      ///< Creates and returns a new entity or component.
            static int Init(lua_State* LuaState);           ///< Calls the OnInit() script methods of all entities.
            static int toString(lua_State* LuaState);       ///< Returns a short string description of this world.

            /// Adds a new global variable of type (meta-)table with name "cf::GameSys::WorldT" to the given Lua state,
            /// containing functions (or rather "methods") that can be called on userdata objects of type cf::GameSys::WorldT.
            /// This is very analogous to how normal C-code modules are registered with Lua, except for
            /// the fact that this table is intended to be set as metatable for userdata objects of type cf::GameSys::WorldT.
            /// For more details, see the implementation of this function and the PiL2 book, chapter 28.1 to 28.3.
            /// @param LuaState   The Lua state to register the metatable in.
            static void RegisterLua(lua_State* LuaState);
        };
    }
}


/// A class that is thrown on WorldT initialization errors.
class cf::GameSys::WorldT::InitErrorT : public std::runtime_error
{
    public:

    InitErrorT(const std::string& Message);
};

#endif
