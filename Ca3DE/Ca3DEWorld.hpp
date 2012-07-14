/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
class EngineEntityT;


// Ca3DEWorldT implementiert die Eigenschaften, die eine CaServerWorld und eine CaClientWorld gemeinsam haben.
class Ca3DEWorldT : public cf::GameSys::GameWorldI
{
    public:

    Ca3DEWorldT(const char* FileName, ModelManagerT& ModelMan, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction) /*throw (WorldT::LoadErrorT)*/;
    ~Ca3DEWorldT();

    const WorldT& GetWorld() const { return *m_World; }

    // The virtual methods inherited from the base class GameWorldI.
    cf::ClipSys::ClipWorldT&     GetClipWorld();
    PhysicsWorldT&               GetPhysicsWorld();
    cf::UniScriptStateT&         GetScriptState();
    Vector3fT                    GetAmbientLightColorFromBB(const BoundingBox3T<double>& Dimensions, const VectorT& Origin) const;
    const ArrayT<unsigned long>& GetAllEntityIDs() const;
    BaseEntityT*                 GetBaseEntityByID(unsigned long EntityID) const;
    // unsigned long             CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin);
    // void                      RemoveEntity(unsigned long EntityID);
    const CafuModelT*            GetModel(const std::string& FileName) const;


    protected:

    const WorldT*            m_World;
    cf::ClipSys::ClipWorldT* m_ClipWorld;
    PhysicsWorldT            m_PhysicsWorld;
    ScriptStateT             m_ScriptState;
    ArrayT<EngineEntityT*>   m_EngineEntities;
    ModelManagerT&           m_ModelMan;


    private:

    Ca3DEWorldT(const Ca3DEWorldT&);            ///< Use of the Copy Constructor    is not allowed.
    void operator = (const Ca3DEWorldT&);       ///< Use of the Assignment Operator is not allowed.
};

#endif
