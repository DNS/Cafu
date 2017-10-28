/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>
#include <iostream>

#include "../../Common/World.hpp"   // SHOULD *NOT* BE HERE, only needed for some MapT::xy stuff!
#include "BspTreeBuilder.hpp"
#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/Material.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"

#ifndef _WIN32
#define _stricmp strcasecmp
#endif

#if defined(_WIN32)
    #if defined(_MSC_VER)
        #define vsnprintf _vsnprintf
    #endif
#endif


const time_t ProgramStartTime2=time(NULL);

// Returns a string with the elapsed time since program start.
// The string is in the format "hh:mm:ss".
static const char* GetTimeSinceProgramStart()
{
    const unsigned long TotalSec=(unsigned long)difftime(time(NULL), ProgramStartTime2);
    const unsigned long Sec     =TotalSec % 60;
    const unsigned long Min     =(TotalSec/60) % 60;
    const unsigned long Hour    =TotalSec/3600;

    static char TimeString[24];
    sprintf(TimeString, "%2lu:%2lu:%2lu", Hour, Min, Sec);

    return TimeString;
}


static void Error(const char* ErrorText, ...)
{
    va_list ArgList;
    char    ErrorString[256];

    if (ErrorText!=NULL)
    {
        va_start(ArgList, ErrorText);
            vsnprintf(ErrorString, 256, ErrorText, ArgList);
        va_end(ArgList);

        Console->Print(std::string("\nFATAL ERROR: ")+ErrorString+"\n");
    }

    Console->Print("Program aborted.\n\n");
    exit(1);
}


BspTreeBuilderT::BspTreeBuilderT(cf::SceneGraph::BspTreeNodeT* BspTree_, bool MostSimpleTree, bool BspSplitFaces, bool ChopUpFaces)
    : BspTree             (BspTree_                    ),
   // Nodes               (BspTree_->Nodes             ),
   // Leaves              (BspTree_->Leaves            ),
      PVS                 (BspTree_->PVS               ),
      FaceChildren        (BspTree_->FaceChildren      ),
      OtherChildren       (BspTree_->OtherChildren     ),
      GlobalDrawVertices  (BspTree_->GlobalDrawVertices),
      m_Option_MostSimpleTree(MostSimpleTree),
      m_Option_BspSplitFaces(BspSplitFaces),
      m_Option_ChopUpFaces(ChopUpFaces)
{
}


