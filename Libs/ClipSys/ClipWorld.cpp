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

#include "ClipWorld.hpp"
#include "ClipWorld_private.hpp"
#include "ClipModel.hpp"
#include "CollisionModel_base.hpp"
#include "TraceResult.hpp"
#include "TraceSolid.hpp"
#include "ConsoleCommands/Console.hpp"


using namespace cf::ClipSys;
using namespace cf::math;


ClipWorldT::ClipWorldT(const CollisionModelT* WorldCollMdl_)
    : WorldCollMdl(WorldCollMdl_),
      WorldBB(WorldCollMdl->GetBoundingBox()),
      SectorSubdivs(64),
      SectorSideLen((WorldBB.Max-WorldBB.Min)/double(SectorSubdivs)),
      Sectors(new ClipSectorT[SectorSubdivs*SectorSubdivs])
{
}


ClipWorldT::~ClipWorldT()
{
#ifdef DEBUG
    // Make sure that all sectors are empty (that is, all clip models have been destructed already).
    for (unsigned long SectorNr=0; SectorNr<SectorSubdivs*SectorSubdivs; SectorNr++)
        assert(Sectors[SectorNr].ListOfModels==NULL);
#endif

    delete[] Sectors;
}


void ClipWorldT::TraceBoundingBox(const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray,
    unsigned long ClipMask, const ClipModelT* Ignore, TraceResultT& Result, ClipModelT** HitClipModel) const
{
    // Use the optimized point trace whenever possible.
    if (TraceBB.Min==TraceBB.Max)
    {
        // TraceBB.Min and TraceBB.Max are normally supposed to be (0, 0, 0), but let's support the generic case.
        TraceRay(Start+TraceBB.Min, Ray, ClipMask, Ignore, Result, HitClipModel);
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

    TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, Ignore, Result, HitClipModel);
}


void ClipWorldT::TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray,
    unsigned long ClipMask, const ClipModelT* Ignore, TraceResultT& Result, ClipModelT** HitClipModel) const
{
    // Violation of this requirement is usually, but not necessarily, an error.
    // If you ever get false-positives here, just remove the test entirely.
    assert(Result.Fraction==1.0 && !Result.StartSolid);

    if (HitClipModel) *HitClipModel=NULL;

    if (TraceSolid.Vertices.Size()==0) return;

    // Use the optimized point trace whenever possible.
    if (TraceSolid.Vertices.Size()==1)
    {
        // TraceSolid.Vertices[0] is normally supposed to be (0, 0, 0), but let's support the generic case.
        TraceRay(Start+TraceSolid.Vertices[0], Ray, ClipMask, Ignore, Result, HitClipModel);
        return;
    }


    // Try the trace against the WorldCollMdl first.
    WorldCollMdl->TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, Result);

 // if (Result.Fraction<OldFrac && HitClipModel) *HitClipModel=WorldCollMdl;     // FIXME: WorldCollMdl is of type CollisionModelT...
    if (Result.Fraction==0.0) return;


    // Now try all the entity models.
    ArrayT<ClipModelT*>  ClipModels;
    const BoundingBox3dT HullBB(TraceSolid.Vertices);
    const BoundingBox3dT OverallHullBB=HullBB.GetOverallTranslationBox(Start, Start+Ray*Result.Fraction);

    GetClipModelsFromBB(ClipModels, ClipMask, OverallHullBB);

    for (unsigned long ModelNr=0; ModelNr<ClipModels.Size(); ModelNr++)
    {
        ClipModelT*  ClipModel  =ClipModels[ModelNr];
        const double OldFraction=Result.Fraction;

        if (ClipModel==Ignore) continue;

        ClipModel->TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, Result);

        if (Result.Fraction<OldFraction && HitClipModel) *HitClipModel=ClipModel;
        if (Result.Fraction==0.0) break;
    }
}


void ClipWorldT::TraceRay(const Vector3dT& Start, const Vector3dT& Ray,
    unsigned long ClipMask, const ClipModelT* Ignore, TraceResultT& Result, ClipModelT** HitClipModel) const
{
    // Violation of this requirement is usually, but not necessarily, an error.
    // If you ever get false-positives here, just remove the test entirely.
    assert(Result.Fraction==1.0 && !Result.StartSolid);

    if (HitClipModel) *HitClipModel=NULL;

    // Try the trace against the WorldCollMdl first.
    WorldCollMdl->TraceRay(Start, Ray, ClipMask, Result);

 // if (Result.Fraction<OldFrac && HitClipModel) *HitClipModel=WorldCollMdl;     // FIXME: WorldCollMdl is of type CollisionModelT...
    if (Result.Fraction==0.0) return;


    // Now try all the entity models.
    ArrayT<ClipModelT*>  ClipModels;
    const BoundingBox3dT OverallBB(Start, Start+Ray*Result.Fraction);

    GetClipModelsFromBB(ClipModels, ClipMask, OverallBB);

    for (unsigned long ModelNr=0; ModelNr<ClipModels.Size(); ModelNr++)
    {
        ClipModelT*  ClipModel  =ClipModels[ModelNr];
        const double OldFraction=Result.Fraction;

        if (ClipModel==Ignore) continue;

        ClipModel->TraceRay(Start, Ray, ClipMask, Result);

        if (Result.Fraction<OldFraction && HitClipModel) *HitClipModel=ClipModel;
        if (Result.Fraction==0.0) break;
    }
}


