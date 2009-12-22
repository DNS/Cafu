/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CA3DECOMMONWORLD_HPP_
#define _CA3DECOMMONWORLD_HPP_

#include "../../Common/World.hpp"
#include "../../Games/GameWorld.hpp"


namespace cf { namespace ClipSys { class ClipWorldT; } }
class EntityManagerT;


// Ca3DEWorldT implementiert die Eigenschaften, die eine CaServerWorld und eine CaClientWorld gemeinsam haben.
// Insbesondere werden wg. der 'Client-Prediction' die LeafClipBrushes und die dazugehörigen Funktionen geteilt.
// Sollte die 'Client-Prediction' jemals wieder abgeschafft werden (Internet schnell genug), können diese Daten/Funktionen wieder Server-eigen werden!
// Beachte: Client-Prediction bisher nicht abgeschafft, die ClipBrushes sind nun aber in der Map enthalten.
// ==> Können wir auf eine Ca3DEWorld verzichten?? Antwort: Nein, wegen dem EntityServiceInterface! (?)
class Ca3DEWorldT : public cf::GameSys::GameWorldI
{
    private:

    // This must unfortunately be above the public stuff below, or else the ctor inits the members in the wrong order!
    const WorldT* World;


    public:

    Ca3DEWorldT(const char* FileName, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction) /*throw (WorldT::LoadErrorT)*/;
    ~Ca3DEWorldT();

    // Decorator Pattern: Mimic the public interface of the original (Loadable)WorldT
    cf::SceneGraph::BspTreeNodeT* const&       BspTree;
    cf::ClipSys::CollisionModelStaticT* const& CollModel;
    const ArrayT<InfoPlayerStartT>&            InfoPlayerStarts;
    const cf::SceneGraph::LightMapManT&        LightMapMan;
    const cf::SceneGraph::SHLMapManT&          SHLMapMan;
    const ArrayT<GameEntityT*>&                GameEntities;

    // Decorations
    cf::ClipSys::ClipWorldT*  ClipWorld;        // This should eventually be moved into the game DLL code, but as Cl & Sv still have separate worlds, we need it here.
    EntityManagerT*           EntityManager;

    // The virtual methods inherited from the base class GameWorldI.
    cf::ClipSys::ClipWorldT&     GetClipWorld();
    Vector3fT                    GetAmbientLightColorFromBB(const BoundingBox3T<double>& Dimensions, const VectorT& Origin) const;
    const ArrayT<unsigned long>& GetAllEntityIDs() const;
    BaseEntityT*                 GetBaseEntityByID(unsigned long EntityID) const;
    unsigned long                CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin);
    void                         RemoveEntity(unsigned long EntityID);


    private:

    Ca3DEWorldT(const Ca3DEWorldT&);            ///< Use of the Copy Constructor    is not allowed.
    void operator = (const Ca3DEWorldT&);       ///< Use of the Assignment Operator is not allowed.
};

#endif
