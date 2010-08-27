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

/*************************************/
/***                               ***/
/***    Cafu Map Debugging Tool    ***/
/***                               ***/
/*** Never touch a running system. ***/
/*** Never change a winning  team. ***/
/***                               ***/
/*************************************/

#ifdef _WIN32
#include <conio.h>
#endif

#ifdef _WIN32
    #if defined(_MSC_VER)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
    #endif
#endif

#include <stdio.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "Templates/Array.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "FileSys/Password.hpp"
#include "Math3D/Brush.hpp"
#include "Bitmap/Bitmap.hpp"
#include "OpenGL/OpenGLWindow.hpp"
#include "Util/Util.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "ClipSys/CollisionModelMan_impl.hpp"

#include "../Common/World.hpp"


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

static cf::ClipSys::CollModelManImplT CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=&CCM;

ConsoleInterpreterI* ConsoleInterpreter=NULL;
MaterialManagerI*    MaterialManager   =NULL;


WorldT* World=NULL;


bool Visible(unsigned long L1, unsigned long L2)
{
    unsigned long PVSTotalBitNr=L1*World->BspTree->Leaves.Size()+L2;
    unsigned long PVS_W32_Nr   =PVSTotalBitNr >> 5;

    return bool((World->BspTree->PVS[PVS_W32_Nr] >> (PVSTotalBitNr & 31)) & 1);
}


void ExportLightMaps(const char* WorldPathName)
{
    char WorldName[256]="unknown";

    // Strip path and extension from 'WorldPathName' first.
    if (WorldPathName)
    {
        // Dateinamen abtrennen (mit Extension).
        size_t i=strlen(WorldPathName);

        while (i>0 && WorldPathName[i-1]!='/' && WorldPathName[i-1]!='\\') i--;
        strncpy(WorldName, WorldPathName+i, 256);
        WorldName[255]=0;

        // Extension abtrennen.
        i=strlen(WorldName);

        while (i>0 && WorldName[i-1]!='.') i--;
        if (i>0) WorldName[i-1]=0;
    }

    for (unsigned long LMNr=0; LMNr<World->LightMapMan.Bitmaps.Size(); LMNr++)
    {
        printf("LM_%s_%02lu\n", WorldName, LMNr);

        char LMNamePNG[200];
        sprintf(LMNamePNG, "LM_%s_%02lu.png", WorldName, LMNr);

        World->LightMapMan.Bitmaps[LMNr]->SaveToDisk(LMNamePNG);
    }
}


void ExportPVS()
{
    for (unsigned long Leaf1Nr=0; Leaf1Nr<World->BspTree->Leaves.Size(); Leaf1Nr++)
    {
        printf("%5lu: ", Leaf1Nr);

        for (unsigned long Leaf2Nr=0; Leaf2Nr<World->BspTree->Leaves.Size(); Leaf2Nr++)
        {
            const unsigned long PVSTotalBitNr=Leaf1Nr*World->BspTree->Leaves.Size()+Leaf2Nr;

            if ((World->BspTree->PVS[PVSTotalBitNr >> 5] >> (PVSTotalBitNr & 31)) & 1)
            {
                printf("%5lu", Leaf2Nr);
                if (Leaf2Nr<World->BspTree->Leaves.Size()-1) printf(" ");
            }
        }

        printf("\n");
    }
}


