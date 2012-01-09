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

#ifndef _COMPANY_BOT_HPP_
#define _COMPANY_BOT_HPP_

#include "../../BaseEntity.hpp"
#include "btBulletDynamicsCommon.h"


class CafuModelT;
class EntityCreateParamsT;


class EntCompanyBotT : public BaseEntityT, public btMotionState
{
    public:

    EntCompanyBotT(const EntityCreateParamsT& Params);
    ~EntCompanyBotT();

    // Implement the BaseEntityT interface.
    void TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir);
    void Think(float FrameTime, unsigned long ServerFrameNr);

    void Cl_UnserializeFrom();
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

    const CafuModelT* m_CompanyBotModel;
    const CafuModelT* m_WeaponModel;
    float             m_TimeForLightSource;

    btCollisionShape* m_CollisionShape;     ///< The collision shape that is used to approximate and represent this player in the physics world.
    btRigidBody*      m_RigidBody;          ///< The rigid body (of "kinematic" type) for use in the physics world.
};

#endif
