/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_PHYSICS_WORLD_HPP_INCLUDED
#define CAFU_PHYSICS_WORLD_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"   // Temporary for PhysicsWorldT::TraceBoundingBox().
#include "Math3D/Quaternion.hpp"
#include "Math3D/Vector3.hpp"
#include "btBulletDynamicsCommon.h"

#undef min    // See http://stackoverflow.com/questions/5004858/stdmin-gives-error
#undef max


namespace cf { namespace ClipSys { class CollisionModelT; } }
namespace cf { namespace GameSys { class ComponentPhysicsT; } }
typedef cf::GameSys::ComponentPhysicsT TraceUserEntityT;


// Auxiliary functions for making the conversion between Bullet and Cafu vectors easier.
inline btVector3 conv(const Vector3T<float>& v) { return btVector3(v.x, v.y, v.z); }
inline btVector3 conv(const Vector3T<double>& v) { return btVector3(float(v.x), float(v.y), float(v.z)); }
inline Vector3fT conv(const btVector3& v) { return Vector3fT(v.x(), v.y(), v.z()); }
inline Vector3dT convd(const btVector3& v) { return Vector3dT(v.x(), v.y(), v.z()); }

inline cf::math::QuaternionfT conv(const btQuaternion&           v) { return cf::math::QuaternionfT(v.x(), v.y(), v.z(), v.w()); }
inline btQuaternion           conv(const cf::math::QuaternionfT& v) { return btQuaternion(v.x, v.y, v.z, v.w); }

inline float     UnitsToPhys(float            v) { return v * 0.0254f; }
inline double    UnitsToPhys(double           v) { return v * 0.0254; }
inline Vector3fT UnitsToPhys(const Vector3fT& v) { return v * 0.0254f; }
inline Vector3dT UnitsToPhys(const Vector3dT& v) { return v * 0.0254; }

inline float     PhysToUnits(float            v) { return v / 0.0254f; }
inline double    PhysToUnits(double           v) { return v / 0.0254; }
inline Vector3fT PhysToUnits(const Vector3fT& v) { return v / 0.0254f; }
inline Vector3dT PhysToUnits(const Vector3dT& v) { return v / 0.0254; }


/// This class handles the results of tracing a ray through the world.
/// As such, it is called back for intermediate results and thus can "configure" or "parametrize" the trace,
/// and it keeps the final trace result for inspection by the caller as well.
class RayResultT : public btCollisionWorld::ClosestRayResultCallback
{
    public:

    /// The constructor.
    /// @param IgnoreObject   A collision object to ignore for this trace. This is usually the object from which the trace emanates.
    RayResultT(const btCollisionObject* IgnoreObject)
        : btCollisionWorld::ClosestRayResultCallback(btVector3(), btVector3()),
          m_IgnoreObject(IgnoreObject)
    {
    }


    /// If something was hit (hasHit() returns true), this method returns a pointer to the
    /// Physics component that the hit collision object belongs to.
    /// The returned pointer is NULL if the collision object belongs to the world.
    /// If nothing was hit (hasHit() returns false), NULL is always returned.
    TraceUserEntityT* GetHitPhysicsComp() const
    {
        if (m_collisionObject==NULL) return NULL;

        return static_cast<TraceUserEntityT*>(m_collisionObject->getUserPointer());
    }


    virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& RayResult, bool NormalIsInWorldSpace)
    {
        if (RayResult.m_collisionObject==m_IgnoreObject) return 1.0;

        return ClosestRayResultCallback::addSingleResult(RayResult, NormalIsInWorldSpace);
    }


    protected:

    const btCollisionObject* m_IgnoreObject;
};


/// This class handles the results of tracing a convex shape through the world.
/// As such, it is called back for intermediate results and thus can "configure" or "parametrize" the trace,
/// and it keeps the final trace result for inspection by the caller as well.
class ShapeResultT : public btCollisionWorld::ClosestConvexResultCallback
{
    public:

    /// The constructor.
    ShapeResultT()
        : btCollisionWorld::ClosestConvexResultCallback(btVector3(), btVector3()),
          m_IgnoreObjCount(0),
          m_IgnoreEntCount(0)
    {
    }


    /// Adds the given collision object to the objects to be ignored for this trace.
    /// @param Obj   A collision object to ignore for this trace. This is often the object from which the trace emanates.
    void Ignore(const btCollisionObject* Obj)
    {
        assert(m_IgnoreObjCount<2);
        if (m_IgnoreObjCount>=2) return;

        m_IgnoreObjects[m_IgnoreObjCount++]=Obj;
    }


    /// Adds the given entity to the entities to be ignored for this trace.
    /// @param Ent   An entity to ignore for this trace. This is often the entity from which the trace emanates, or e.g. NULL (the world).
    void Ignore(const TraceUserEntityT* Ent)
    {
        assert(m_IgnoreEntCount<2);
        if (m_IgnoreEntCount>=2) return;

        m_IgnoreEntities[m_IgnoreEntCount++]=Ent;
    }


    /// If something was hit (hasHit() returns true), this method returns a pointer to the
    /// Physics component that the hit collision object belongs to.
    /// The returned pointer is NULL if the collision object belongs to the world.
    /// If nothing was hit (hasHit() returns false), NULL is always returned.
    TraceUserEntityT* GetHitPhysicsComp() const
    {
        if (m_hitCollisionObject==NULL) return NULL;

        return static_cast<TraceUserEntityT*>(m_hitCollisionObject->getUserPointer());
    }


    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& ConvexResult, bool NormalIsInWorldSpace)
    {
        for (unsigned short Count=0; Count<m_IgnoreObjCount; Count++)
            if (ConvexResult.m_hitCollisionObject==m_IgnoreObjects[Count]) return 1.0;

        for (unsigned short Count=0; Count<m_IgnoreEntCount; Count++)
            if (ConvexResult.m_hitCollisionObject->getUserPointer()==m_IgnoreEntities[Count]) return 1.0;

        return ClosestConvexResultCallback::addSingleResult(ConvexResult, NormalIsInWorldSpace);
    }


    protected:

    unsigned short           m_IgnoreObjCount;
    unsigned short           m_IgnoreEntCount;

    const btCollisionObject* m_IgnoreObjects[2];
    const TraceUserEntityT*  m_IgnoreEntities[2];
};


class PhysicsWorldT
{
    public:

    PhysicsWorldT(const cf::ClipSys::CollisionModelT* WorldCollMdl);
    ~PhysicsWorldT();

    void AddRigidBody(btRigidBody* RigidBody);
    void RemoveRigidBody(btRigidBody* RigidBody);

    void TraceRay(const Vector3dT& Origin, const Vector3dT& Ray, RayResultT& RayResult) const;
    void TraceBoundingBox(const BoundingBox3T<double>& BB, const Vector3dT& Origin, const Vector3dT& Dir, ShapeResultT& ShapeResult) const;
    void TraceShape() const;

    void Think(float FrameTime);


    private:

    btDefaultCollisionConfiguration* m_CollisionConfiguration;
    btCollisionDispatcher*           m_Dispatcher;
    btBroadphaseInterface*           m_Broadphase;
    btConstraintSolver*              m_Solver;
    btDiscreteDynamicsWorld*         m_PhysicsWorld;
};

#endif