void PrintMaterialCounts(int Mode)
{
    // Would be nice to implement this with a hash...
    ArrayT<MaterialT*>    Materials;
    ArrayT<unsigned long> Counts;
    ArrayT<double>        Areas;

    if (Mode & 1)
    {
        printf("Counts and areas for faces...\n");
        for (unsigned long FaceNr=0; FaceNr<World->BspTree->FaceChildren.Size(); FaceNr++)
        {
            MaterialT* Mat  =World->BspTree->FaceChildren[FaceNr]->Material;
            int        Index=Materials.Find(Mat);

            if (Index>=0)
            {
                Counts[Index]++;
                Areas [Index]+=World->BspTree->FaceChildren[FaceNr]->Polygon.GetArea()/1000000.0;
            }
            else
            {
                Materials.PushBack(Mat);
                Counts.PushBack(1);
                Areas .PushBack(World->BspTree->FaceChildren[FaceNr]->Polygon.GetArea()/1000000.0);
            }
        }
    }


    // Print out the results.
    unsigned long TotalCount=0;

    for (unsigned long MatNr=0; MatNr<Materials.Size(); MatNr++)
    {
        if (Mode==1) printf("%5lu %12.2f %s\n", Counts[MatNr], Areas[MatNr], Materials[MatNr]->Name.c_str());
                else printf("%5lu %s\n", Counts[MatNr], Materials[MatNr]->Name.c_str());
        TotalCount+=Counts[MatNr];
    }
    printf("Total: %lu\n\n", TotalCount);
}


