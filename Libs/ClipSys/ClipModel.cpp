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

#include "ClipModel.hpp"
#include "ClipWorld.hpp"
#include "ClipWorld_private.hpp"
#include "CollisionModel_base.hpp"
#include "TraceResult.hpp"
#include "TraceSolid.hpp"
#include "Templates/Pool.hpp"


using namespace cf::ClipSys;


static cf::PoolSingleT<ClipLinkT> ClipLinkPool(512);


ClipModelT::ClipModelT(const ClipWorldT& ClipWorld_, const CollisionModelT* CollisionModel_)
    : ClipWorld(ClipWorld_),
      CollisionModel(CollisionModel_),
      Origin(),
      Orientation(),
      UserData(NULL),
      ListOfSectors(NULL),
      IsEnabled(true),
      AlreadyChecked(false)
{
}


ClipModelT::~ClipModelT()
{
    Unregister();
}


const BoundingBox3dT& ClipModelT::GetAbsoluteBB() const
{
    // Are we registered (linked) when this method is called?
    // We should be, because only linking updates the AbsBounds member!
    assert(ListOfSectors!=NULL);

    return AbsoluteBB;
}


unsigned long ClipModelT::GetContents() const
{
    return CollisionModel!=NULL ? CollisionModel->GetContents() : 0;
}


void ClipModelT::TraceBoundingBox(const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    if (!CollisionModel) return;

    // Use the optimized point trace whenever possible.
    if (TraceBB.Min==TraceBB.Max)
    {
        // TraceBB.Min and TraceBB.Max are normally supposed to be (0, 0, 0), but let's support the generic case.
        TraceRay(Start+TraceBB.Min, Ray, ClipMask, Result);
        return;
    }


    static TraceSolidT TraceSolid(TraceBB);

    TraceBB.GetCornerVertices(&TraceSolid.Vertices[0]);

    TraceSolid.Planes[0].Dist= TraceBB.Max.x;
    TraceSolid.Planes[1].Dist=-TraceBB.Min.x;
    TraceSolid.Planes[2].Dist= TraceBB.Max.y;
    TraceSolid.Planes[3].Dist=-TraceBB.Min.y;
    TraceSolid.Planes[4].Dist= TraceBB.Max.z;
    TraceSolid.Planes[5].Dist=-TraceBB.Min.z;

    TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, Result);
}


void ClipModelT::TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    if (!CollisionModel) return;

    if (TraceSolid.Vertices.Size()==0) return;

    // Use the optimized point trace whenever possible.
    if (TraceSolid.Vertices.Size()==1)
    {
        // TraceSolid.Vertices[0] is normally supposed to be (0, 0, 0), but let's support the generic case.
        TraceRay(Start+TraceSolid.Vertices[0], Ray, ClipMask, Result);
        return;
    }


    // This is a quick and cheap test if Orientation is the identity matrix.
    const bool IsIdentityOrientation=(Orientation[0][0]==1.0 && Orientation[1][1]==1.0 && Orientation[2][2]==1.0);

    if (IsIdentityOrientation)
    {
        // Handle the simple case separately. This makes the rest of the code much simpler.
        CollisionModel->TraceConvexSolid(TraceSolid, Start-Origin, Ray, ClipMask, Result);
        return;
    }


    // Transform Start and Ray from world to model space.
    // Doing so requires multiplying them with the inverse of the model-to-world matrix formed by Origin and Orientation.
    // We exploit the fact that Orientation is always an orthogonal base rotation matrix, where the inverse is the transpose.
    const double       OldFrac=Result.Fraction;
    const Vector3dT    Start_ =Orientation.MulTranspose(Start-Origin);  // Start is a point in space.
    const Vector3dT    Ray_   =Orientation.MulTranspose(Ray);           // Ray is a directional vector.
    static TraceSolidT TraceSolid_;                                     // The trace solid must be appropriately rotated as well.

    TraceSolid_.Vertices.Overwrite();
    TraceSolid_.Vertices.PushBackEmpty(TraceSolid.Vertices.Size());

    for (unsigned long VertexNr=0; VertexNr<TraceSolid.Vertices.Size(); VertexNr++)
        TraceSolid_.Vertices[VertexNr]=Orientation.MulTranspose(TraceSolid.Vertices[VertexNr]);

    TraceSolid_.Planes.Overwrite();
    TraceSolid_.Planes.PushBackEmpty(TraceSolid.Planes.Size());

    for (unsigned long PlaneNr=0; PlaneNr<TraceSolid.Planes.Size(); PlaneNr++)
    {
        TraceSolid_.Planes[PlaneNr].Normal=Orientation.MulTranspose(TraceSolid.Planes[PlaneNr].Normal);
        TraceSolid_.Planes[PlaneNr].Dist  =TraceSolid.Planes[PlaneNr].Dist;
    }

    CollisionModel->TraceConvexSolid(TraceSolid_, Start_, Ray_, ClipMask, Result);

    // If there was a hit and Orientation is not the identity matrix,
    // we have to rotate the new impact normal vector from model to world space.
    if (Result.Fraction<OldFrac)
    {
        Result.ImpactNormal=Orientation.Mul(Result.ImpactNormal);
    }
}


