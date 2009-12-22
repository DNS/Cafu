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

/****************************/
/*** CaLight World (Code) ***/
/****************************/

#include "CaLightWorld.hpp"
#include "Bitmap/Bitmap.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/TraceResult.hpp"
#include "MaterialSystem/Material.hpp"
#include "SceneGraph/BspTreeNode.hpp"


CaLightWorldT::CaLightWorldT(const char* FileName)
    : World(FileName)
{
}


double CaLightWorldT::TraceRay(const Vector3dT& Start, const Vector3dT& Ray) const
{
#if 1
    cf::ClipSys::TraceResultT Result(1.0);

    World.CollModel->TraceRay(Start, Ray, MaterialT::Clip_Radiance, Result);

    return Result.Fraction;
#else
    return World.BspTree->TraceRay(Start, Ray, 0.0, 1.0, cf::SceneGraph::ConsiderAll, MaterialT::Clip_Radiance);
#endif
}


void CaLightWorldT::SaveToDisk(const char* FileName) const
{
    World.SaveToDisk(FileName);
}


void CaLightWorldT::CreateLightMapsForEnts()
{
    // Pre-cache some sample directional vectors.
    // Code copied from "CaSHL/Init2.cpp".
    const unsigned long SqrtNrOfSamples=8;
    ArrayT<Vector3dT> SampleDirs;

    for (unsigned long SampleX=0; SampleX<SqrtNrOfSamples; SampleX++)
        for (unsigned long SampleY=0; SampleY<SqrtNrOfSamples; SampleY++)
        {
            const double Pi   =3.14159265358979323846;
            const double x    =double(SampleX)/double(SqrtNrOfSamples) + 0.5/double(SqrtNrOfSamples);
            const double y    =double(SampleY)/double(SqrtNrOfSamples) + 0.5/double(SqrtNrOfSamples);
            const double theta=2.0*acos(sqrt(1.0-x));
            const double phi  =2.0*Pi*y;

            SampleDirs.PushBack(Vector3dT(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)));
        }


    for (unsigned long EntNr=0; EntNr<World.GameEntities.Size(); EntNr++)
    {
        cf::SceneGraph::BspTreeNodeT* EntBspTree=World.GameEntities[EntNr]->BspTree;


        // Obtain the patch meshes for this entity.
        ArrayT<cf::PatchMeshT> PatchMeshes;

        for (unsigned long FaceNr=0; FaceNr<EntBspTree->FaceChildren.Size(); FaceNr++)
        {
            ArrayT< ArrayT< ArrayT<Vector3dT> > > SampleCoords;

            EntBspTree->FaceChildren[FaceNr]->CreatePatchMeshes(PatchMeshes, SampleCoords);
        }

        for (unsigned long OtherNr=0; OtherNr<EntBspTree->OtherChildren.Size(); OtherNr++)
        {
            ArrayT< ArrayT< ArrayT<Vector3dT> > > SampleCoords;

            EntBspTree->OtherChildren[OtherNr]->CreatePatchMeshes(PatchMeshes, SampleCoords);
        }


        // Fill patch meshes with color.
        for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        {
            cf::PatchMeshT& PM=PatchMeshes[PatchMeshNr];

            for (unsigned long t=0; t<PM.Height; t++)
                for (unsigned long s=0; s<PM.Width; s++)
                {
                    cf::PatchT& Patch=PM.GetPatch(s, t);

                    Patch.TotalEnergy  =Vector3dT(128, 128, 128);   // Use mid-grey (128, 128, 128) for normal use, and e.g. (255, 0, 0) for debugging.
                 // Patch.EnergyFromDir=Patch.Normal;

                    if (!Patch.InsideFace) continue;


                    // Gather the averages.
                    double        AvgWeightsSum=0.0;
                    Vector3dT     AvgColorsSum;     // Sum of weighted colors.
                    Vector3dT     AvgDirsSum;       // Sum of weighted directions.

                    for (unsigned long SampleNr=0; SampleNr<SampleDirs.Size(); SampleNr++)
                    {
                        // printf("Mesh %lu, st %lu %lu: coord %s, normal %s, SampleDir %s \n", PatchMeshNr, s, t, convertToString(Patch.Coord).c_str(), convertToString(Patch.Normal).c_str(), convertToString(SampleDirs[SampleNr]).c_str());

                        if (dot(Patch.Normal, SampleDirs[SampleNr])<0) continue;    // Don't try sample dirs against the normal.

                        const double RayLength=50000.0;         // 50 meters max.
                        const double HitFrac=TraceRay(Patch.Coord, SampleDirs[SampleNr]*RayLength);

                        if (HitFrac==0.0) continue;     // Probably immediately stuck in solid.
                        if (HitFrac==1.0) continue;     // Ray did not hit anything.

                        const Vector3dT     HitPos=Patch.Coord+SampleDirs[SampleNr]*(HitFrac*RayLength-0.1);
                        const unsigned long LeafNr=World.BspTree->WhatLeaf(HitPos);

                        // printf("    HitFrac %f, HitPos %s, LeafNr %lu\n", HitFrac, convertToString(HitPos).c_str(), LeafNr);

                        for (unsigned long FNr=0; FNr<World.BspTree->Leaves[LeafNr].FaceChildrenSet.Size(); FNr++)
                        {
                            cf::SceneGraph::FaceNodeT* FaceNode=World.BspTree->FaceChildren[World.BspTree->Leaves[LeafNr].FaceChildrenSet[FNr]];
                            Vector3fT                  HitColor;

                            if (!FaceNode->GetLightmapColorNearPosition(HitPos, HitColor)) continue;

                            const double d1    =dot(Patch.Normal, SampleDirs[SampleNr]);
                            const double d2    =-dot(FaceNode->Polygon.Plane.Normal, SampleDirs[SampleNr]);
                            const double Weight=d1*d2/(HitFrac*HitFrac);

                            // printf("    HitColor %s, d1 %f, d2 %f, Weight %f\n", convertToString(HitColor).c_str(), d1, d2, Weight);

                            AvgWeightsSum+=Weight;
                            AvgColorsSum +=HitColor.AsVectorOfDouble()*255.0*Weight;
                            AvgDirsSum   +=SampleDirs[SampleNr]*Weight;
                            break;
                        }
                    }

                    if (AvgWeightsSum>0)
                    {
                        Patch.TotalEnergy  =scale(AvgColorsSum, 1.0/AvgWeightsSum);
                        Patch.EnergyFromDir=scale(AvgDirsSum,   1.0/AvgWeightsSum);
                    }
                }
        }


        // Minimal postprocessing.
        // This is analogous to the first step in PostProcessBorders().
        for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        {
            cf::PatchMeshT& PM=PatchMeshes[PatchMeshNr];

            for (unsigned long t=0; t<PM.Height; t++)
                for (unsigned long s=0; s<PM.Width; s++)
                {
                    cf::PatchT& Patch=PM.GetPatch(s, t);

                    if (Patch.InsideFace) continue;

                    Patch.TotalEnergy =VectorT(0, 0, 0);
                    Vector3dT AvgDir  =VectorT(0, 0, 0);    // Do NOT(!) build the average directly in Patch.EnergyFromDir, or else you may end with zero-vector dirs for patches in the midst of faces that have received no light at all!
                    double CoveredArea=0.0;

                    // Der Patch liegt in der Mitte eines 3x3-Feldes bei Koordinate (1,1).
                    for (char y=0; y<=2; y++)
                        for (char x=0; x<=2; x++)
                        {
                            if (x==1 && y==1) continue;     // Nur die acht umliegenden Patches betrachten.

                            int Nx=int(s+x)-1;
                            int Ny=int(t+y)-1;

                            if (PM.WrapsHorz)
                            {
                                // Patches that wrap do *not* duplicate the leftmost column at the right.
                                if (Nx<             0) Nx+=PM.Width;
                                if (Nx>=int(PM.Width)) Nx-=PM.Width;
                            }

                            if (PM.WrapsVert)
                            {
                                // Patches that wrap do *not* duplicate the topmost column at the bottom.
                                if (Ny<              0) Ny+=PM.Height;
                                if (Ny>=int(PM.Height)) Ny-=PM.Height;
                            }

                            if (Nx<              0) continue;   // Linken  Rand beachten.
                            if (Nx>= int(PM.Width)) continue;   // Rechten Rand beachten.
                            if (Ny<              0) continue;   // Oberen  Rand beachten.
                            if (Ny>=int(PM.Height)) continue;   // Unteren Rand beachten.

                            const double      RelevantArea=(x!=1 && y!=1) ? 0.25 : 0.5;
                            const cf::PatchT& Neighb      =PM.GetPatch(Nx, Ny);

                            if (!Neighb.InsideFace) continue;

                            Patch.TotalEnergy+=Neighb.TotalEnergy  *RelevantArea;
                            AvgDir           +=Neighb.EnergyFromDir*RelevantArea;
                            CoveredArea      +=RelevantArea;
                        }

                    if (CoveredArea>0.0001)
                    {
                        Patch.TotalEnergy  /=CoveredArea;
                        Patch.EnergyFromDir+=AvgDir/CoveredArea;
                    }
                }
        }


        // Write back.
        for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        {
            const cf::PatchMeshT&         PM     =PatchMeshes[PatchMeshNr];
            cf::SceneGraph::GenericNodeT* PM_Node=const_cast<cf::SceneGraph::GenericNodeT*>(PM.Node);

            // Need a non-const pointer to the "source" NodeT of the patch mesh here.
            PM_Node->BackToLightMap(PM);
        }
    }
}
