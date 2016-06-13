/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CA3DECOMMONWORLD_HPP_INCLUDED
#define CAFU_CA3DECOMMONWORLD_HPP_INCLUDED

#include "Templates/Pointer.hpp"
#include "PhysicsWorld.hpp"
#include "../Common/World.hpp"


namespace cf { namespace ClipSys { class ClipWorldT; } }
namespace cf { namespace GameSys { class EntityT; } }
namespace cf { class UniScriptStateT; }
class EngineEntityT;


// Ca3DEWorldT implementiert die Eigenschaften, die eine CaServerWorld und eine CaClientWorld gemeinsam haben.
class Ca3DEWorldT
{
    public:

    Ca3DEWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction) /*throw (WorldT::LoadErrorT)*/;
    ~Ca3DEWorldT();

    const WorldT& GetWorld() const { return *m_World; }

    /// Returns a "good" ambient light color for an arbitrary object (i.e. a model) of size Dimensions at Origin.
    /// The return value is derived from the worlds lightmap information "close" to the Dimensions at Origin.
    Vector3fT GetAmbientLightColorFromBB(const BoundingBox3T<double>& Dimensions, const VectorT& Origin) const;


    protected:

    /// Creates a new entity that is added to the m_EngineEntities array.
    ///
    ///   - This is called in the constructor (and thus both on the client *and* the server, whenever a new world has
    ///     been loaded) with the parameters from the `.cw` world file.
    ///   - On the server, this method is also called if the Think() code auto-detects that an entity was newly created.
    ///   - Third, this is called by `ServerWorldT::InsertHumanPlayerEntity()` for creating human player entities for
    ///     newly joined clients or after a world-change for the existing clients.
    void CreateNewEntityFromBasicInfo(IntrusivePtrT<cf::GameSys::EntityT> Ent, unsigned long CreationFrameNr);

    const WorldT*                      m_World;
    cf::ClipSys::ClipWorldT*           m_ClipWorld;
    PhysicsWorldT                      m_PhysicsWorld;
    cf::UniScriptStateT*               m_ScriptState;
    IntrusivePtrT<cf::GameSys::WorldT> m_ScriptWorld;   ///< The "script world" contains the entity hierarchy and their components.
    ArrayT<EngineEntityT*>             m_EngineEntities;


    private:

    Ca3DEWorldT(const Ca3DEWorldT&);            ///< Use of the Copy Constructor    is not allowed.
    void operator = (const Ca3DEWorldT&);       ///< Use of the Assignment Operator is not allowed.
};

#endif
