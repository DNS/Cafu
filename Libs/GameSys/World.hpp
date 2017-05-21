/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_WORLD_HPP_INCLUDED
#define CAFU_GAMESYS_WORLD_HPP_INCLUDED

#include "UniScriptState.hpp"

#include <stdexcept>


namespace cf { namespace ClipSys { class ClipWorldT; } }
namespace cf { namespace ClipSys { class CollModelManI; } }
namespace cf { namespace GuiSys { class GuiResourcesT; } }
class ModelManagerT;
class PhysicsWorldT;


namespace cf
{
    namespace GameSys
    {
        /// The TypeInfoTs of all WorldT-derived classes must register with this TypeInfoManT instance.
        cf::TypeSys::TypeInfoManT& GetWorldTIM();


        class EntityT;


        /// This class holds the hierarchy of game entities that populate a game world.
        /// The root of the hierarchy is the map entity, all other entities are direct or indirect children of it.
        /// The world also holds shared resources that all entities commonly use, such as the script state and the
        /// model manager.
        class WorldT : public RefCountedT
        {
            public:

            class InitErrorT;

            /// A value that indicates where and to which purpose a game world is instantiated.
            /// The details of a world sometimes depend on its realm:
            ///   - prefabs can only be loaded into worlds in the Map Editor,
            ///   - client interpolations only need to be accounted for on the clients,
            ///   - the human player's "think" code sometimes must know whether it is running
            ///     on the server or in a prediction step on the client.
            enum RealmT
            {
                RealmServer,
                RealmClient,
                RealmMapEditor,
                RealmOther
            };

            /// Flags for initializing a world from a map script.
            enum InitFlagsT
            {
                InitFlag_InlineCode = 1,    ///< Normally, the `ScriptName` parameter to the WorldT ctor is a filename. If this is set, it is treated as inline script code.
                InitFlag_OnlyStatic = 2,    ///< If set, only the static data will be loaded. User-defined scripts with custom, initial behaviour are *not* run.
                InitFlag_AsPrefab   = 4     ///< This must be set if the map script is loaded for use as a prefab. Can only be used if the world is in realm `RealmMapEditor`.
            };

            /// Initializes the given script state for use with WorldT instances.
            static void InitScriptState(UniScriptStateT& ScriptState);


            /// Constructor for creating an entity hierarchy (== "a world") from the given script file.
            /// @param Realm        The realm of this world, indicating where and to which purpose the world is instantiated.
            /// @param ScriptState  The caller will use this world with this script state (binds the world to it).
            /// @param ModelMan     The manager for all models that are used in this world.
            /// @param GuiRes       The provider for resources (fonts and models) for all GUIs in this world.
            /// @param CollModelMan The manager for all collision models that are used in this world.
            /// @param ClipWorld    The clip world, where entities can register their collision models and run collision detection queries. Can be `NULL`, e.g. in CaWE or the map compile tools.
            /// @param PhysicsWorld The physics world, where entities can register their rigid bodies and run collision detection queries. Can be `NULL`, e.g. in CaWE or the map compile tools.
            WorldT(RealmT Realm, UniScriptStateT& ScriptState, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes,
                   cf::ClipSys::CollModelManI& CollModelMan, cf::ClipSys::ClipWorldT* ClipWorld, PhysicsWorldT* PhysicsWorld);

            /// Loads and runs the given script in order to initialize this world instance.
            /// Also assigns this world instance to the global script variable "world", see implementation for details.
            /// @param ScriptName   The file name of the script to load or inline script code (if InitFlag_InlineCode is set).
            /// @param Flags        A combination of the flags in InitFlagsT.
            /// @throws Throws an InitErrorT object on problems initializing the world.
            /// @returns The root entity as loaded from the given script.
            IntrusivePtrT<EntityT> LoadScript(const std::string& ScriptName, int Flags = 0);

            /// Returns the realm of this world, indicating where and to which purpose the
            /// world has been instantiated.
            RealmT GetRealm() const { return m_Realm; }

