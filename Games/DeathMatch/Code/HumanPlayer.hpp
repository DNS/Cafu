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

#ifndef CAFU_HUMAN_PLAYER_HPP_INCLUDED
#define CAFU_HUMAN_PLAYER_HPP_INCLUDED

#include "BaseEntity.hpp"
#include "../../PlayerCommand.hpp"
#include "Libs/Physics.hpp"
#include "btBulletDynamicsCommon.h"


class EntityCreateParamsT;
class EntStaticDetailModelT;
namespace cf { namespace GuiSys { class GuiI; } }


class EntHumanPlayerT : public BaseEntityT, public btMotionState
{
    public:

    // Publicly defined enum for access from the "carried weapons".
    enum EventTypesT { EVENT_TYPE_PRIMARY_FIRE, EVENT_TYPE_SECONDARY_FIRE, NUM_EVENT_TYPES };

    EntHumanPlayerT(const EntityCreateParamsT& Params);
    ~EntHumanPlayerT();

    EntityStateT& GetState() { return State; }
    const EntityStateT& GetState() const { return State; }
    unsigned short GetPitch() const { return m_Pitch; }
    unsigned short GetBank() const { return m_Bank; }
    const btRigidBody* GetRigidBody() const { return m_RigidBody; }

    /// Increases the frag count of this entity by the given number.
    void AddFrag(int NumFrags=1);

    // Implement the BaseEntityT interface.
    void TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir);
    void ProcessConfigString(const void* ConfigData, const char* ConfigString);
    void Think(float FrameTime, unsigned long ServerFrameNr);

    void ProcessEvent(unsigned int EventType, unsigned int NumEvents);
    bool GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const;
    void Draw(bool FirstPersonView, float LodDist) const;
    void PostDraw(float FrameTime, bool FirstPersonView);

    // Implement the btMotionState interface.
    void getWorldTransform(btTransform& worldTrans) const;
    void setWorldTransform(const btTransform& worldTrans);


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    // Override the base class methods.
    void DoSerialize(cf::Network::OutStreamT& Stream) const;
    void DoDeserialize(cf::Network::InStreamT& Stream);

    /// A helper function for Think().
    bool CheckGUI(IntrusivePtrT<EntStaticDetailModelT> GuiEnt, Vector3fT& MousePos) const;

    ArrayT<PlayerCommandT> PlayerCommands;

    EntityStateT      State;                    ///< The current state of this entity.
    PhysicsHelperT    m_Physics;
    btCollisionShape* m_CollisionShape;         ///< The collision shape that is used to approximate and represent this player in the physics world.
    btRigidBody*      m_RigidBody;              ///< The rigid body (of "kinematic" type) for use in the physics world.

 // char              ThisHumanPlayerNum;       // The sole purpose is to help to make a good descision about the light source color in GetLightSourceInfo().
    mutable VectorT   LastSeenAmbientColor;     // This is a client-side variable, unrelated to prediction, and thus allowed.
    float             TimeForLightSource;
    cf::GuiSys::GuiI* GuiHUD;                   ///< The HUD GUI for this local human player entity.
};

#endif
