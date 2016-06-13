/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "PhysicsWorld.hpp"

#include "MapFile.hpp"
#include "ClipSys/CollisionModel_base.hpp"
#include "ConsoleCommands/Console.hpp"
#include "Math3D/Polygon.hpp"
#include "TextParser/TextParser.hpp"

// #include <iostream>     // For std::cout debug output

// Turn off bogus warnings that occur with VC11's static code analysis.
// (Should move this to a better place though, e.g. some `compat.h` file...)
#if defined(_WIN32) && defined(_MSC_VER)
    // warning C28182: Dereferencing NULL pointer.
    #pragma warning(disable:28182)
#endif


using namespace cf;


PhysicsWorldT::PhysicsWorldT(const cf::ClipSys::CollisionModelT* WorldCollMdl)
    : m_CollisionConfiguration(NULL),
      m_Dispatcher(NULL),
      m_Broadphase(NULL),
      m_Solver(NULL),
      m_PhysicsWorld(NULL)
{
    m_CollisionConfiguration=new btDefaultCollisionConfiguration();
    m_Dispatcher            =new btCollisionDispatcher(m_CollisionConfiguration);
    m_Broadphase            =new btDbvtBroadphase();
    m_Solver                =new btSequentialImpulseConstraintSolver();
    m_PhysicsWorld          =new btDiscreteDynamicsWorld(m_Dispatcher, m_Broadphase, m_Solver, m_CollisionConfiguration);

    m_PhysicsWorld->setGravity(btVector3(0, 0, -9.81f));


    // Corresponding to the worlds CollisionModelT, use the related btCollisionShape
    // for adding a btRigidBody (which "is a" btCollisionObject) to the PhysicsWorld.
    // Note that entities other than the world add their rigid bodies theirselves upon their instantiation.
    btCollisionShape* WorldShape     =WorldCollMdl->GetBulletAdapter();
    btRigidBody*      WorldStaticBody=new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(0, NULL /*myMotionState*/, WorldShape, btVector3(0, 0, 0)));

    WorldStaticBody->setUserPointer(NULL);      // There is no BaseEntityT instance associated to this rigid body.

    m_PhysicsWorld->addRigidBody(WorldStaticBody);
}


PhysicsWorldT::~PhysicsWorldT()
{
    // Clean-up in the reverse order of creation/initialization.
    // This assumes that all entities other than the world have already removed their stuff.

    // Remove the rigid bodies from the dynamics world and delete them.
    for (int CONr=m_PhysicsWorld->getNumCollisionObjects()-1; CONr>=0; CONr--)
    {
        btCollisionObject* obj=m_PhysicsWorld->getCollisionObjectArray()[CONr];

        assert(btRigidBody::upcast(obj)->getMotionState()==NULL);   // Only entities (but not the world) have a proper motion state.
        assert(obj->getUserPointer()==NULL);                        // Only entities (but not the world) have a proper pointer to their instance.

        m_PhysicsWorld->removeCollisionObject(obj);
        delete obj;
    }

    delete m_PhysicsWorld;           m_PhysicsWorld=NULL;
    delete m_Solver;                 m_Solver=NULL;
    delete m_Broadphase;             m_Broadphase=NULL;
    delete m_Dispatcher;             m_Dispatcher=NULL;
    delete m_CollisionConfiguration; m_CollisionConfiguration=NULL;
}


void PhysicsWorldT::AddRigidBody(btRigidBody* RigidBody)
{
    m_PhysicsWorld->addRigidBody(RigidBody);
}


void PhysicsWorldT::RemoveRigidBody(btRigidBody* RigidBody)
{
    m_PhysicsWorld->removeRigidBody(RigidBody);
}


void PhysicsWorldT::TraceRay(const Vector3dT& Origin, const Vector3dT& Ray, RayResultT& RayResult) const
{
    // We intentionally leave it to the user to convert from world units to meters here,
    // so that the user code is more aware on the details...
    RayResult.m_rayFromWorld=conv(Origin);
    RayResult.m_rayToWorld  =conv(Origin+Ray);

    m_PhysicsWorld->rayTest(RayResult.m_rayFromWorld, RayResult.m_rayToWorld, RayResult);
}


void PhysicsWorldT::TraceBoundingBox(const BoundingBox3T<double>& BB, const VectorT& Origin, const VectorT& Dir, ShapeResultT& ShapeResult) const
{
    const double METERS_PER_WORLD_UNIT = 0.0254;

    btBoxShape Shape(conv((BB.Max-BB.Min)/2.0 * METERS_PER_WORLD_UNIT));

    // The box shape equally centered around the origin point, whereas BB is possibly "non-uniformely displaced".
    // In order to compensate, compute how far the BB center is away from the origin.
    const Vector3dT Ofs=BB.GetCenter();

    ShapeResult.m_convexFromWorld=conv((Origin+Ofs    ) * METERS_PER_WORLD_UNIT);
    ShapeResult.m_convexToWorld  =conv((Origin+Ofs+Dir) * METERS_PER_WORLD_UNIT);

    btTransform TransFrom; TransFrom.setIdentity(); TransFrom.setOrigin(ShapeResult.m_convexFromWorld);
    btTransform TransTo;   TransTo  .setIdentity(); TransTo  .setOrigin(ShapeResult.m_convexToWorld  );

    m_PhysicsWorld->convexSweepTest(&Shape, TransFrom, TransTo, ShapeResult);
}


void PhysicsWorldT::TraceShape() const
{
    ;
}


void PhysicsWorldT::Think(float FrameTime)
{
    // std::cout << __FILE__ << " (" << __LINE__ << "): Think(), " << FrameTime << "\n";
    m_PhysicsWorld->stepSimulation(FrameTime, 20);
}
