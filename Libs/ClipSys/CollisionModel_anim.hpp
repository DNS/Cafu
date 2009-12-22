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

#ifndef _CF_CLIPSYS_COLLISION_MODEL_ANIM_HPP_
#define _CF_CLIPSYS_COLLISION_MODEL_ANIM_HPP_

#include "CollisionModel_base.hpp"


namespace cf
{
    namespace ClipSys
    {
        /// This class represents an animated collision model.
        /// Its main advantage is that it supports collision models that are animated,
        /// its main disadvantage is the limitations with which many types of queries can only be answered.
        class CollisionModelAnimT : public CollisionModelT
        {
            public:

            /// The constructor.
            CollisionModelAnimT();

            /// The destructor.
            ~CollisionModelAnimT();


            // The CollisionModelT interface.
            BoundingBox3dT GetBoundingBox() const;
            unsigned long GetContents() const;
            void SaveToFile(std::ostream& OutFile, SceneGraph::aux::PoolT& Pool) const;
            void TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;
            void TraceRay(const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;
            unsigned long GetContents(const Vector3dT& Point, double BoxRadius, unsigned long ContMask) const;
            btCollisionShape* GetBulletAdapter() const;


            private:
        };
    }
}

#endif
