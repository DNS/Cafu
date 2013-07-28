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

#ifndef CAFU_CA3DECOMMONWORLD_HPP_INCLUDED
#define CAFU_CA3DECOMMONWORLD_HPP_INCLUDED

#include "PhysicsWorld.hpp"
#include "ScriptState.hpp"
#include "../Common/World.hpp"
#include "../Games/GameWorld.hpp"


namespace cf { namespace ClipSys { class ClipWorldT; } }
class CompGameEntityT;
class EngineEntityT;


// Ca3DEWorldT implementiert die Eigenschaften, die eine CaServerWorld und eine CaClientWorld gemeinsam haben.
class Ca3DEWorldT : public cf::GameSys::GameWorldI
{
    public:

    Ca3DEWorldT(cf::GameSys::GameInfoI* GameInfo, cf::GameSys::GameI* Game, const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction) /*throw (WorldT::LoadErrorT)*/;
    ~Ca3DEWorldT();

    const WorldT& GetWorld() const { return *m_World; }

    // The virtual methods inherited from the base class GameWorldI.
    cf::GameSys::GameI*          GetGame();
    cf::ClipSys::ClipWorldT&     GetClipWorld();
    PhysicsWorldT&               GetPhysicsWorld();
    cf::UniScriptStateT&         GetScriptState();
    Vector3fT                    GetAmbientLightColorFromBB(const BoundingBox3T<double>& Dimensions, const VectorT& Origin) const;
    const ArrayT<unsigned long>& GetAllEntityIDs() const;
    IntrusivePtrT<GameEntityI>   GetGameEntityByID(unsigned long EntityID) const;
    std::string                  GetEntityNameByID(unsigned long EntityID) const;
    // unsigned long             CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin);
    // void                      RemoveEntity(unsigned long EntityID);
    const CafuModelT*            GetModel(const std::string& FileName) const;
    cf::GuiSys::GuiResourcesT&   GetGuiResources() const;


    protected:

    /// Creates a new entity that is added to the m_EngineEntities array.
    ///
    ///   - This is called in the constructor (and thus both on the client *and* the server, whenever a new world has
    ///     been loaded) with the parameters from the `.cw` world file.
    ///   - On the server, this method is also called from the `GameWorldI::CreateNewEntity()` implementation,
    ///     when an entity wants to create a new entity.
    ///   - Third, this is called by `ServerWorldT::InsertHumanPlayerEntity()` for creating human player entities for
    ///     newly joined clients or after a world-change for the existing clients.
    ///
    /// @returns The ID of the newly created entity, so that the server can let the client know which entity the
    ///     client itself is. If it was not possible to create the entity, 0xFFFFFFFF is returned.
    unsigned long CreateNewEntityFromBasicInfo(IntrusivePtrT<const CompGameEntityT> CompGameEnt,
        unsigned long CreationFrameNr, const Vector3dT& Origin, const char* PlayerName = NULL, const char* ModelName = NULL);

    cf::GameSys::GameI*        m_Game;
    const WorldT*              m_World;
    cf::GameSys::WorldT*       m_ScriptWorld;   ///< The "script world" contains the entity hierarchy and their components.
    cf::ClipSys::ClipWorldT*   m_ClipWorld;
    PhysicsWorldT              m_PhysicsWorld;
    ScriptStateT               m_ScriptState;
    ArrayT<EngineEntityT*>     m_EngineEntities;
    ModelManagerT&             m_ModelMan;
    cf::GuiSys::GuiResourcesT& m_GuiRes;


    private:

    Ca3DEWorldT(const Ca3DEWorldT&);            ///< Use of the Copy Constructor    is not allowed.
    void operator = (const Ca3DEWorldT&);       ///< Use of the Assignment Operator is not allowed.
};

#endif
