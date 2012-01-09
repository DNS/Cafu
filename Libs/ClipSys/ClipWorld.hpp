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

#ifndef CAFU_CLIPSYS_CLIPWORLD_HPP_INCLUDED
#define CAFU_CLIPSYS_CLIPWORLD_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"


namespace cf
{
    namespace ClipSys
    {
        class  ClipModelT;
        class  ClipSectorT;
        class  CollisionModelT;
        struct ContactsResultT;
        struct TraceResultT;
        class  TraceSolidT;


        /// The clip world manages all the clip models that exist in a world (their "union").
        /// This is done in a very efficient manner, even if clip models change (i.e. move, rotate, morph (update geometry) etc.) over time.
        class ClipWorldT
        {
            public:

            /// The constructor.
            ClipWorldT(const CollisionModelT* WorldCollMdl_);

            /// The destructor.
            ~ClipWorldT();

            /// Traces the given bounding box from Start along Ray (up to the input value of Result.Fraction) through the clip world,
            /// and reports the first collision, if any.
            /// @param TraceBB   The bounding box to trace through the world.
            /// @param Start     The start point in world space where the trace begins.
            /// @param Ray       The ray along which the trace is performed. Note that with F being the input value of Result.Fraction, the endpoint is at Start+Ray*F.
            /// @param ClipMask  Only surfaces whose clip flags match this mask participate in the test. This is for optimization, because it allows the implementation to cull surfaces that are not of interest early.
            /// @param Ignore    A clip model that is to be ignored during the trace, even if the content mask matches.
            ///                  This is normally used to "hide" the clip model from which the trace emanates in order to prevent it colliding with itself.
            /// @param Result    The start value of Fraction is input via this reference, and the result of the trace returned.
            ///     Using an input/output parameter for returning the result, rather than a true return type, suggests itself because it makes
            ///     cascaded calls to this function natural (i.e. from (possibly many) super-objects and to (possibly many) sub-objects).
            /// @param HitClipModel   A pointer to the clip model instance with which the reported collision occurred, or NULL if there was no collision.
            /// @see TraceResultT
            void TraceBoundingBox(const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray,
                unsigned long ClipMask, const ClipModelT* Ignore, TraceResultT& Result, ClipModelT** HitClipModel=NULL) const;

            void TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray,
                unsigned long ClipMask, const ClipModelT* Ignore, TraceResultT& Result, ClipModelT** HitClipModel=NULL) const;

            void TraceRay(const Vector3dT& Start, const Vector3dT& Ray,
                unsigned long ClipMask, const ClipModelT* Ignore, TraceResultT& Result, ClipModelT** HitClipModel=NULL) const;

            /// Determines the set of clip models that touch a given bounding-box and meet a given contents mask.
            ///   NOTE: The return list is always exclusive the world, that is, the world clip model is *never* mentioned in the list, even if it meets all criteria otherwise.
            /// @param ClipModels    The method returns the found clip models by appending them to this list. Note: The list is *always* exclusive the "world" clip model!
            /// @param ContentMask   The content filter mask; only clip models that meet this content mask are returned.
            /// @param BB            The bounding-box to be queried for clip models.
            void GetClipModelsFromBB(ArrayT<ClipModelT*>& ClipModels, unsigned long ContentMask, const BoundingBox3dT& BB) const;

            /// Determines all places of contact between all clip models in this world and the given trace model
            /// that would occur when the trace model was translated from trmOrig into direction trmMoveDir by trmMoveAmount.
            ///
            /// @returns This methods returns its results in the two parallel arrays Contacts and ClipModels,
            ///          where each contact that is described by FoundContacts[i] occurred with the clip model pointed to by FoundClipModels[i].
            void GetContacts(const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray,
                unsigned long ClipMask, const ClipModelT* Ignore, ContactsResultT& Contacts) const;

            void GetContacts(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray,
                unsigned long ClipMask, const ClipModelT* Ignore, ContactsResultT& Contacts) const;

            void GetContacts(const Vector3dT& Start, const Vector3dT& Ray,
                unsigned long ClipMask, const ClipModelT* Ignore, ContactsResultT& Contacts) const;


            private:

            friend class ClipModelT;

            // Private interface (also for use by ClipModelT friend class).
            void GetGridRectFromBB(unsigned long GridRect[], const BoundingBox3dT& BB) const;

            ClipWorldT(const ClipWorldT&);          ///< Use of the Copy Constructor    is not allowed.
            void operator = (const ClipWorldT&);    ///< Use of the Assignment Operator is not allowed.

            const CollisionModelT* WorldCollMdl;    ///< The collision model for the world. Never explicitly kept in the Sectors! Should probably be of type ClipModelT though!
            const BoundingBox3dT   WorldBB;         ///< The absolute dimensions of the world.
            const unsigned long    SectorSubdivs;   ///< The number of clip sectors along each side of the uniform grid.
            const Vector3dT        SectorSideLen;   ///< The side lengths of each sector.
            ClipSectorT*           Sectors;         ///< The clip sectors that cover the world (as a uniform grid of SectorSubdivs*SectorSubdivs cells).
        };
    }
}

#endif