void ViewWorld()
{
    VectorT Viewer   =World->InfoPlayerStarts[0].Origin;
    float   Heading  =  0.0;
    float   Pitch    =  0.0;
    float   MoveSpeed=100.0;
    float   RotSpeed =  5.0;

    bool          r_pvs_enabled=true;
    unsigned long DrawLeafNr   =831;

    ArrayT<bool>          FaceIsInPVS;
    ArrayT<unsigned long> OrderedFaces;

    FaceIsInPVS .PushBackEmpty(World->BspTree->FaceChildren.Size());
    OrderedFaces.PushBackEmpty(World->BspTree->FaceChildren.Size());


    const char* ErrorMsg=SingleOpenGLWindow->Open("Simple World Viewer", 1024, 768, 16, false);

    if (ErrorMsg)
    {
        printf("\nUnable to open OpenGL window: %s\n", ErrorMsg);
        return;
    }


    while (true)
    {
        // Rufe die Nachrichten der Windows-Nachrichtenschlange ab.
        if (SingleOpenGLWindow->HandleWindowMessages()) break;

        // Draw the model
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        glRotatef(Pitch  , 1.0, 0.0, 0.0);
        glRotatef(Heading, 0.0, 1.0, 0.0);
        glTranslatef(float(-Viewer.x), float(-Viewer.z), float(Viewer.y));


        static ArrayT<unsigned long> OrderedLeaves;
        const unsigned long ViewerLeafNr=World->BspTree->WhatLeaf(Viewer);
        World->BspTree->GetLeavesOrderedBackToFront(OrderedLeaves, Viewer);

        for (unsigned long FaceNr=0; FaceNr<World->BspTree->FaceChildren.Size(); FaceNr++) FaceIsInPVS[FaceNr]=false;

        for (unsigned long OrderNr=0; OrderNr<OrderedLeaves.Size(); OrderNr++)
        {
            const unsigned long LeafNr=OrderedLeaves[OrderNr];
            const cf::SceneGraph::BspTreeNodeT::LeafT& L=World->BspTree->Leaves[LeafNr];

            if (r_pvs_enabled && !World->BspTree->IsInPVS(LeafNr, ViewerLeafNr)) continue;

            OrderedFaces.PushBack(L.FaceChildrenSet);

            for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
                FaceIsInPVS[L.FaceChildrenSet[SetNr]]=true;
        }


        // if (r_style==SolidPolygons)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            for (unsigned long OrderedFaceNr=0; OrderedFaceNr<OrderedFaces.Size(); OrderedFaceNr++)
            {
                const unsigned long FaceNr=OrderedFaces[OrderedFaceNr];
                const Polygon3T<double>& F=World->BspTree->FaceChildren[FaceNr]->Polygon;
                char                Alpha =255; // World->BspTree->FaceChildren[FaceNr]->TI.Alpha;

                if (Alpha>80) Alpha-=40; else Alpha/=2;

                glColor4ub(char(((FaceNr >> 0) & 3) << 6)+63,
                           char(((FaceNr >> 2) & 3) << 6)+63,
                           char(((FaceNr >> 4) & 3) << 6)+63, Alpha);

                glBegin(GL_TRIANGLE_FAN);
                    for (unsigned long VertexNr=0; VertexNr<F.Vertices.Size(); VertexNr++)
                        glVertex3d(F.Vertices[VertexNr].x, F.Vertices[VertexNr].z, -F.Vertices[VertexNr].y);
                glEnd();
            }

            glDisable(GL_BLEND);


            // Spezielle Teile (Leaf 'DrawLeafNr') rendern.
            {
                glColor3f(0.5, 0.5, 1.0);
                glDisable(GL_DEPTH_TEST);

                if (DrawLeafNr<World->BspTree->Leaves.Size())
                {
                    for (unsigned long PortalNr=0; PortalNr<World->BspTree->Leaves[DrawLeafNr].Portals.Size(); PortalNr++)
                    {
                        const Polygon3T<double>& Portal=World->BspTree->Leaves[DrawLeafNr].Portals[PortalNr];

                        glBegin(GL_LINE_LOOP);
                            for (unsigned long VertexNr=0; VertexNr<Portal.Vertices.Size(); VertexNr++)
                                glVertex3d(Portal.Vertices[VertexNr].x, Portal.Vertices[VertexNr].z, -Portal.Vertices[VertexNr].y);
                        glEnd();
                    }
                }
            }

            glEnable(GL_DEPTH_TEST);
        }

        SingleOpenGLWindow->SwapBuffers();


        CaKeyboardEventT KE;
        bool QuitProgram=false;

        while (SingleOpenGLWindow->GetNextKeyboardEvent(KE)>0)
        {
            if (KE.Type!=CaKeyboardEventT::CKE_KEYDOWN) continue;

            if (KE.Key==CaKeyboardEventT::CK_ESCAPE) QuitProgram=true;

            switch (KE.Key)
            {
                case CaKeyboardEventT::CK_ADD     : MoveSpeed*=2.0; printf("Movement speed doubled.\n"); break;
                case CaKeyboardEventT::CK_SUBTRACT: MoveSpeed/=2.0; printf("Movement speed halved.\n"); break;
                case CaKeyboardEventT::CK_L:
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_LSHIFT] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RSHIFT])
                    {
                        // Großes 'L'.
                        DrawLeafNr=World->BspTree->WhatLeaf(Viewer);
                        printf("DrawLeafNr==%lu\n", DrawLeafNr);
                        break;
                    }
                    else
                    {
                        // Kleines 'l'.
                        unsigned long LeafNr=World->BspTree->WhatLeaf(Viewer);
                        bool          InnerL=World->BspTree->Leaves[LeafNr].IsInnerLeaf;

                        printf("WhatLeaf(Viewer)==%lu (\"%s\")\n", LeafNr, InnerL ? "inner" : "outer");
                        printf("%5lu Faces: ", World->BspTree->Leaves[LeafNr].FaceChildrenSet.Size());
                        for (unsigned long SetNr=0; SetNr<World->BspTree->Leaves[LeafNr].FaceChildrenSet.Size(); SetNr++) printf("%5lu", World->BspTree->Leaves[LeafNr].FaceChildrenSet[SetNr]);
                        printf("\n");
                        break;
                    }
                case CaKeyboardEventT::CK_O:
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_LSHIFT] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RSHIFT])
                    {
                        // Großes 'O'.
                        if (DrawLeafNr==0) DrawLeafNr=World->BspTree->Leaves.Size()-1;
                                      else DrawLeafNr--;
                        printf("DrawLeafNr==%lu\n", DrawLeafNr);
                        break;
                    }
                    else
                    {
                        // Kleines 'o'.
                        DrawLeafNr++;
                        if (DrawLeafNr>=World->BspTree->Leaves.Size()) DrawLeafNr=0;
                        printf("DrawLeafNr==%lu\n", DrawLeafNr);
                        break;
                    }
                case CaKeyboardEventT::CK_P: printf("Position: (%.3f %.3f %.3f)\n", Viewer.x, Viewer.y, Viewer.z); break;
                default: break;
            }
        }

        const float vx=float(MoveSpeed*sin(Heading/180.0*3.1415926));
        const float vy=float(MoveSpeed*cos(Heading/180.0*3.1415926));

        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_UP    ] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_W]) Viewer=Viewer+VectorT( vx,  vy, 0);
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_DOWN  ] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_S]) Viewer=Viewer+VectorT(-vx, -vy, 0);
        if (                                                                       SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_A]) Viewer=Viewer+VectorT(-vy,  vx, 0);
        if (                                                                       SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_D]) Viewer=Viewer+VectorT( vy, -vx, 0);
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_INSERT] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_R]) Viewer.z+=MoveSpeed;
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_DELETE] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_F]) Viewer.z-=MoveSpeed;
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_LEFT  ]                                                                  ) Heading-=RotSpeed;
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RIGHT ]                                                                  ) Heading+=RotSpeed;
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGUP  ]                                                                  ) Pitch-=RotSpeed;
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGDN  ]                                                                  ) Pitch+=RotSpeed;
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_END   ]                                                                  ) Pitch=0.0;

        if (QuitProgram) break;
    }

    SingleOpenGLWindow->Close();
}


