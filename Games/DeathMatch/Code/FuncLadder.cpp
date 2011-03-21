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

#include "FuncLadder.hpp"
#include "EntityCreateParams.hpp"
#include "PhysicsWorld.hpp"
#include "Libs/LookupTables.hpp"
#include "ClipSys/CollisionModel_base.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "TypeSys.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntFuncLadderT::GetType() const
{
    return &TypeInfo;
 // return &EntFuncLadderT::TypeInfo;
}

void* EntFuncLadderT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntFuncLadderT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntFuncLadderT::TypeInfo(GetBaseEntTIM(), "EntFuncLadderT", "BaseEntityT", EntFuncLadderT::CreateInstance, NULL /*MethodsList*/);


EntFuncLadderT::EntFuncLadderT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  EntityStateT(VectorT(),   // Origin
                               VectorT(),   // Velocity
                               BoundingBox3T<double>(Vector3dT()),
                               0,           // Heading
                               0,           // Pitch
                               0,           // Bank
                               0,
                               0,
                               0,           // ModelIndex
                               0,           // ModelSequNr
                               0.0,         // ModelFrameNr
                               0,           // Health
                               0,           // Armor
                               0,           // HaveItems
                               0,           // HaveWeapons
                               0,           // ActiveWeaponSlot
                               0,           // ActiveWeaponSequNr
                               0.0))        // ActiveWeaponFrameNr
   // RootNode(Params.RootNode)
{
    assert(CollisionModel!=NULL);   // A ladder entity without collision model is useless.

    // Registering the clip model with the clip world is very important, so that we cannot run "into" ladder brushes.
    // !!! Note that ladder brushes are currently always at the origin (0, 0, 0).
    // !!! That is the only reason why we don't need to call e.g. ClipModel.SetOrigin()
    // !!! AND don't need to provide the Cl_UnserializeFrom() method!
    ClipModel.Register();


    // Corresponding to this entities CollisionModelT, use the related btCollisionShape
    // for adding a btRigidBody (which "is a" btCollisionObject) to the PhysicsWorld.
    btCollisionShape* LadderShape=CollisionModel->GetBulletAdapter();

    m_RigidBody=new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(0, NULL /*MotionState*/, LadderShape, btVector3(0, 0, 0)));
    m_RigidBody->setUserPointer(this);  // This entity is associated to the m_RigidBody.

    PhysicsWorld->AddRigidBody(m_RigidBody);
}


EntFuncLadderT::~EntFuncLadderT()
{
    // Remove our ladder body from the physics world again and then delete it.
    PhysicsWorld->RemoveRigidBody(m_RigidBody);

    delete m_RigidBody;
}


/* bool EntFuncLadderT::IsOnLadder(const EntityStateT& PlayerState, Vector3dT& ImpactNormal) const
{
    cf::ClipSys::TraceResultT Result(1.0);
    cf::ClipSys::TraceSolidT  tm(PlayerState.Dimensions);

    // ViewDir should probably be perpendicular to the gravity vector...
    const Vector3fT ViewDir=Vector3fT(LookupTables::Angle16ToSin[PlayerState.Heading], LookupTables::Angle16ToCos[PlayerState.Heading], 0.0f);
    const Vector3fT Start  =PlayerState.Origin.AsVectorOfFloat();
    const Vector3fT End    =Start+ViewDir*20.0f;


    ClipModel.Translation(&Result, Start, End, &tm, cf::math::Matrix3x3T<float>::Identity, MaterialT::Clip_Players);

    if (Result.fraction<1.0f)
    {
        ImpactNormal=Result.c.normal.AsVectorOfDouble();
        return true;
    }

    return false;
} */


/* void EntFuncLadderT::Draw(float, bool)
    const ArrayT<BrushT>* Brushes;

    GameWorld->GetMapFileData(MapFileID, &Brushes, NULL, NULL);

    if (Brushes==NULL) return;

    // This code is awfully dumb and slow,
    // and just exists for the purpose of debugging.
    for (unsigned long BrushNr=0; BrushNr<Brushes->Size(); BrushNr++)
    {
        ArrayT< Polygon3T<double> > Polys;

        for (unsigned long PlaneNr=0; PlaneNr<(*Brushes)[BrushNr].Planes.Size(); PlaneNr++)
        {
            Polys.PushBackEmpty();
            Polys[Polys.Size()-1].Plane=(*Brushes)[BrushNr].Planes[PlaneNr];
        }

        PolygonComplete(Polys);

        glScalef(1.0/25.4, 1.0/25.4, 1.0/25.4);
        glRotatef(90, 1.0, 0.0, 0.0);
        glRotatef(-90.0, 0.0, 1.0, 0.0);

        glColor3f(1.0, 0.0, 0.0);
        glDisable(GL_TEXTURE_2D);

        for (unsigned long PolyNr=0; PolyNr<Polys.Size(); PolyNr++)
        {
            glBegin(GL_LINE_LOOP);
                for (unsigned long VertexNr=0; VertexNr<Polys[PolyNr].Vertices.Size(); VertexNr++)
                {
                    glVertex3f(Polys[PolyNr].Vertices[VertexNr].x, Polys[PolyNr].Vertices[VertexNr].z, -Polys[PolyNr].Vertices[VertexNr].y);
                    // GameWorld->PrintDebug("%f %f %f", Polys[PolyNr].Vertices[VertexNr].x, Polys[PolyNr].Vertices[VertexNr].y, Polys[PolyNr].Vertices[VertexNr].z);
                }
            glEnd();
        }

        glEnable(GL_TEXTURE_2D);
    }
} */