            /// Returns the script state of this world.
            UniScriptStateT& GetScriptState() { return m_ScriptState; }

            /// Returns how many millimeters one world unit is large.
            /// Used whenever we have to deal with concrete units of measurement such as millimeters or meters
            /// (e.g. for physics computations, radiometric units (e.g. W/m^2), the acoustic Doppler effect, etc.).
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

            /// Returns the manager for all collision models that are used in this world.
            cf::ClipSys::CollModelManI& GetCollModelMan() const { return m_CollModelMan; }

            /// The clip world, where entities can register their collision models and run collision detection queries.
            /// Can be `NULL`, e.g. in CaWE or the map compile tools.
            cf::ClipSys::ClipWorldT* GetClipWorld() const { return m_ClipWorld; }

            /// The physics world, where entities can register their rigid bodies and run collision detection queries.
            /// Can be `NULL`, e.g. in CaWE or the map compile tools.
            PhysicsWorldT* GetPhysicsWorld() const { return m_PhysicsWorld; }

            /// Renders this world.
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices:
            /// it's up to the caller to do that.
            void Render() const;

            // /// Advances the world one frame (one "clock-tick") on the server.
            // /// It typically updates all game-relevant state that is sync'ed over the network to all
            // /// connected game clients.
            // /// EntityT::OnServerFrame() is called by this method for each entity in this world.
            // ///
            // /// @param t   The time in seconds since the last server frame.
            // void OnServerFrame(float t);

            /// Advances the entity one frame (one "clock-tick") on the client.
            /// It typically updates eye-candy that is *not* sync'ed over the network.
            /// EntityT::OnClientFrame() is called by this method for each entity in this world.
            ///
            /// @param t   The time in seconds since the last client frame.
            void OnClientFrame(float t);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            WorldT(const WorldT&);              ///< Use of the Copy Constructor    is not allowed.
            void operator = (const WorldT&);    ///< Use of the Assignment Operator is not allowed.

            void Init();    ///< Calls the OnInit() script methods of all entities.


            const RealmT                m_Realm;        ///< The realm of this world, indicating where and to which purpose the world has been instantiated.
            UniScriptStateT&            m_ScriptState;  ///< The script state that this world is bound to.
            IntrusivePtrT<EntityT>      m_RootEntity;   ///< The root of the entity hierarchy that forms this world.
            unsigned int                m_NextEntID;    ///< The ID that the next newly created entity should get.
            ModelManagerT&              m_ModelMan;     ///< The manager for all models that are used in this world.
            cf::GuiSys::GuiResourcesT&  m_GuiResources; ///< The provider for resources (fonts and models) for all GUIs in this world.
            cf::ClipSys::CollModelManI& m_CollModelMan; ///< The manager for all collision models that are used in this world.
            cf::ClipSys::ClipWorldT*    m_ClipWorld;    ///< The clip world, where entities can register their collision models and run collision detection queries. Can be `NULL`, e.g. in CaWE or the map compile tools.
            PhysicsWorldT*              m_PhysicsWorld; ///< The physics world, where entities can register their rigid bodies and run collision detection queries. Can be `NULL`, e.g. in CaWE or the map compile tools.


            // Methods called from Lua scripts on cf::GameSys::WorldT instances.
            static int CreateNew(lua_State* LuaState);      ///< Creates and returns a new entity or component.
            static int GetRootEntity(lua_State* LuaState);  ///< Returns the root entity of this world.
            static int SetRootEntity(lua_State* LuaState);  ///< Sets the root entity for this world.
            static int TraceRay(lua_State* LuaState);       ///< Employs m_ClipWorld->TraceRay() to trace a ray through the (clip) world.
            static int Phys_TraceBB(lua_State* LuaState);   ///< Employs m_PhysicsWorld->TraceBoundingBox() to trace a bounding-box through the (physics) world.
            static int toString(lua_State* LuaState);       ///< Returns a short string description of this world.

            static const luaL_Reg               MethodsList[];  ///< List of methods registered with Lua.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
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