void PrintMapInfo()
{
    struct HelperT
    {
        const WorldT* HelperWorld;

        unsigned long CurrentDepth;
        unsigned long MinDepth;
        unsigned long MaxDepth;
        unsigned long TotalDepthSum;
        unsigned long SeperateLeafCount;


        HelperT(const WorldT* World_) : HelperWorld(World_)
        {
            CurrentDepth     =0;
            MinDepth         =0xFFFFFFFF;
            MaxDepth         =0;
            TotalDepthSum    =0;
            SeperateLeafCount=0;
        }


        void WalkDepthTree(unsigned long NodeNr)
        {
            CurrentDepth++;

            if (HelperWorld->BspTree->Nodes[NodeNr].FrontIsLeaf)
            {
                if (MinDepth>CurrentDepth) MinDepth=CurrentDepth;
                if (MaxDepth<CurrentDepth) MaxDepth=CurrentDepth;

                TotalDepthSum+=CurrentDepth;
                SeperateLeafCount++;
            }
            else WalkDepthTree(HelperWorld->BspTree->Nodes[NodeNr].FrontChild);

            if (HelperWorld->BspTree->Nodes[NodeNr].BackIsLeaf)
            {
                if (MinDepth>CurrentDepth) MinDepth=CurrentDepth;
                if (MaxDepth<CurrentDepth) MaxDepth=CurrentDepth;

                TotalDepthSum+=CurrentDepth;
                SeperateLeafCount++;
            }
            else WalkDepthTree(HelperWorld->BspTree->Nodes[NodeNr].BackChild);

            CurrentDepth--;
        }
    } Helper(World);

    printf("Map summary:\n");
    printf("Faces         : %6lu\n", World->BspTree->FaceChildren.Size());
    printf("Nodes         : %6lu\n", World->BspTree->Nodes.Size());
    printf("Leaves        : %6lu\n", World->BspTree->Leaves.Size());

    printf("\nBSP tree data:\n");
    Helper.WalkDepthTree(0);
    printf("Leaf count         : %6lu\n", Helper.SeperateLeafCount);
    printf("Maximum tree depth : %6lu\n", Helper.MaxDepth);
    printf("Minimum tree depth : %6lu\n", Helper.MinDepth);
    printf("Average tree depth : %6lu\n", Helper.TotalDepthSum/Helper.SeperateLeafCount);

    printf("\nPVS, LightMap, and SHL info:\n");
    printf("PVS size (bytes)         : %12lu\n", World->BspTree->PVS.Size()*4);
    unsigned long Checksum=0;
    for (unsigned long Count=0; Count<World->BspTree->PVS.Size(); Count++)
    {
        Checksum+=(World->BspTree->PVS[Count] >> 24) & 0xFF;
        Checksum+=(World->BspTree->PVS[Count] >> 16) & 0xFF;
        Checksum+=(World->BspTree->PVS[Count] >>  8) & 0xFF;
        Checksum+=(World->BspTree->PVS[Count]      ) & 0xFF;
    }
    printf("PVS checksum             : %12lu\n", Checksum);
    printf("\n");
    printf("LightMapMan.Bitmaps      : %12lu  (at %ux%u, PatchSize %.2f)\n", World->LightMapMan.Bitmaps.Size(), cf::SceneGraph::LightMapManT::SIZE_S, cf::SceneGraph::LightMapManT::SIZE_T, cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize);
    printf("LightMaps total size     : %12lu\n", World->LightMapMan.Bitmaps.Size()*(cf::SceneGraph::LightMapManT::SIZE_S*cf::SceneGraph::LightMapManT::SIZE_T*3+4));
    printf("\n");
    printf("SHL Bands                : %12u  (%u^2 == %u coefficients)\n", cf::SceneGraph::SHLMapManT::NrOfBands, cf::SceneGraph::SHLMapManT::NrOfBands, cf::SceneGraph::SHLMapManT::NrOfBands*cf::SceneGraph::SHLMapManT::NrOfBands);
    printf("SHL Representatives      : %12u  %s\n", cf::SceneGraph::SHLMapManT::NrOfRepres, cf::SceneGraph::SHLMapManT::NrOfRepres>0 ? "" : "(NO compression)");
    printf("SHL patch size           : %12.1f\n", cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize);
    printf("SHLMaps                  : %12lu  (at %lux%lu)\n", World->SHLMapMan.SHLMaps.Size(), cf::SceneGraph::SHLMapManT::SIZE_S, cf::SceneGraph::SHLMapManT::SIZE_T);

    if (cf::SceneGraph::SHLMapManT::NrOfRepres>0)
    {
        const unsigned long NR_OF_SH_COEFFS    =cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;
        const unsigned long NrOfColumns        =(cf::SceneGraph::SHLMapManT::NrOfRepres+255)/256;   // =ceil(double(cf::SceneGraph::SHLMapManT::NrOfRepres)/256);
        const unsigned long NrOfPixelsPerVector=(NR_OF_SH_COEFFS+3)/4;

        unsigned long Width=1; while (Width<NrOfColumns*NrOfPixelsPerVector) Width*=2;

        printf("There are %u representatives with %lu SH coeffs each.\n", cf::SceneGraph::SHLMapManT::NrOfRepres, NR_OF_SH_COEFFS);
        printf("They are stored in %lu columns (%lu pixels per column).\n", NrOfColumns, NrOfPixelsPerVector);
        printf("The look-up texture thus has dimensions %lu x 256.\n", Width);
    }

    printf("\n\nDo you want to see the face details? (press 'y' to confirm)\n");
    if (_getch()=='y')
        for (unsigned long FaceNr=0; FaceNr<World->BspTree->FaceChildren.Size(); FaceNr++)
        {
            const cf::SceneGraph::FaceNodeT* FN=World->BspTree->FaceChildren[FaceNr];

            printf("\n");
            printf("Face%5lu, MatName '%s', LightMap %3u, Plane (%7.4f %7.4f %7.4f) %14.3f\n", FaceNr, FN->Material->Name.c_str(), FN->LightMapInfo.LightMapNr, FN->Polygon.Plane.Normal.x, FN->Polygon.Plane.Normal.y, FN->Polygon.Plane.Normal.z, FN->Polygon.Plane.Dist);

            for (unsigned long VertexNr=0; VertexNr<FN->Polygon.Vertices.Size(); VertexNr++)
                printf("    (%14.3f %14.3f %14.3f)\n", FN->Polygon.Vertices[VertexNr].x, FN->Polygon.Vertices[VertexNr].y, FN->Polygon.Vertices[VertexNr].z);
        }
}


