/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************************/
/*** CaSHL World (Code) ***/
/**************************/

#include "CaSHLWorld.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "MaterialSystem/Material.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"

#include <stdio.h>


CaSHLWorldT::CaSHLWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes)
    : m_World(FileName, ModelMan, GuiRes),
      m_BspTree(m_World.m_StaticEntityData[0]->m_BspTree),
      m_CollModel(m_World.m_StaticEntityData[0]->m_CollModel)
{
}


double CaSHLWorldT::TraceRay(const Vector3dT& Start, const Vector3dT& Ray) const
{
#if 1
    const static cf::ClipSys::TracePointT Point;
    cf::ClipSys::TraceResultT Result(1.0);

    m_CollModel->TraceConvexSolid(Point, Start, Ray, MaterialT::Clip_Radiance, Result);

    return Result.Fraction;
#else
    return m_BspTree->TraceRay(Start, Ray, 0.0, 1.0, cf::SceneGraph::ConsiderAll, MaterialT::Clip_Radiance);
#endif
}


inline double GetSqrDist(const ArrayT<double>& A, const ArrayT<double>& B)
{
    double Dist=0.0;

    // if (A.Size()!=16) printf("A.Size()!=16 !!!!!!!!!!!!!!!!!!!!!!!!!\n");
    // if (B.Size()!=16) printf("B.Size()!=16 !!!!!!!!!!!!!!!!!!!!!!!!!\n");

    for (unsigned long i=0; i<A.Size(); i++)
    {
        const double d=B[i]-A[i];

        Dist+=d*d;
    }

    // No need for sqrt().
    return Dist;
}


