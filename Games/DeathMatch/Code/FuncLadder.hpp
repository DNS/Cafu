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

#ifndef _LADDER_HPP_
#define _LADDER_HPP_

#include "../../BaseEntity.hpp"


class btRigidBody;
class EntityCreateParamsT;


class EntFuncLadderT : public BaseEntityT
{
    public:

    EntFuncLadderT(const EntityCreateParamsT& Params);
    ~EntFuncLadderT();

    /// Returns whether the bounding-box AbsBB (which is given in absolute ("world") space) is "on" this ladder.
    /// If so, true is returned and the ImpactNormal is set to the normal of the plane of impact.
    /// Otherwise, false is returned and the value of ImpactNormal is unchanged.
 // With svn revision 365, this method has become obsolete. It's just left in for reference for a while.
 // bool IsOnLadder(const EntityStateT& State, Vector3dT& ImpactNormal) const;


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

 // const cf::SceneGraph::GenericNodeT* RootNode;   ///< The root node of the scene graph of the model (brushwork) of this entity.
    btRigidBody* m_RigidBody;
};

#endif