void TestValidityOfFacesAndPortals()
{
    for (unsigned long FaceNr=0; FaceNr<World->BspTree->FaceChildren.Size(); FaceNr++)
        if (!World->BspTree->FaceChildren[FaceNr]->Polygon.IsValid(MapT::RoundEpsilon, MapT::MinVertexDist))
            printf("Face   %4lu is INVALID!\n", FaceNr);

    for (unsigned long LeafNr=0; LeafNr<World->BspTree->Leaves.Size(); LeafNr++)
    {
        const cf::SceneGraph::BspTreeNodeT::LeafT& L=World->BspTree->Leaves[LeafNr];

        if (!L.IsInnerLeaf && L.FaceChildrenSet.Size()>0)
        {
            VectorT Near=World->BspTree->FaceChildren[L.FaceChildrenSet[0]]->Polygon.Vertices[0];

            printf("Outer Leaf %4lu has a non-empty FaceChildrenSet!\n",   LeafNr);
            printf("FaceChildrenSet of that leaf:\n");
            for (unsigned long FaceNr=0; FaceNr<L.FaceChildrenSet.Size(); FaceNr++) printf("%lu ", L.FaceChildrenSet[FaceNr]);
            printf("\nThe 1st vertex of the 1st face is at\n   (%14.3f %14.3f %14.3f).\n", Near.x, Near.y, Near.z);
        }

        if (!L.IsInnerLeaf && L.Portals.Size()>0) printf("Outer Leaf %4lu has a non-empty PortalSet!\n", LeafNr);

        for (unsigned long PortalNr=0; PortalNr<L.Portals.Size(); PortalNr++)
            if (!L.Portals[PortalNr].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist))
                printf("Portal %4lu (Leaf %4lu) is INVALID!\n", PortalNr, LeafNr);
    }
}


