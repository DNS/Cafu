/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _CF_CLIPSYS_CLIPMODEL_HPP_
#define _CF_CLIPSYS_CLIPMODEL_HPP_

#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Math3D/Vector3.hpp"


namespace cf
{
    namespace ClipSys
    {
        class  CollisionModelT;
        class  ClipLinkT;
        class  ClipWorldT;
        struct TraceResultT;
        class  TraceSolidT;


        /// A clip model represents an object in the world against which clipping queries can be performed.
        /// Normally, clip models are inserted into a ClipWorldT instance and are then used via the clip world.
        class ClipModelT
        {
            public:

            /// The constructor for creating a clip model.
            ClipModelT(const ClipWorldT& ClipWorld_, const CollisionModelT* CollisionModel_=NULL);

            /// The destructor.
            ~ClipModelT();

            /// Registers this clip model with the clip world.
            void Register();

            /// Unregisters this clip model from the clip world.
            void Unregister();

            // /// Returns the collision model of this clip model.
            // const CollisionModelT* GetCollisionModel() const { return CollisionModel; }

            /// Sets the given collision model for use with (as the basis of) this clip model.
            /// Unregisters from the clip world first.
            void SetCollisionModel(const CollisionModelT* CollisionModel_);

            /// Returns the origin of this clip model.
            const Vector3dT& GetOrigin() const { return Origin; }

            /// Sets a new origin for this clip model.
            /// Unregisters the model from the clip world before updating the origin, but does *not* re-register it!
            /// It is up to the user to call Register() again to re-register to model in the clip world.
            void SetOrigin(const Vector3dT& NewOrigin);

            /// Returns the orientation (axes, rotation matrix) for this clip model.
            const math::Matrix3x3T<double>& GetOrientation() const { return Orientation; }

            /// Sets a new orientation (axes, rotation matrix) for this clip model.
            /// Unregisters the model from the clip world before updating the orientation, but does *not* re-register it!
            /// It is up to the user to call Register() again to re-register to model in the clip world.
            void SetOrientation(const math::Matrix3x3T<double>& NewOrientation);

            /// Returns the user data associated with this clip model.
            /// This is usually the pointer to the entity that "owns" this clip model, i.e. this clip model is a member of this entity.
            void* GetUserData() const { return UserData; }

            /// Sets the user data associated with this clip model.
            /// @param UD   The new user data to be associated with this clip model.
            /// @see GetUserData()
            void SetUserData(void* UD);

            /// Returns the bounding box of this clip model in absolute world coordinates.
            /// Note that the clip model must be registered (linked) with the clip world when this method is called.
            const BoundingBox3dT& GetAbsoluteBB() const;

            /// Returns the contents of this clip model, which is the forwarded contents of the underlying collision model.
            unsigned long GetContents() const;

            /// Traces the given bounding box from Start along Ray (up to the input value of Result.Fraction) through the clip model,
            /// and reports the first collision, if any.
            /// @param TraceBB   The bounding box to trace through the model.
            /// @param Start     The start point in world space where the trace begins.
            /// @param Ray       The ray along which the trace is performed. Note that with F being the input value of Result.Fraction, the endpoint is at Start+Ray*F.
            /// @param ClipMask  Only surfaces whose clip flags match this mask participate in the test. This is for optimization, because it allows the implementation to cull surfaces that are not of interest early.
            /// @param Result    The start value of Fraction is input via this reference, and the result of the trace returned.
            ///     Using an input/output parameter for returning the result, rather than a true return type, suggests itself because it makes
            ///     cascaded calls to this function natural (i.e. from (possibly many) super-objects and to (possibly many) sub-objects).
            /// @see TraceResultT
            void TraceBoundingBox(const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;

            void TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;

            void TraceRay(const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;

            /// Determines the volume contents of the model at the given point / in the given box.
            /// The function considers all brush volumes in the clip model that contain the given point or intersect the given box,
            /// and returns their combined ("or'ed") contents.
            /// @param Point       The point in world space at which the volume contents of this model is to be determined.
            /// @param BoxRadius   When nonzero, this is the radius of an imaginary box around Point.
            /// @param ContMask    Only volumes whose contents matches this mask participate in the test. This is for optimization, because it allows the implementation to cull volumes that are not of interest early.
            /// @returns the combined volume content flags of the intersected volumes in the clip model.
            //      If the Point (and thus the associated box) is outside of all volumes, 0 is returned.
            unsigned long GetContents(const Vector3dT& Point, double BoxRadius, unsigned long ContMask) const;


            private:

            friend class ClipWorldT;

            ClipModelT(const ClipModelT&);          ///< Use of the Copy Constructor    is not allowed.
            void operator = (const ClipModelT&);    ///< Use of the Assignment Operator is not allowed.

            const ClipWorldT&        ClipWorld;
            const CollisionModelT*   CollisionModel;
            Vector3T<double>         Origin;        ///< The translation of the collision model that positions it in world space. Origin and Orientation together form a matrix M that transforms a point from model to world space.
            math::Matrix3x3T<double> Orientation;   ///< The local axes  of the collision model that positions it in world space. Origin and Orientation together form a matrix M that transforms a point from model to world space.
            void*                    UserData;      ///< This is usually the pointer to the entity that "owns" this clip model, i.e. this clip model is a member of this entity.
            BoundingBox3dT           AbsoluteBB;    ///< The world-space bounding box of the collision model, i.e. with Transformation applied.
            ClipLinkT*               ListOfSectors; ///< The list of sectors that this model is in.
            bool                     IsEnabled;     ///< Whether this model is enabled for clipping or not. When disabled, the model is not taken into account even while being registered with the clip world.
            bool                     AlreadyChecked;///< For sole use by the ClipWorldT::GetClipModelsFromBB() method only, should always be false outside of that method.
        };
    }
}

#endif