void ClipModelT::TraceRay(const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    if (!CollisionModel) return;

    // This is a quick and cheap test if Orientation is the identity matrix.
    const bool IsIdentityOrientation=(Orientation[0][0]==1.0 && Orientation[1][1]==1.0 && Orientation[2][2]==1.0);

    if (IsIdentityOrientation)
    {
        // Handle the simple case separately. This makes the rest of the code much simpler.
        CollisionModel->TraceRay(Start-Origin, Ray, ClipMask, Result);
        return;
    }


    // Transform Start and Ray from world to model space.
    // Doing so requires multiplying them with the inverse of the model-to-world matrix formed by Origin and Orientation.
    // We exploit the fact that Orientation is always an orthogonal base rotation matrix, where the inverse is the transpose.
    const double    OldFrac=Result.Fraction;
    const Vector3dT Start_ =Orientation.MulTranspose(Start-Origin);     // Start is a point in space.
    const Vector3dT Ray_   =Orientation.MulTranspose(Ray);              // Ray is a directional vector.

    CollisionModel->TraceRay(Start_, Ray_, ClipMask, Result);

    // If there was a hit and Orientation is not the identity matrix,
    // we have to rotate the new impact normal vector from model to world space.
    if (Result.Fraction<OldFrac)
    {
        Result.ImpactNormal=Orientation.Mul(Result.ImpactNormal);
    }
}


unsigned long ClipModelT::GetContents(const Vector3dT& Point, double BoxRadius, unsigned long ContMask) const
{
    if (!CollisionModel) return 0;

    // Transform Point from world to model space.
    // Doing so requires multiplying it with the inverse of the model-to-world matrix formed by Origin and Orientation.
    // We exploit the fact that Orientation is always an orthogonal base rotation matrix, where the inverse is the transpose.
    Vector3dT ModelPoint=Point-Origin;

    // This is a quick and cheap test if Orientation is (not) the identity matrix.
    if (Orientation[0][0]!=1.0 || Orientation[1][1]!=1.0 || Orientation[2][2]!=1.0)
        ModelPoint=Orientation.MulTranspose(ModelPoint);

    return CollisionModel->GetContents(ModelPoint, BoxRadius, ContMask);
}


/* void ClipModelT::Dump() const
{
    Console->Print(cf::va("\nDump for Clip Model at 0x%p\n", this));
    Console->Print("=========================\n");

    for (ClipLinkT* Link=ListOfSectors; Link!=NULL; Link=Link->NextSectorOfModel)
    {
        assert(Link->ClipModel==this);

        const unsigned long SectorNr=Link->ClipSector - ClipWorld.Sectors;

        Console->Print(cf::va("Link: 0x%p\n", Link));
        Console->Print(cf::va("Link->ClipModel: this\n"));
        Console->Print(cf::va("Link->ClipSector: 0x%p  --> SectorNr %lu, (%lu, %lu)\n", Link->ClipSector, SectorNr, SectorNr % ClipWorld.SectorSubdivs, SectorNr / ClipWorld.SectorSubdivs));
        Console->Print(cf::va("  Link->PrevModelInSector: 0x%p\n", Link->PrevModelInSector));
        Console->Print(cf::va("  Link->NextModelInSector: 0x%p\n", Link->NextModelInSector));
        Console->Print(cf::va("Link->NextSectorOfModel: 0x%p\n", Link->NextSectorOfModel));
        Console->Print(cf::va("\n"));
    }

    Console->Print(cf::va("END of Sector List.\n\n"));
} */