void TestPVSIntegrity()
{
    unsigned long Leaf1Nr;

    for (Leaf1Nr=0; Leaf1Nr<World->BspTree->Leaves.Size(); Leaf1Nr++)
        for (unsigned long Leaf2Nr=0; Leaf2Nr<World->BspTree->Leaves.Size(); Leaf2Nr++)
            if (Visible(Leaf1Nr, Leaf2Nr) && !Visible(Leaf2Nr, Leaf1Nr))
                printf("WARNING: Can see from%5lu to%5lu, but not vice versa!\n", Leaf1Nr, Leaf2Nr);

    for (Leaf1Nr=0; Leaf1Nr<World->BspTree->Leaves.Size(); Leaf1Nr++)
        for (unsigned long Leaf2Nr=0; Leaf2Nr<World->BspTree->Leaves.Size(); Leaf2Nr++)
            if (Visible(Leaf1Nr, Leaf2Nr) && (!World->BspTree->Leaves[Leaf1Nr].IsInnerLeaf || !World->BspTree->Leaves[Leaf2Nr].IsInnerLeaf))
                printf("WARNING: Can see from%5lu to%5lu, but%5lu is \"%s\", and%5lu is \"%s\"!\n",
                    Leaf1Nr, Leaf2Nr, Leaf1Nr, World->BspTree->Leaves[Leaf1Nr].IsInnerLeaf ? "inner" : "outer", Leaf2Nr, World->BspTree->Leaves[Leaf2Nr].IsInnerLeaf ? "inner" : "outer");
}