void ClipWorldT::GetGridRectFromBB(unsigned long GridRect[], const BoundingBox3dT& BB) const
{
    assert(((unsigned long)(3.9))==3);      // Assert that doubles are cast to unsigned longs as expected.

    GridRect[0]=(unsigned long)((BB.Min.x-WorldBB.Min.x)/SectorSideLen.x);
    GridRect[1]=(unsigned long)((BB.Min.y-WorldBB.Min.y)/SectorSideLen.y);
    GridRect[2]=(unsigned long)((BB.Max.x-WorldBB.Min.x)/SectorSideLen.x);
    GridRect[3]=(unsigned long)((BB.Max.y-WorldBB.Min.y)/SectorSideLen.y);

    for (unsigned long CheckNr=0; CheckNr<4; CheckNr++)
    {
        // Hmmm. I think it *is* possible to have situations where this assertion is violated...
        // so I rather reduce it to a developer warning instead of having assert abort the program.
        // assert(GridRect[i]<SectorSubdivs);
        if (GridRect[CheckNr]>=SectorSubdivs) Console->DevWarning(cf::va("%s(%lu): CheckNr==%lu, %lu>=%lu\n", __FILE__, __LINE__, CheckNr, GridRect[CheckNr], SectorSubdivs));

        if (int(GridRect[CheckNr])<0        ) GridRect[CheckNr]=0;
        if (GridRect[CheckNr]>=SectorSubdivs) GridRect[CheckNr]=SectorSubdivs-1;
    }

    GridRect[2]++;
    GridRect[3]++;
}


// Note that the world clip model is *not* in the list of returned clip models!
void ClipWorldT::GetClipModelsFromBB(ArrayT<ClipModelT*>& ClipModels, unsigned long ContentMask, const BoundingBox3dT& BB) const
{
    BoundingBox3dT ExpandedBB=BB.GetEpsilonBox(1.0);
    unsigned long  GridRect[4];

    GetGridRectFromBB(GridRect, BB);

#ifdef DEBUG
    for (unsigned long ModelNr=0; ModelNr<ClipModels.Size(); ModelNr++)
        assert(!ClipModels[ModelNr]->AlreadyChecked);
#endif

    for (unsigned long x=GridRect[0]; x<GridRect[2]; x++)
        for (unsigned long y=GridRect[1]; y<GridRect[3]; y++)
        {
            ClipSectorT& Sector=Sectors[y*SectorSubdivs + x];   // The sector at (x, y).

            if ((Sector.ModelContents & ContentMask)==0) continue;

            for (ClipLinkT* Link=Sector.ListOfModels; Link!=NULL; Link=Link->NextModelInSector)
            {
                ClipModelT* ClipModel=Link->ClipModel;

                if (ClipModel->AlreadyChecked) continue;
                if (!ClipModel->IsEnabled) continue;
                if ((ClipModel->GetContents() & ContentMask)==0) continue;
                if (!ClipModel->GetAbsoluteBB().Intersects(ExpandedBB)) continue;

                ClipModel->AlreadyChecked=true;
                ClipModels.PushBack(ClipModel);
            }
        }

    for (unsigned long ModelNr=0; ModelNr<ClipModels.Size(); ModelNr++)
        ClipModels[ModelNr]->AlreadyChecked=false;
}