void BspTreeBuilderT::Build(bool IsWorldspawn, const ArrayT<Vector3dT>& FloodFillSources_,
    const ArrayT<Vector3dT>& OutsidePointSamples, const std::string& MapFileName)
{
    ArrayT<Vector3dT> FloodFillSources=FloodFillSources_;
    MaterialT LeakDetectMat;    // Just a material instance that is different from all others.

    if (IsWorldspawn)
    {
        // Worldspawn entities are supposed to come with a flood-fill origins list provided by the caller,
        // usually obtained from other entity origins (e.g. info_player_start).
        assert(FloodFillSources.Size()>0);
    }
    else
    {
        assert(FloodFillSources.Size()==0);

        if (FaceChildren.Size()>0)
        {
            // We have faces, but no flood-fill origins.

            // Determine a world bounding box.
            BoundingBox3T<double> WorldBB(FaceChildren[0]->Polygon.Vertices);

            for (unsigned long FaceNr=1; FaceNr<FaceChildren.Size(); FaceNr++)
                WorldBB.Insert(FaceChildren[FaceNr]->Polygon.Vertices);

            const double d=20.0*MapT::MinVertexDist;

            FloodFillSources.PushBack(WorldBB.Max+Vector3dT(d, d, d));
        }
    }


    if (!m_Option_MostSimpleTree && FaceChildren.Size() >= 4 /*No need to try to optimize the tree if it is too simple.*/)
    {
        if (m_Option_ChopUpFaces)
        {
            // FIRST TREE
            PrepareLeakDetection(FloodFillSources, &LeakDetectMat);
            BuildBSPTree();
            Portalize();
            FloodFillInside(FloodFillSources, OutsidePointSamples);

            if (IsWorldspawn)
            {
                // Only worldspawn entities are seen from the "inside", and thus supposed to be "watertight".
                DetectLeaks(FloodFillSources, MapFileName, &LeakDetectMat);
            }

            RemoveOuterFaces();

            for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
                if (FaceChildren[FaceNr]->Material==&LeakDetectMat)
                {
                    assert(!IsWorldspawn);
                    delete FaceChildren[FaceNr];

                    FaceChildren[FaceNr]=FaceChildren[FaceChildren.Size()-1]; FaceChildren.DeleteBack();
                    FaceNr--;
                }

            // After 1st tree code.
            ChopUpFaces();
        }

        // SECOND TREE
        PrepareLeakDetection(FloodFillSources, &LeakDetectMat);
        BuildBSPTree();
        Portalize();
        FloodFillInside(FloodFillSources, OutsidePointSamples);

        if (IsWorldspawn)
        {
            // Only worldspawn entities are seen from the "inside", and thus supposed to be "watertight".
            DetectLeaks(FloodFillSources, MapFileName, &LeakDetectMat);
        }

        RemoveOuterFaces();

        for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
            if (FaceChildren[FaceNr]->Material==&LeakDetectMat)
            {
                assert(!IsWorldspawn);
                delete FaceChildren[FaceNr];

                FaceChildren[FaceNr]=FaceChildren[FaceChildren.Size()-1]; FaceChildren.DeleteBack();
                FaceNr--;
            }

        // After 1st+2nd tree code.
        ;
    }

    MergeFaces();
    ChopUpForMaxLightMapSize();
    ChopUpForMaxSHLMapSize();

    // THIRD TREE
    BuildBSPTree();

    if (IsWorldspawn)
    {
        // Do this only for worldspawn entities (seen from the "inside") for now.
        //
        // For other entities (seen from the "outside"), the outer portals are not built correctly with the current code.
        // This is not so bad, because we don't really need a PVS for those entities anyway, but if we did, we had to make sure first
        // that outer portals work right, which in turn is difficult due to their unlimited size (could limit it to the world BB though).
        Portalize();                                                // Nur für CaPVS!
        FloodFillInside(FloodFillSources, OutsidePointSamples);     // Nicht mehr wirklich nützlich, es gibt aber noch Zusammenhänge mit "InnerLeaf"...
        /*if (IsWorldspawn)*/ RemoveOuterPortals();
    }

    SortFacesIntoTexNameOrder();
    CreateFullBrightLightMaps();
    CreateZeroBandSHLMaps();
    AssignOtherChildrenToLeaves();
    ComputeDrawStructures();
    CreateFullVisPVS();
}


#include "ChopUpFaces.cpp"
#include "BuildBSPTree.cpp"
#include "Portalize.cpp"
#include "FloodFillInside.cpp"
#include "LeakDetection.cpp"
#include "RemoveOuterStuff.cpp"
#include "MergeFaces.cpp"
#include "SortFaces.cpp"
#include "LightMaps.cpp"
#include "SHLMaps.cpp"
#include "ComputeDrawStructures.cpp"


void BspTreeBuilderT::AssignOtherChildrenToLeaves()
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    for (unsigned long ChildNr=0; ChildNr<OtherChildren.Size(); ChildNr++)
    {
        // Figure out all leaves that are touched by the BB of the current child.
        ArrayT<unsigned long> TouchedLeaves;

        BspTree->WhatLeaves(TouchedLeaves, OtherChildren[ChildNr]->GetBoundingBox().GetEpsilonBox(-MapT::RoundEpsilon));

        for (unsigned long LeafNr=0; LeafNr<TouchedLeaves.Size(); LeafNr++)
            Leaves[TouchedLeaves[LeafNr]].OtherChildrenSet.PushBack(ChildNr);
    }
}


void BspTreeBuilderT::CreateFullVisPVS()
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    for (unsigned long Nr=0; Nr<(Leaves.Size()*Leaves.Size()+31)/32; Nr++)
        PVS.PushBack(0xFFFFFFFF);
}
