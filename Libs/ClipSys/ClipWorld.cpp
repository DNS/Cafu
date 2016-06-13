/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
      SectorSideLen((WorldBB.Max - WorldBB.Min) / double(SectorSubdivs)),
      Sectors(new ClipSectorT[SectorSubdivs * SectorSubdivs])
{
}


ClipWorldT::~ClipWorldT()
{
#ifdef DEBUG
    // Make sure that all sectors are empty (that is, all clip models have been destructed already).
    for (unsigned long SectorNr = 0; SectorNr < SectorSubdivs * SectorSubdivs; SectorNr++)
    {
        // Don't inline this into the assert() statement. If the assertion triggers,
        // it is convenient to have CS readily available in the debugger.
        const ClipSectorT& CS = Sectors[SectorNr];

        assert(CS.ListOfModels == NULL);
    }
#endif

    delete[] Sectors;
}


void ClipWorldT::TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray,
                                  unsigned long ClipMask, const ClipModelT* Ignore, TraceResultT& Result, ClipModelT** HitClipModel) const
{
    if (HitClipModel) *HitClipModel = NULL;

    static ArrayT<WorldTraceResultT> Results;
    // Results.Overwrite();     // Done in Trace().

    Trace(TraceSolid, Start, Ray, ClipMask, Ignore, Results);

    if (Results.Size() > 0)
    {
        Result = Results[0].Result;

        if (HitClipModel)
            *HitClipModel = Results[0].ClipModel;
    }
}


void ClipWorldT::GetGridRectFromBB(unsigned long GridRect[], const BoundingBox3dT& BB) const
{
    assert(((unsigned long)(3.9)) == 3);    // Assert that doubles are cast to unsigned longs as expected.

    GridRect[0] = (unsigned long)((BB.Min.x - WorldBB.Min.x) / SectorSideLen.x);
    GridRect[1] = (unsigned long)((BB.Min.y - WorldBB.Min.y) / SectorSideLen.y);
    GridRect[2] = (unsigned long)((BB.Max.x - WorldBB.Min.x) / SectorSideLen.x);
    GridRect[3] = (unsigned long)((BB.Max.y - WorldBB.Min.y) / SectorSideLen.y);

    for (unsigned long CheckNr = 0; CheckNr < 4; CheckNr++)
    {
        // Hmmm. I think it *is* possible to have situations where this assertion is violated...
        // so I rather reduce it to a developer warning instead of having assert abort the program.
        // assert(GridRect[i]<SectorSubdivs);
        if (GridRect[CheckNr] >= SectorSubdivs) Console->DevWarning(cf::va("%s(%lu): CheckNr==%lu, %lu>=%lu\n", __FILE__, __LINE__, CheckNr, GridRect[CheckNr], SectorSubdivs));

        if (int(GridRect[CheckNr]) < 0        ) GridRect[CheckNr] = 0;
        if (GridRect[CheckNr] >= SectorSubdivs) GridRect[CheckNr] = SectorSubdivs - 1;
    }

    GridRect[2]++;
    GridRect[3]++;
}


// Note that the world clip model is *not* in the list of returned clip models!
void ClipWorldT::GetClipModelsFromBB(ArrayT<ClipModelT*>& ClipModels, unsigned long ContentMask, const BoundingBox3dT& BB) const
{
    const BoundingBox3dT ExpandedBB = BB.GetEpsilonBox(1.0);
    unsigned long GridRect[4];

    GetGridRectFromBB(GridRect, BB);

#ifdef DEBUG
    // TODO: Can we assert that for all clip models CM in the world,
    // assert(!CM->AlreadyChecked)  holds?
#endif

    for (unsigned long x = GridRect[0]; x < GridRect[2]; x++)
        for (unsigned long y = GridRect[1]; y < GridRect[3]; y++)
        {
            const ClipSectorT& Sector = Sectors[y*SectorSubdivs + x];   // The sector at (x, y).

            if ((Sector.ModelContents & ContentMask) == 0) continue;

            for (ClipLinkT* Link = Sector.ListOfModels; Link != NULL; Link = Link->NextModelInSector)
            {
                ClipModelT* ClipModel = Link->ClipModel;

                if (ClipModel->AlreadyChecked) continue;
                if (!ClipModel->IsEnabled) continue;
                if ((ClipModel->GetContents() & ContentMask) == 0) continue;
                if (!ClipModel->GetAbsoluteBB().Intersects(ExpandedBB)) continue;

                ClipModel->AlreadyChecked = true;
                ClipModels.PushBack(ClipModel);
            }
        }

    for (unsigned long ModelNr = 0; ModelNr < ClipModels.Size(); ModelNr++)
        ClipModels[ModelNr]->AlreadyChecked = false;
}


void ClipWorldT::Trace(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray,
                       unsigned long ClipMask, const ClipModelT* Ignore, ArrayT<WorldTraceResultT>& Results) const
{
    Results.Overwrite();

    // First trace against the WorldCollMdl.
    // This is a special-case, because WorldCollMdl is a CollisionModelT,
    // not a ClipModelT, and as such not registered in the clip sectors.
    {
        TraceResultT ModelResult;

        WorldCollMdl->TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, ModelResult);

        Results.PushBack(WorldTraceResultT(ModelResult, NULL));
    }

    // Now trace against all entity clip models.
    static ArrayT<ClipModelT*> ClipModels;
    ClipModels.Overwrite();

    const BoundingBox3dT OverallBB = TraceSolid.GetBB().GetOverallTranslationBox(Start, Start + Ray);

    GetClipModelsFromBB(ClipModels, ClipMask, OverallBB);

    for (unsigned int ModelNr = 0; ModelNr < ClipModels.Size(); ModelNr++)
    {
        ClipModelT* ClipModel = ClipModels[ModelNr];

        if (ClipModel == Ignore) continue;

        TraceResultT EntityResult;

        ClipModel->TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, EntityResult);

        Results.PushBack(WorldTraceResultT(EntityResult, ClipModel));
    }

    // Remove all results in which no hit occurred (Fraction == 1.0).
    for (unsigned int Nr = 0; Nr < Results.Size(); Nr++)
    {
        if (Results[Nr].Result.Fraction == 1.0)
        {
            Results.RemoveAt(Nr);
            Nr--;
        }
    }

    // Sort the results by StartSolid first, then by Fraction value.
    struct ResultsComparatorT
    {
        bool operator () (const WorldTraceResultT& r1, const WorldTraceResultT& r2)
        {
            if (r1.Result.StartSolid == r2.Result.StartSolid)
                return r1.Result.Fraction < r2.Result.Fraction;

            return r1.Result.StartSolid;
        }
    };

    Results.QuickSort(ResultsComparatorT());

    // Assert that the results are sorted as intended.
#ifdef DEBUG
    for (unsigned int Nr = 1; Nr < Results.Size(); Nr++)
    {
        assert(!(Results[Nr-1].Result.StartSolid == false && Results[Nr].Result.StartSolid == true));
        assert(Results[Nr-1].Result.Fraction <= Results[Nr].Result.Fraction);
    }
#endif
}
