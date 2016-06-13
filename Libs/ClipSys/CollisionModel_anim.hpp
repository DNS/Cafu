/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_COLLISION_MODEL_ANIM_HPP_INCLUDED
#define CAFU_CLIPSYS_COLLISION_MODEL_ANIM_HPP_INCLUDED

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
            BoundingBox3dT GetBoundingBox() const override;
            unsigned long GetContents() const override;
            void SaveToFile(std::ostream& OutFile, SceneGraph::aux::PoolT& Pool) const override;
            void TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const override;
            unsigned long GetContents(const Vector3dT& Point, double BoxRadius, unsigned long ContMask) const override;
            btCollisionShape* GetBulletAdapter() const override;


            private:
        };
    }
}

#endif