void Test10SmallestPortals()
{
    printf("   Note: If two or more portals have the same area, only ONE of them is listed!\n");

    for (char Nr=0; Nr<10; Nr++)
    {
        static double LastWinnerArea=0;
        unsigned long CurrentLNr    =World->BspTree->Leaves.Size();
        unsigned long CurrentPNr    =0;
        double        CurrentArea   =1000000.0;

        for (unsigned long LeafNr=0; LeafNr<World->BspTree->Leaves.Size(); LeafNr++)
        {
            const cf::SceneGraph::BspTreeNodeT::LeafT& L=World->BspTree->Leaves[LeafNr];

            for (unsigned long PortalNr=0; PortalNr<L.Portals.Size(); PortalNr++)
            {
                double PortalArea=L.Portals[PortalNr].GetArea();

                if (PortalArea<CurrentArea && PortalArea>LastWinnerArea)
                {
                    CurrentLNr =LeafNr;
                    CurrentPNr =PortalNr;
                    CurrentArea=PortalArea;
                }
            }
        }

        if (CurrentLNr==World->BspTree->Leaves.Size()) { printf("WARNING: NOT SO MANY PORTALS!?!\n"); break; }

        VectorT V=World->BspTree->Leaves[CurrentLNr].Portals[CurrentPNr].Vertices[0];
        printf("%2u, L#%5lu, P#%3lu, Vertex0 (%9.2f %9.2f %9.2f), Area: %10.2f\n", Nr+1, CurrentLNr, CurrentPNr, V.x, V.y, V.z, CurrentArea);

        LastWinnerArea=CurrentArea;
    }
}


void Usage()
{
    printf("\nUSAGE:\n");
    printf("CaSanity WorldName                    for normal operation\n");
    printf("CaSanity WorldName -ListTexNames      list texture names  (OBSOLETE!!)\n");
    printf("CaSanity WorldName -GetSkyName        print sky name      (OBSOLETE!!)\n");
    printf("CaSanity WorldName -GetTerrainMaps    print terrain map names\n");
    printf("CaSanity WorldName -ExportLightMaps   export lightmaps to BMP files\n");
    printf("CaSanity WorldName -ExportPVS         export PVS (to stdout)\n");
    printf("CaSanity WorldName -PrintMatCounts    print material usage counts\n");
    printf("CaSanity WorldName -ViewWorld         simple OpenGL world viewer\n");
    exit(0);
}