void ClipModelT::Register()
{
    // Make sure that we are unregistered be re-registering.
    Unregister();

    // ClipModels without CollisionModel don't really need to register.
    if (!CollisionModel) return;

    // Update the world-space bounding box.
    Vector3dT BBCorners[8];
    CollisionModel->GetBoundingBox().GetCornerVertices(BBCorners);

    AbsoluteBB=BoundingBox3dT(Orientation*BBCorners[0]+Origin);
    for (unsigned long CornerNr=1; CornerNr<8; CornerNr++)
        AbsoluteBB.Insert(Orientation*BBCorners[CornerNr]+Origin);

    // Console->Print(cf::va("RelBB ")+convertToString(CollisionModel->GetBoundingBox().Min)+" "+convertToString(CollisionModel->GetBoundingBox().Max)+".\n");
    // Console->Print(cf::va("AbsBB ")+convertToString(AbsoluteBB.Min)+" "+convertToString(AbsoluteBB.Max)+".\n");

    // Determine which sectors this model covers.
    unsigned long GridRect[4];
    ClipWorld.GetGridRectFromBB(GridRect, AbsoluteBB);

    // Console->Print(cf::va(" --> Grid Coords: (%lu, %lu) (%lu, %lu)\n", GridRect[0], GridRect[1], GridRect[2], GridRect[3]));

    // Create a link for each sector covered by this model.
    for (unsigned long x=GridRect[0]; x<GridRect[2]; x++)
        for (unsigned long y=GridRect[1]; y<GridRect[3]; y++)
        {
            ClipSectorT& Sector=ClipWorld.Sectors[y*ClipWorld.SectorSubdivs + x];   // The sector at (x, y).
            ClipLinkT*   Link  =ClipLinkPool.Alloc();                               // Note: Link is *not* initialized (zeroed)!

            // Console->Print(cf::va("%lu (%lu, %lu) 0x%p: \n", y*ClipWorld.SectorSubdivs + x, x, y, &Sector));
            Sector.ModelContents|=GetContents();

            Link->ClipModel =this;
            Link->ClipSector=&Sector;

            Link->NextModelInSector=Sector.ListOfModels;
            Link->PrevModelInSector=NULL;
            if (Sector.ListOfModels!=NULL)
            {
                assert(Sector.ListOfModels->PrevModelInSector==NULL);
                Sector.ListOfModels->PrevModelInSector=Link;
            }
            Sector.ListOfModels=Link;

            Link->NextSectorOfModel=ListOfSectors;
            ListOfSectors          =Link;
        }
}


void ClipModelT::Unregister()
{
    // Iterate over all sectors of this model.
    // In each sector, remove us from the list of models in that sector.
    for (ClipLinkT* Link=ListOfSectors; Link!=NULL; Link=ListOfSectors)
    {
        ListOfSectors=Link->NextSectorOfModel;

        assert(Link->ClipModel==this);

        if (Link->PrevModelInSector!=NULL)
        {
            assert(Link->PrevModelInSector->NextModelInSector==Link);

            Link->PrevModelInSector->NextModelInSector=Link->NextModelInSector;
        }
        else
        {
            assert(Link->ClipSector!=NULL);
            assert(Link->ClipSector->ListOfModels==Link);

            Link->ClipSector->ListOfModels=Link->NextModelInSector;
        }

        if (Link->NextModelInSector!=NULL)
        {
            assert(Link->NextModelInSector->PrevModelInSector==Link);

            Link->NextModelInSector->PrevModelInSector=Link->PrevModelInSector;
        }

        ClipLinkPool.Free(Link);
    }
}


void ClipModelT::SetCollisionModel(const CollisionModelT* CollisionModel_)
{
    Unregister();
    CollisionModel=CollisionModel_;
}


void ClipModelT::SetOrigin(const Vector3dT& NewOrigin)
{
    // If registered to the clip world, unregister first.
    if (ListOfSectors!=NULL) Unregister();

    Origin=NewOrigin;
}


void ClipModelT::SetOrientation(const cf::math::Matrix3x3T<double>& NewOrientation)
{
    // If registered to the clip world, unregister first.
    if (ListOfSectors!=NULL) Unregister();

    Orientation=NewOrientation;
}


void ClipModelT::SetUserData(void* UD)
{
    UserData=UD;
}
