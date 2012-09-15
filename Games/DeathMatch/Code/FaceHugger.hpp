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

#ifndef CAFU_FACEHUGGER_HPP_INCLUDED
#define CAFU_FACEHUGGER_HPP_INCLUDED

#include "BaseEntity.hpp"
#include "Libs/Physics.hpp"


class CafuModelT;


namespace GAME_NAME
{
    class EntFaceHuggerT : public BaseEntityT
    {
        public:

        EntFaceHuggerT(const EntityCreateParamsT& Params);

        void SetHeading(unsigned short h) { m_Heading = h; }
        void SetVelocity(const Vector3dT& v) { m_Velocity = v; }

        void Think(float FrameTime, unsigned long ServerFrameNr);
        void Draw(bool FirstPersonView, float LodDist) const;
        void PostDraw(float FrameTime, bool FirstPersonView);


        const cf::TypeSys::TypeInfoT* GetType() const;
        static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
        static const cf::TypeSys::TypeInfoT TypeInfo;


        private:

        // Override the base class methods.
        void DoSerialize(cf::Network::OutStreamT& Stream) const;
        void DoDeserialize(cf::Network::InStreamT& Stream);

        Vector3dT         m_Velocity;
        PhysicsHelperT    m_Physics;

        const CafuModelT* m_Model;
        const int         m_ModelSequNr;
        float             m_ModelFrameNr;
    };
}

#endif