void CaSHLWorldT::PatchesToSHLMaps(const ArrayT< ArrayT<PatchT> >& Patches)
{
    const cf::SceneGraph::BspTreeNodeT& Map = *m_BspTree;

    // Clear everything - the code below will fill in the new data.
    for (unsigned long SHLMapNr = 0; SHLMapNr < m_World.SHLMapMan.SHLMaps.Size(); SHLMapNr++)
    {
        m_World.SHLMapMan.SHLMaps[SHLMapNr]->Coeffs .Clear();
        m_World.SHLMapMan.SHLMaps[SHLMapNr]->Indices.Clear();
    }

    m_World.SHLMapMan.SHLCoeffsTable.Clear();


    // Proceed depending on whether the data is to be stored compressed or not.
    if (cf::SceneGraph::SHLMapManT::NrOfRepres>0)
    {
        // Compress the SHL coeffs by some representatives.

        // Start by gathering all SHL vectors in one big list.
        ArrayT< const ArrayT<double>* > AllVectors;

        {
            for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
            {
                const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=Map.FaceChildren[FaceNr]->SHLMapInfo;

                for (unsigned long t=0; t<SMI.SizeT; t++)
                    for (unsigned long s=0; s<SMI.SizeS; s++)
                        AllVectors.PushBack(&(Patches[FaceNr][t*SMI.SizeS+s].SHCoeffs_TotalTransfer));
            }
        }

        if (cf::SceneGraph::SHLMapManT::NrOfRepres>AllVectors.Size()) cf::SceneGraph::SHLMapManT::NrOfRepres=AllVectors.Size();


        // Now pick an initial set of representatives from the complete list, approximately in an even distribution.
        ArrayT< ArrayT<double> > Representatives;

        {
            for (unsigned long RepNr=0; RepNr<cf::SceneGraph::SHLMapManT::NrOfRepres; RepNr++)
                Representatives.PushBack(*(AllVectors[RepNr*(AllVectors.Size()/cf::SceneGraph::SHLMapManT::NrOfRepres)]));
        }


        // Also maintain a list that states the best (nearest) representative for each vector.
        ArrayT<unsigned long> BestRepForVector;

        BestRepForVector.PushBackEmpty(AllVectors.Size());


        // Iterate until the optimal solution is found.
        double PrevLargestDist=1000.0;
        char   TriesLeft      =3;

        for (unsigned long IterationCounter=0; true; IterationCounter++)
        {
            // Create an array of clusters (one cluster for each representative),
            // where each cluster is an array of (pointers to) vectors (those in AllVectors whose closest representative is this one).
            ArrayT< ArrayT< const ArrayT<double>* > > RepClusters;

            RepClusters.PushBackEmpty(Representatives.Size());


            // For each vector in AllVectors, figure out the representative that it is closest to,
            // and then assign this vector to (the cluster of) this representative.
            double LargestDist=0.0;

            for (unsigned long VectorNr=0; VectorNr<AllVectors.Size(); VectorNr++)
            {
                unsigned long ClosestRepNr  =0;
                double        ClosestRepDist=GetSqrDist(*(AllVectors[VectorNr]), Representatives[0]);

                for (unsigned long CurrentRepNr=1; CurrentRepNr<Representatives.Size(); CurrentRepNr++)
                {
                    const double CurrentRepDist=GetSqrDist(*(AllVectors[VectorNr]), Representatives[CurrentRepNr]);

                    if (CurrentRepDist<ClosestRepDist)
                    {
                        ClosestRepNr  =CurrentRepNr;
                        ClosestRepDist=CurrentRepDist;
                    }
                }

                RepClusters[ClosestRepNr].PushBack(AllVectors[VectorNr]);
                BestRepForVector[VectorNr]=ClosestRepNr;    // Also do the opposite assignment: Save for the current vector its closest representative.

                if (LargestDist<ClosestRepDist) LargestDist=ClosestRepDist;
            }

            printf("%lu  %.15f\r", IterationCounter, LargestDist);
            fflush(stdout);

            if (LargestDist>=PrevLargestDist)
            {
                TriesLeft--;

                if (TriesLeft==0)
                {
                    unsigned long UnusedCount=0;

                    for (unsigned long RepNr=0; RepNr<Representatives.Size(); RepNr++)
                        if (RepClusters[RepNr].Size()==0) UnusedCount++;

                    printf("Almost done. %lu of %lu representatives unused.\n", UnusedCount, Representatives.Size());
                    break;
                }
            }
            else
            {
                PrevLargestDist=LargestDist;
                TriesLeft=3;
            }


            // Finally re-determine each representative to become the average of its cluster contents.
            for (unsigned long RepNr=0; RepNr<Representatives.Size(); RepNr++)
            {
                if (RepClusters[RepNr].Size()==0) continue;

                // Clear the representative vector.
                for (unsigned long CoeffNr=0; CoeffNr<Representatives[RepNr].Size(); CoeffNr++)
                    Representatives[RepNr][CoeffNr]=0.0;

                // Sum up the vectors in the cluster.
                for (unsigned long ClusterVecNr=0; ClusterVecNr<RepClusters[RepNr].Size(); ClusterVecNr++)
                    for (unsigned long CoeffNr=0; CoeffNr<Representatives[RepNr].Size(); CoeffNr++)
                        Representatives[RepNr][CoeffNr]+=(*(RepClusters[RepNr][ClusterVecNr]))[CoeffNr];

                // Compute the average.
                for (unsigned long CoeffNr=0; CoeffNr<Representatives[RepNr].Size(); CoeffNr++)
                    Representatives[RepNr][CoeffNr]/=double(RepClusters[RepNr].Size());
            }
        }


        // Allocate space for the indices.
        for (unsigned long SHLMapNr = 0; SHLMapNr < m_World.SHLMapMan.SHLMaps.Size(); SHLMapNr++)
        {
            // Make sure that all indices are initialized to zero.
            while (m_World.SHLMapMan.SHLMaps[SHLMapNr]->Indices.Size() < (unsigned long)cf::SceneGraph::SHLMapManT::SIZE_S * cf::SceneGraph::SHLMapManT::SIZE_T)
                m_World.SHLMapMan.SHLMaps[SHLMapNr]->Indices.PushBack(0);
        }


        // Fill in the indices to the best representatives for each vector.
        unsigned long VectorNr=0;

        for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        {
            const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=Map.FaceChildren[FaceNr]->SHLMapInfo;

            for (unsigned long t=0; t<SMI.SizeT; t++)
                for (unsigned long s=0; s<SMI.SizeS; s++)
                    m_World.SHLMapMan.SHLMaps[SMI.SHLMapNr]->Indices[(SMI.PosT+t) * cf::SceneGraph::SHLMapManT::SIZE_S+SMI.PosS+s] = (unsigned short)BestRepForVector[VectorNr++];
        }


        // Finally, write the representatives into the SHLCoeffsTable.
        for (unsigned long RepNr=0; RepNr<Representatives.Size(); RepNr++)
            for (unsigned long CoeffNr=0; CoeffNr<Representatives[RepNr].Size(); CoeffNr++)
                m_World.SHLMapMan.SHLCoeffsTable.PushBack(float(Representatives[RepNr][CoeffNr]));
    }
    else
    {
        // Store the SHL coeffs uncompressed.
        const unsigned long NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;

        for (unsigned long SHLMapNr = 0; SHLMapNr < m_World.SHLMapMan.SHLMaps.Size(); SHLMapNr++)
        {
            const unsigned long SHLMapCoeffSize = cf::SceneGraph::SHLMapManT::SIZE_S * cf::SceneGraph::SHLMapManT::SIZE_T * NR_OF_SH_COEFFS;

            // Make sure that all coefficients are initialized with zeros.
            while (m_World.SHLMapMan.SHLMaps[SHLMapNr]->Coeffs.Size() < SHLMapCoeffSize)
                m_World.SHLMapMan.SHLMaps[SHLMapNr]->Coeffs.PushBack(0.0);
        }

        // Übertrage die Patches-Werte zurück in die SHLMaps.
        for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        {
            const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=Map.FaceChildren[FaceNr]->SHLMapInfo;

            for (unsigned long t=0; t<SMI.SizeT; t++)
                for (unsigned long s=0; s<SMI.SizeS; s++)
                    for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                        m_World.SHLMapMan.SHLMaps[SMI.SHLMapNr]->Coeffs[((SMI.PosT+t)*cf::SceneGraph::SHLMapManT::SIZE_S + SMI.PosS + s) * NR_OF_SH_COEFFS + CoeffNr]
                            = float(Patches[FaceNr][t*SMI.SizeS + s].SHCoeffs_TotalTransfer[CoeffNr]);
        }
    }
}


void CaSHLWorldT::SaveToDisk(const char* FileName) const
{
    m_World.SaveToDisk(FileName);
}