void ClipWorldT::GetContacts(const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray,
    unsigned long ClipMask, const ClipModelT* Ignore, ContactsResultT& Contacts) const
{
    // Use the optimized point trace whenever possible.
    if (TraceBB.Min==TraceBB.Max)
    {
        // TraceBB.Min and TraceBB.Max are normally supposed to be (0, 0, 0), but let's support the generic case.
        GetContacts(Start+TraceBB.Min, Ray, ClipMask, Ignore, Contacts);
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

    GetContacts(TraceSolid, Start, Ray, ClipMask, Ignore, Contacts);
}


void ClipWorldT::GetContacts(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray,
    unsigned long ClipMask, const ClipModelT* Ignore, ContactsResultT& Contacts) const
{
    Contacts.NrOfRepContacts=0;
    Contacts.NrOfAllContacts=0;

    if (TraceSolid.Vertices.Size()==0) return;

    // Use the optimized point trace whenever possible.
    if (TraceSolid.Vertices.Size()==1)
    {
        // TraceSolid.Vertices[0] is normally supposed to be (0, 0, 0), but let's support the generic case.
        GetContacts(Start+TraceSolid.Vertices[0], Ray, ClipMask, Ignore, Contacts);
        return;
    }


    // Try all the entity models - the WorldCollMdl is never included among the contacts.
    ArrayT<ClipModelT*>  ClipModels;
    const BoundingBox3dT HullBB(TraceSolid.Vertices);
    const BoundingBox3dT OverallHullBB=HullBB.GetOverallTranslationBox(Start, Start+Ray);

    GetClipModelsFromBB(ClipModels, ClipMask, OverallHullBB);

    for (unsigned long ModelNr=0; ModelNr<ClipModels.Size(); ModelNr++)
    {
        ClipModelT* ClipModel=ClipModels[ModelNr];

        if (ClipModel==Ignore) continue;

        TraceResultT Result(1.0);
        ClipModel->TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, Result);

        if (Result.Fraction<1.0)
        {
            Contacts.NrOfAllContacts++;

            if (Contacts.NrOfRepContacts==ContactsResultT::MAX_CONTACTS)
            {
                // No more room for storing another contact.
                if (Result.Fraction>=Contacts.TraceResults[ContactsResultT::MAX_CONTACTS-1].Fraction) continue; // Outside of Contacts array.
                Contacts.NrOfRepContacts--;    // Will throw out the topmost element.
            }


            unsigned long ConNr=Contacts.NrOfRepContacts;

            // ClipModel was hit by the trace - that is, there was a contact!
            // Now insert the result at the right place into Contacts.
            while (ConNr>0 && Contacts.TraceResults[ConNr-1].Fraction>Result.Fraction)
            {
                Contacts.TraceResults[ConNr]=Contacts.TraceResults[ConNr-1];
                Contacts.ClipModels  [ConNr]=Contacts.ClipModels  [ConNr-1];

                ConNr--;
            }

            Contacts.TraceResults[ConNr]=Result;
            Contacts.ClipModels  [ConNr]=ClipModel;
            Contacts.NrOfRepContacts++;
        }
    }
}


void ClipWorldT::GetContacts(const Vector3dT& Start, const Vector3dT& Ray,
    unsigned long ClipMask, const ClipModelT* Ignore, ContactsResultT& Contacts) const
{
    Contacts.NrOfRepContacts=0;
    Contacts.NrOfAllContacts=0;

    // Try all the entity models - the WorldCollMdl is never included among the contacts.
    ArrayT<ClipModelT*>  ClipModels;
    const BoundingBox3dT OverallBB(Start, Start+Ray);

    GetClipModelsFromBB(ClipModels, ClipMask, OverallBB);

    for (unsigned long ModelNr=0; ModelNr<ClipModels.Size(); ModelNr++)
    {
        ClipModelT* ClipModel=ClipModels[ModelNr];

        if (ClipModel==Ignore) continue;

        TraceResultT Result(1.0);
        ClipModel->TraceRay(Start, Ray, ClipMask, Result);

        if (Result.Fraction<1.0)
        {
            Contacts.NrOfAllContacts++;

            if (Contacts.NrOfRepContacts==ContactsResultT::MAX_CONTACTS)
            {
                // No more room for storing another contact.
                if (Result.Fraction>=Contacts.TraceResults[ContactsResultT::MAX_CONTACTS-1].Fraction) continue; // Outside of Contacts array.
                Contacts.NrOfRepContacts--;    // Will throw out the topmost element.
            }


            unsigned long ConNr=Contacts.NrOfRepContacts;

            // ClipModel was hit by the trace - that is, there was a contact!
            // Now insert the result at the right place into Contacts.
            while (ConNr>0 && Contacts.TraceResults[ConNr-1].Fraction>Result.Fraction)
            {
                Contacts.TraceResults[ConNr]=Contacts.TraceResults[ConNr-1];
                Contacts.ClipModels  [ConNr]=Contacts.ClipModels  [ConNr-1];

                ConNr--;
            }

            Contacts.TraceResults[ConNr]=Result;
            Contacts.ClipModels  [ConNr]=ClipModel;
            Contacts.NrOfRepContacts++;
        }
    }
}