int main(int ArgC, char* ArgV[])
{
    if (ArgC>1 && std::string(ArgV[1])=="-p")
    {
        // Generate a password
        if (ArgC!=4) { printf("CaSanity -p length seed\n"); exit(0); }

        const unsigned long Length=atoi(ArgV[2]);
        const unsigned long Seed  =atoi(ArgV[3]);

        printf("OK, Length %lu, Seed %lu.\n", Length, Seed);

        const ArrayT<unsigned char> Password=cf::Password::GenerateAsciiPassword(Length, Seed);
        const ArrayT<unsigned char> ObfusStr=cf::Password::GenerateObfuscationString(Length);
        const ArrayT<unsigned char> ObfusPwd=cf::Password::XOR(Password, ObfusStr);
        const ArrayT<unsigned char> VerifyPw=cf::Password::XOR(ObfusPwd, ObfusStr);     // Must be identical to Password.

        // The user should enter this into WinZip, 7-zip, InfoZip, ... to create the encrypted archive.
        printf("Password: %s\n", cf::Password::ToString(Password).c_str());

        // Output the obfuscated password as array of characters (C-Code).
        printf("C array init code: %s\n", cf::Password::GenerateArrayCode(ObfusPwd).c_str());

        // Make sure that VerifyPw==Password.
        printf("Test de-obfuscation: %s\n", (Password.Size()==VerifyPw.Size() && cf::Password::ToString(Password)==cf::Password::ToString(VerifyPw)) ? "OK" : " !!! FAIL !!!");
        exit(0);
    }

    if (ArgC<2 || ArgC>3) Usage();
    if (ArgC==2) printf("\nCafu Map Debugging Tool, Version 02 (%s)\n\n", __DATE__);

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");


    std::string GameDirectory=ArgV[1];

    // Determine the game directory, cleverly assuming that the destination file is in "Worlds".
    {
        // Strip the file name and extention off.
        size_t i=GameDirectory.find_last_of("/\\");

        GameDirectory=GameDirectory.substr(0, i==std::string::npos ? 0 : i)+"/..";
    }


    // Setup the global MaterialManager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    if (MaterialManager->RegisterMaterialScriptsInDir(GameDirectory+"/Materials", GameDirectory+"/").Size()==0)
    {
        printf("\nNo materials found in scripts in \"%s/Materials\".\n", GameDirectory.c_str());
        printf("No materials found.\n\n");
        Usage();
    }


    try
    {
        if (ArgC==2) printf("Loading world %s...\n\n", ArgV[1]);
        World=new WorldT(ArgV[1]);

        // Gib ggf. nur die gewünschten Infos aus
        if (ArgC==3)
        {
                 if (!_stricmp(ArgV[2], "-ListTexNames"   )) printf("%s\n", "NOT SUPPORTED!");
            else if (!_stricmp(ArgV[2], "-GetSkyName"     )) printf("%s\n", "NOT SUPPORTED!");
            else if (!_stricmp(ArgV[2], "-GetTerrainMaps" )) printf("%s\n", "NOT SUPPORTED!");
            else if (!_stricmp(ArgV[2], "-ExportLightMaps")) ExportLightMaps(ArgV[1]);
            else if (!_stricmp(ArgV[2], "-ExportPVS"      )) ExportPVS();
            else if (!_stricmp(ArgV[2], "-PrintMatCounts" )) { PrintMaterialCounts(1); PrintMaterialCounts(2); PrintMaterialCounts(3); }
            else if (!_stricmp(ArgV[2], "-ViewWorld"      )) ViewWorld();
            else                                            Usage();

            return 0;
        }

        PrintMapInfo();

        printf("\n\n--- Now, several sanity checks will be performed on this map.  ---\n");
        printf("--- Press any key after each test to begin with the next test. ---\n\n");
        _getch();

        printf("\n1. Validity of face and portal polygons:\n");  TestValidityOfFacesAndPortals(); printf("Done. Press any key for the next test.\n\n"); _getch();
        printf("\n2. PVS integrity:\n");                         TestPVSIntegrity();              printf("Done. Press any key for the next test.\n\n"); _getch();
        printf("\n3. The 10 portals with the smallest area:\n"); Test10SmallestPortals();         printf("Done. Press any key for the next test.\n\n"); _getch();
     // printf("\n4. LightMap Patch ratios:\n");                 LightMapPatchRatios();           printf("Done. Press any key for the next test.\n\n"); _getch();

        // TO DO: Hier mal MapT::FillTJunctions aufrufen,
        //        und prüfen, daß keine Vertices in einer Face doppelt vorkommen (sollte generell ein Prüfpunkt sein!)
        //        und zählen, wieviele Vertices es vorher und nachher sind, Erhöhung in % ausgeben.
        // TO DO: Existieren Vertices, deren Abstand <GeometryEpsilon ist, die aber ungleich sind??
        //        Mit anderen Worten: Vertices A und B mit VectorEqual(A, B)==true müssen BITWEISE gleich sein.
        // TO DO: Prüfe, ob Vertices, die in einer Plane liegen, DOCH einen Abstand zur Plane haben.
        // TO DO: Prüfe, ob Faces mit Sky-Texture wirklich LightMap.SizeS==LightMap.SizeT==LightMap.Data.Size()==0 haben!
        // TO DO: Bestimme die maximale Lightmap.Size und prüfe, ob SizeS oder SizeT > 128 !

        delete World;
        printf("--- All tests are completed! Press any key to leave. ---\n");
        _getch();
    }
    catch (const WorldT::LoadErrorT& E)
    {
        printf("\nFATAL ERROR: %s\n", E.Msg);
        printf("Program aborted.\n\n");
    }

    return 0;
}
