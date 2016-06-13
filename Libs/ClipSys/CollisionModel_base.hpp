/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_COLLISION_MODEL_BASE_HPP_INCLUDED
#define CAFU_CLIPSYS_COLLISION_MODEL_BASE_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "Math3D/BoundingBox.hpp"


class btCollisionShape;


namespace cf
{
    namespace SceneGraph { namespace aux { class PoolT; } }


    namespace ClipSys
    {
        struct TraceResultT;
        class  TraceSolidT;


        /// This is the base class for collision models, defining their common interface.
        class CollisionModelT
        {
            public:

            /// The (virtual) destructor.
            virtual ~CollisionModelT() { }

            /// Returns the bounding box of this collision model.
            virtual BoundingBox3dT GetBoundingBox() const = 0;

            /// Returns the contents of this collision model.
            virtual unsigned long GetContents() const = 0;

            /// Saves the model to OutFile.
            /// TODO: Review serialization/deser. of class hierarchies (e.g. compare to cf::SceneGraph)!
            ///       Right now this is fixed and works for CollisionModelStaticTs only!!!
            virtual void SaveToFile(std::ostream& OutFile, SceneGraph::aux::PoolT& Pool) const = 0;

            /// Traces the given TraceSolidT instance from Start along Ray (up to the input value of Result.Fraction)
            /// through the collision model, and reports the first collision, if any.
            ///
            /// @param TraceSolid
            ///     The TraceSolidT instance that is traced from Start along Ray.
            ///
            /// @param Start
            ///     The start point in model space where the trace begins.
            ///
            /// @param Ray
            ///     The ray along which the trace is performed. Note that with F being the input value of Result.Fraction, the endpoint is at Start+Ray*F.
            ///
            /// @param ClipMask
            ///     Only surfaces whose clip flags match this mask participate in the test.
            ///     This is for optimization, because it allows the implementation to cull surfaces that are not of interest early.
            ///
            /// @param Result
            ///     The start value of Fraction is input via this reference, and the result of the trace returned.
            ///     Using an input/output parameter for returning the result, rather than a true return type, suggests itself because it makes
            ///     cascaded calls to this function natural (i.e. from (possibly many) super-objects and to (possibly many) sub-objects).
            ///
            /// @see TraceResultT
            virtual void TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const = 0;

            /// Determines the volume contents of the model at the given point / in the given box.
            /// The function considers all brush volumes in the collision model that contain the given point or intersect the given box,
            /// and returns their combined ("or'ed") contents.
            /// @param Point       The point in model space at which the volume contents of this model is to be determined.
            /// @param BoxRadius   When nonzero, this is the radius of an imaginary box around Point.
            /// @param ContMask    Only volumes whose contents matches this mask participate in the test. This is for optimization, because it allows the implementation to cull volumes that are not of interest early.
            /// @returns the combined volume content flags of the intersected volumes in the collision model.
            //      If the Point (and thus the associated box) is outside of all volumes, 0 is returned.
            virtual unsigned long GetContents(const Vector3dT& Point, double BoxRadius, unsigned long ContMask) const = 0;

            /// Returns an adapter class for using CollisionModelT instances also as Bullet btCollisionShape instances.
            virtual btCollisionShape* GetBulletAdapter() const = 0;
        };
    }
}

#endif
