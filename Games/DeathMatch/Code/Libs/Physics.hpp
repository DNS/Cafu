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

#ifndef CAFU_PHYSICS_HPP_INCLUDED
#define CAFU_PHYSICS_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"


namespace cf { namespace ClipSys { class ClipModelT; } }
namespace cf { namespace ClipSys { class ClipWorldT; } }


namespace GAME_NAME
{
    /// This class implements the physics for moving entities through the world.
    class PhysicsHelperT
    {
        public:

        PhysicsHelperT(Vector3dT& Origin, Vector3dT& Velocity, const BoundingBox3dT& Dimensions,
                       const cf::ClipSys::ClipModelT& ClipModel, const cf::ClipSys::ClipWorldT& ClipWorld);

        void MoveHuman(float FrameTime, unsigned short Heading, const VectorT& WishVelocity,
                       const VectorT& WishVelLadder, bool WishJump, bool& OldWishJump, double StepHeight);


        private:

        enum PosCatT { InAir, OnSolid };

        PosCatT CategorizePosition() const;
        void    ApplyFriction(double FrameTime, PosCatT PosCat);
        void    ApplyAcceleration(double FrameTime, PosCatT PosCat, const VectorT& WishVelocity);
        void    ApplyGravity(double FrameTime, PosCatT PosCat);
        void    FlyMove(double TimeLeft);
        void    GroundMove(double FrameTime, double StepHeight);

        Vector3dT&                     m_Origin;
        Vector3dT&                     m_Velocity;
        const BoundingBox3dT&          m_Dimensions;
        const cf::ClipSys::ClipModelT& m_ClipModel;
        const cf::ClipSys::ClipWorldT& m_ClipWorld;
    };
}

#endif
