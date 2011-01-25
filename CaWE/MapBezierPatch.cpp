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

#include "wx/wx.h"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "EditorMaterial.hpp"
#include "EditorMaterialManager.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "Options.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MapBezierPatch.hpp"
#include "MapFile.hpp"
#include "MapDocument.hpp"
#include "ChildFrame.hpp"
#include "ToolEditSurface.hpp"
#include "ToolManager.hpp"
#include "DialogEditSurfaceProps.hpp"

#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/TraceResult.hpp"
#include "MaterialSystem/Material.hpp"
#include "Math3D/BezierPatch.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "TypeSys.hpp"


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif

#if defined(_WIN32) && defined(_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


/*** Begin of TypeSys related definitions for this class. ***/

void* MapBezierPatchT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapBezierPatchT::TypeInfo(GetMapElemTIM(), "MapBezierPatchT", "MapPrimitiveT", MapBezierPatchT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapBezierPatchT::MapBezierPatchT(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, int SubdivsHorz_, int SubdivsVert_)
    : MapPrimitiveT(wxColour(0, 100 + (rand() % 156), 100 + (rand() % 156))),
      cv_Pos(),
      cv_UVs(),
      cv_Width(0),
      cv_Height(0),
      SubdivsHorz(SubdivsHorz_),
      SubdivsVert(SubdivsVert_),
      NeedsUpdate(true),
      SurfaceInfo(),
      LMM(LMM_),
      BPRenderMesh(new cf::SceneGraph::BezierPatchNodeT(LMM, 400.0f/cf::CA3DE_SCALE)),
      CollModel(NULL),
      Material(NULL)
{
    SetMaterial(Material_);
}


MapBezierPatchT::MapBezierPatchT(const MapBezierPatchT& BP)
    : MapPrimitiveT(BP),
      cv_Pos(BP.cv_Pos),
      cv_UVs(BP.cv_UVs),
      cv_Width(BP.cv_Width),
      cv_Height(BP.cv_Height),
      SubdivsHorz(BP.SubdivsHorz),
      SubdivsVert(BP.SubdivsVert),
      NeedsUpdate(true),
      SurfaceInfo(BP.SurfaceInfo),
      LMM(BP.LMM),
      BPRenderMesh(new cf::SceneGraph::BezierPatchNodeT(LMM, 400.0f/cf::CA3DE_SCALE)),
      CollModel(NULL),
      Material(BP.Material)
{
    // UpdateRenderMesh();      // No need to call this here.
}


MapBezierPatchT::~MapBezierPatchT()
{
    delete CollModel;
    delete BPRenderMesh;
}


MapBezierPatchT* MapBezierPatchT::CreateSimplePatch(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long width, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(width, height);

    float sx = fabs(max.x - min.x)/(float)(BP->cv_Width - 1);
    float sy = -(fabs(max.y - min.y)/(float)(BP->cv_Height - 1));
    float px, py, pz;

    px = min.x;
    py = max.y;
    pz = max.z - ((max.z-min.z)*0.5);

    for (unsigned long y=0; y<BP->cv_Height; y++)
    {
        for (unsigned long x=0; x<BP->cv_Width; x++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));
            px += sx;
        }
        px = min.x;
        py += sy;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreatePatchCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, height);

    // init the points
    double stepSizeX = (max.x - min.x) / 2;
    double stepSizeY = (max.y - min.y) / 2;
    double stepSizeZ = (max.z - min.z) / (BP->cv_Height - 1);

    // define from left/center/top
    double px;
    double py;
    double pz = max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x == 2 || x == 6)
            px = min.x + stepSizeX;
        else if (x == 3 || x == 4 || x == 5)
            px = max.x;
        else
            px = min.x;

        if (x == 1 || x == 2 || x == 3)
            py = min.y;
        else if (x == 5 || x == 6 || x == 7)
            py = max.y;
        else
            py = min.y + stepSizeY;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz -= stepSizeZ;
        }
        pz = max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateSquareCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, height);

    // init the points
    double stepSizeX = (max.x - min.x) / 2;
    double stepSizeY = (max.y - min.y) / 2;
    double stepSizeZ = (max.z - min.z) / (BP->cv_Height - 1);

    // define from left/center/top
    double px;
    double py;
    double pz = max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x == 1 || x == 5)
            px = min.x + stepSizeX;
        else if (x == 2 || x == 3 || x == 4)
            px = max.x;
        else
            px = min.x;

        if (x == 3 || x == 7)
            py = min.y + stepSizeY;
        else if (x == 4 || x == 5 || x == 6)
            py = max.y;
        else
            py = min.y;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz -= stepSizeZ;
        }
        pz = max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateQuarterCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(3, height);

    // Init the stepsize in z direction.
    double stepSizeZ=(max.z-min.z)/(BP->cv_Height-1);

    // Vector parts of each vertex.
    double px;
    double py;
    double pz=max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x==0)
            px=min.x;
        else
            px=max.x;

        if (x==2)
            py=max.y;
        else
            py=min.y;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz-=stepSizeZ;
        }
        pz=max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateHalfCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(5, height);

    // Initialize step size in all axis directions.
    double stepSizeX=(max.x - min.x)/2;
    double stepSizeZ=(max.z - min.z)/(BP->cv_Height - 1);

    // Control vertex components.
    double px;
    double py;
    double pz=max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x==2)
            px=min.x + stepSizeX;
        else if (x==3 || x==4)
            px=max.x;
        else
            px=min.x;

        if (x==1 || x==2 || x==3)
            py=min.y;
        else
            py=max.y;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz-=stepSizeZ;
        }
        pz=max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateEdgePipe(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, 3);

    // Init the steps.
    double stepSizeX=(max.x - min.x)/2;
    double stepSizeY=(max.y - min.y)/2;
    double stepSizeZ=(max.z - min.z)/(BP->cv_Height - 1);

    // Vector parts of each control vertex.
    double px;
    double py;
    double pz;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x==1 || x==2 || x==3)
            py=min.y;
        else if (x==5 || x==6 || x==7)
            py=max.y;
        else
            py=min.y+stepSizeY;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            if (x==0 || x==1 || x==7 || x==8)
            {
                if      (y==0) { px=max.x; pz=max.z; }
                else if (y==1) { px=min.x; pz=max.z; }
                else           { px=min.x; pz=min.z; }
            }
            else if (x==2 || x==6)
            {
                if      (y==0) { px=max.x;             pz=max.z-stepSizeZ/2; }
                else if (y==1) { px=min.x+stepSizeX/2; pz=max.z-stepSizeZ/2; }
                else           { px=min.x+stepSizeX/2; pz=min.z; }
            }
            else
            {
                if      (y==0) { px=max.x;           pz=max.z-stepSizeZ; }
                else if (y==1) { px=min.x+stepSizeX; pz=max.z-stepSizeZ; }
                else           { px=min.x+stepSizeX; pz=min.z; }
            }
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));
        }
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateCone(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, height);

    // Init the steps.
    double stepSizeX=(max.x - min.x)/2/(BP->cv_Height-1);
    double stepSizeY=(max.y - min.y)/2/(BP->cv_Height-1);
    double stepSizeZ=(max.z - min.z)  /(BP->cv_Height-1);

    // Vector parts of each vertex.
    double px;
    double py;
    double pz=max.z;

    for (unsigned long y=0; y<BP->cv_Height; y++)
    {
        for (unsigned long x=0; x<BP->cv_Width; x++)
        {
            if (x==2 || x==6)
                px=min.x+stepSizeX*(BP->cv_Height-1);
            else if (x==3 || x==4 || x==5)
                px=max.x-stepSizeX*(BP->cv_Height-1-y);
            else
                px=min.x+stepSizeX*(BP->cv_Height-1-y);

            if (x==1 || x==2 || x==3)
                py=min.y+stepSizeY*(BP->cv_Height-1-y);
            else if (x==5 || x==6 || x==7)
                py=max.y-stepSizeY*(BP->cv_Height-1-y);
            else
                py=min.y+stepSizeY*(BP->cv_Height-1);

                BP->SetCvPos(x, y, Vector3fT(px, py, pz));
        }

        pz -= stepSizeZ;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateSphere(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, 5);

    double stepSizeX=(max.x - min.x)/2;
    double stepSizeY=(max.y - min.y)/2;
    double stepSizeZ=(max.z - min.z)/2;

    Vector3fT destination; // Destination vector, that is used to position every vertex of the bezier patch.
    Vector3fT top        (min.x+stepSizeX, min.y+stepSizeY, max.z);
    Vector3fT bottom     (min.x+stepSizeX, min.y+stepSizeY, min.z);

    for (int y=0; y<5; y++)
    {
        for (int x=0; x<9; x++)
        {
            if (y==0)
            {
                destination=top;
            }

            if (y==1 || y==2 || y==3)
            {
                destination.z=max.z-(y-1)*stepSizeZ;

                if (x == 2 || x == 6)
                    destination.x=min.x+stepSizeX;
                else if (x == 3 || x == 4 || x == 5)
                    destination.x=max.x;
                else
                    destination.x=min.x;

                if (x == 1 || x == 2 || x == 3)
                    destination.y=min.y;
                else if (x == 5 || x == 6 || x == 7)
                    destination.y=max.y;
                else
                    destination.y=min.y+stepSizeY;
            }

            if (y==4)
            {
                destination=bottom;
            }

            BP->SetCvPos(x, y, destination);
        }
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateConvexEndcap(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_, EndCapPosE pos)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(3, 3);

    Vector3fT source;              // Source point (where most of the vertices are moved to, to build an endcap).
    wxPoint   sourceVertices[6];   // Array of the control vertices that are moved to the sourcepoint.
    wxPoint   outlineVertices[3];  // Array of remaining control vertices, that build up the outline of the endcap.
    Vector3fT outlinePositions[3]; // Array of the positions for the outline vertices.

    // Initialize source points z-coordinate since its the same for all endcaps.
    source.z=max.z;

    // Initialize outline points z-coordinate since its the same for all endcaps and outline points.
    for (int i=0; i<3; i++)
    {
        outlinePositions[i].z=max.z;
    }

    // Choose source point and fill arrays, depending on endcap position parameter.
    switch (pos)
    {
        case TOP_RIGHT:
            source.x=min.x; source.y=min.y;

            sourceVertices[0]=wxPoint(0,2);
            sourceVertices[1]=wxPoint(1,2);
            sourceVertices[2]=wxPoint(2,2);
            sourceVertices[3]=wxPoint(0,1);
            sourceVertices[4]=wxPoint(1,1);
            sourceVertices[5]=wxPoint(2,1);

            outlineVertices[0]=wxPoint(0,0);
            outlineVertices[1]=wxPoint(1,0);
            outlineVertices[2]=wxPoint(2,0);

            outlinePositions[0].x=min.x; outlinePositions[0].y=max.y;
            outlinePositions[1].x=max.x; outlinePositions[1].y=max.y;
            outlinePositions[2].x=max.x; outlinePositions[2].y=min.y;

            break;

        case TOP_LEFT:
            source.x=max.x; source.y=min.y;

            sourceVertices[0]=wxPoint(1,0);
            sourceVertices[1]=wxPoint(1,1);
            sourceVertices[2]=wxPoint(1,2);
            sourceVertices[3]=wxPoint(2,0);
            sourceVertices[4]=wxPoint(2,1);
            sourceVertices[5]=wxPoint(2,2);

            outlineVertices[0]=wxPoint(0,0);
            outlineVertices[1]=wxPoint(0,1);
            outlineVertices[2]=wxPoint(0,2);

            outlinePositions[0].x=max.x; outlinePositions[0].y=max.y;
            outlinePositions[1].x=min.x; outlinePositions[1].y=max.y;
            outlinePositions[2].x=min.x; outlinePositions[2].y=min.y;

            break;

        case BOTTOM_RIGHT:
            source.x=min.x; source.y=max.y;

            sourceVertices[0]=wxPoint(0,0);
            sourceVertices[1]=wxPoint(0,1);
            sourceVertices[2]=wxPoint(0,2);
            sourceVertices[3]=wxPoint(1,0);
            sourceVertices[4]=wxPoint(1,1);
            sourceVertices[5]=wxPoint(1,2);

            outlineVertices[0]=wxPoint(2,0);
            outlineVertices[1]=wxPoint(2,1);
            outlineVertices[2]=wxPoint(2,2);

            outlinePositions[0].x=max.x; outlinePositions[0].y=max.y;
            outlinePositions[1].x=max.x; outlinePositions[1].y=min.y;
            outlinePositions[2].x=min.x; outlinePositions[2].y=min.y;

            break;

        default:    // Also handles case BOTTOM_LEFT.
            source.x=max.x; source.y=max.y;

            sourceVertices[0]=wxPoint(0,0);
            sourceVertices[1]=wxPoint(1,0);
            sourceVertices[2]=wxPoint(2,0);
            sourceVertices[3]=wxPoint(0,1);
            sourceVertices[4]=wxPoint(1,1);
            sourceVertices[5]=wxPoint(2,1);

            outlineVertices[0]=wxPoint(0,2);
            outlineVertices[1]=wxPoint(1,2);
            outlineVertices[2]=wxPoint(2,2);

            outlinePositions[0].x=min.x; outlinePositions[0].y=max.y;
            outlinePositions[1].x=min.x; outlinePositions[1].y=min.y;
            outlinePositions[2].x=max.x; outlinePositions[2].y=min.y;

            break;
    }


    // Set source control vertices to source position.
    for (int i=0; i<6; i++)
    {
        BP->SetCvPos(sourceVertices[i].x, sourceVertices[i].y, source);
    }

    // Set outline control vertices to their positions.
    for (int i=0; i<3; i++)
    {
        BP->SetCvPos(outlineVertices[i].x, outlineVertices[i].y, outlinePositions[i]);
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateConcaveEndcap(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_, EndCapPosE pos)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(3, 3);

    Vector3fT source;              // Source point (where most of the vertices are moved to, to build an endcap).
    wxPoint   sourceVertices[7];   // Array of the control vertices that are moved to the sourcepoint.
    wxPoint   outlineVertices[2];  // Array of remaining control vertices, that build up the outline of the endcap.
    Vector3fT outlinePositions[2]; // Array of the positions for the outline vertices.

    // Initialize source points z-coordinate since its the same for all endcaps.
    source.z=max.z;

    // Initialize outline points z-coordinate since its the same for all endcaps and outline points.
    for (int i=0; i<2; i++)
    {
        outlinePositions[i].z=max.z;
    }

    // Choose source point and fill arrays, depending on endcap position parameter.
    switch (pos)
    {
        case TOP_RIGHT:
            source.x=max.x; source.y=max.y;

            sourceVertices[0]=wxPoint(0,0);
            sourceVertices[1]=wxPoint(1,0);
            sourceVertices[2]=wxPoint(2,0);
            sourceVertices[3]=wxPoint(0,1);
            sourceVertices[4]=wxPoint(1,1);
            sourceVertices[5]=wxPoint(2,1);
            sourceVertices[6]=wxPoint(1,2);

            outlineVertices[0]=wxPoint(0,2);
            outlineVertices[1]=wxPoint(2,2);

            outlinePositions[0].x=min.x; outlinePositions[0].y=max.y;
            outlinePositions[1].x=max.x; outlinePositions[1].y=min.y;

            break;

        case TOP_LEFT:
            source.x=min.x; source.y=max.y;

            sourceVertices[0]=wxPoint(0,0);
            sourceVertices[1]=wxPoint(1,0);
            sourceVertices[2]=wxPoint(2,0);
            sourceVertices[3]=wxPoint(0,1);
            sourceVertices[4]=wxPoint(1,1);
            sourceVertices[5]=wxPoint(2,1);
            sourceVertices[6]=wxPoint(1,2);

            outlineVertices[0]=wxPoint(0,2);
            outlineVertices[1]=wxPoint(2,2);

            outlinePositions[0].x=min.x; outlinePositions[0].y=min.y;
            outlinePositions[1].x=max.x; outlinePositions[1].y=max.y;

            break;

        case BOTTOM_RIGHT:
            source.x=max.x; source.y=min.y;

            sourceVertices[0]=wxPoint(1,0);
            sourceVertices[1]=wxPoint(0,1);
            sourceVertices[2]=wxPoint(1,1);
            sourceVertices[3]=wxPoint(2,1);
            sourceVertices[4]=wxPoint(0,2);
            sourceVertices[5]=wxPoint(1,2);
            sourceVertices[6]=wxPoint(2,2);

            outlineVertices[0]=wxPoint(0,0);
            outlineVertices[1]=wxPoint(2,0);

            outlinePositions[0].x=min.x; outlinePositions[0].y=min.y;
            outlinePositions[1].x=max.x; outlinePositions[1].y=max.y;

            break;

        default:    // Also handles case BOTTOM_LEFT.
            source.x=min.x; source.y=min.y;

            sourceVertices[0]=wxPoint(1,0);
            sourceVertices[1]=wxPoint(0,1);
            sourceVertices[2]=wxPoint(1,1);
            sourceVertices[3]=wxPoint(2,1);
            sourceVertices[4]=wxPoint(0,2);
            sourceVertices[5]=wxPoint(1,2);
            sourceVertices[6]=wxPoint(2,2);

            outlineVertices[0]=wxPoint(0,0);
            outlineVertices[1]=wxPoint(2,0);

            outlinePositions[0].x=min.x; outlinePositions[0].y=max.y;
            outlinePositions[1].x=max.x; outlinePositions[1].y=min.y;

            break;
    }


    // Set source control vertices to source point.
    for (int i=0; i<7; i++)
    {
        BP->SetCvPos(sourceVertices[i].x, sourceVertices[i].y, source);
    }

    // Set outline control vertices to their positions.
    for (int i=0; i<2; i++)
    {
        BP->SetCvPos(outlineVertices[i].x, outlineVertices[i].y, outlinePositions[i]);
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::Clone() const
{
    return new MapBezierPatchT(*this);
}


void MapBezierPatchT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapPrimitiveT::Assign(Elem);

    const MapBezierPatchT* BP=dynamic_cast<const MapBezierPatchT*>(Elem);
    wxASSERT(BP!=NULL);
    if (BP==NULL) return;

    cv_Pos      =BP->cv_Pos;
    cv_UVs      =BP->cv_UVs;
    cv_Width    =BP->cv_Width;
    cv_Height   =BP->cv_Height;
    SubdivsHorz =BP->SubdivsHorz;
    SubdivsVert =BP->SubdivsVert;
    NeedsUpdate =true;
    SurfaceInfo =BP->SurfaceInfo;
 // BPRenderMesh=...;       // Properly dealt with in UpdateRenderMesh() below.
 // CollModel   =...;       // Properly dealt with in UpdateRenderMesh() below.
    Material    =BP->Material;

    UpdateRenderMesh();     // Could probably omit this here, deferring to the next implicit call.
}


BoundingBox3fT MapBezierPatchT::GetBB() const
{
    BoundingBox3fT BB;

    // TODO: Cache!
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            BB+=GetCvPos(x, y);

    return BB;
}


bool MapBezierPatchT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    UpdateRenderMesh();

    // If possible (RayOrigin is not in cull bounding-box), do a quick bounding-box check first.
    if (!GetBB().Contains(RayOrigin) && !MapPrimitiveT::TraceRay(RayOrigin, RayDir, Fraction, FaceNr)) return false;

    const double              RayLength=100000.0;
    cf::ClipSys::TraceResultT Result(1.0);

    wxASSERT(CollModel!=NULL);      // Made sure by UpdateRenderMesh().
    CollModel->TraceRay(RayOrigin.AsVectorOfDouble(), RayDir.AsVectorOfDouble()*RayLength, MaterialT::ClipFlagsT(0xFFFFFFFF), Result);

    if (Result.Fraction==1.0) return false;

    Fraction=float(Result.Fraction*RayLength);
    return true;
}


bool MapBezierPatchT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    const wxRect         Disc=wxRect(Pixel, Pixel).Inflate(Radius, Radius);
    const BoundingBox3fT BB  =GetBB();

    // 1. Do a quick preliminary bounding-box check:
    // Determine if this map elements BB intersects the Disc (which is actually rectangular...).
    if (!wxRect(ViewWin.WorldToWindow(BB.Min), ViewWin.WorldToWindow(BB.Max)).Intersects(Disc)) return false;

    // 2. Check the center X handle.
    if (Disc.Contains(ViewWin.WorldToWindow(BB.GetCenter()))) return true;

    // 3. If we select map elements by center handle only, then this brush was not hit.
    if (Options.view2d.SelectByHandles) return false;

    // 4. Check all edges of the patch mesh for a hit.
    unsigned long xstep=(GetRenderWidth ()-1)/(cv_Width -1);
    unsigned long ystep=(GetRenderHeight()-1)/(cv_Height-1);

    // Make sure we don't get into an infinite loop.
    // E.g. RenderInfo.width==2 and cv_Width==3 *is* possible!
    if (xstep==0) xstep=1;
    if (ystep==0) ystep=1;

    for (unsigned long y=0; y<GetRenderHeight(); y+=ystep)
    {
        for (unsigned long x=0; x+1<GetRenderWidth(); x++)
        {
            const wxPoint Vertex1=ViewWin.WorldToWindow(GetRenderVertexPos(x,   y));
            const wxPoint Vertex2=ViewWin.WorldToWindow(GetRenderVertexPos(x+1, y));

            if (ViewWindow2DT::RectIsIntersected(Disc, Vertex1, Vertex2))
                return true;
        }
    }

    for (unsigned long y=0; y+1<GetRenderHeight(); y++)
    {
        for (unsigned long x=0; x<GetRenderWidth(); x+=xstep)
        {
            const wxPoint Vertex1=ViewWin.WorldToWindow(GetRenderVertexPos(x, y));
            const wxPoint Vertex2=ViewWin.WorldToWindow(GetRenderVertexPos(x, y+1));

            if (ViewWindow2DT::RectIsIntersected(Disc, Vertex1, Vertex2))
                return true;
        }
    }

    // 5. Despite all efforts we found no hit.
    return false;
}


bool MapBezierPatchT::IsTranslucent() const
{
    if (Material==NULL) return false;

    return Material->IsTranslucent();
}


void MapBezierPatchT::Render2D(Renderer2DT& Renderer) const
{
    UpdateRenderMesh();

    const wxColour Color=IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors);
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, Color);
    Renderer.SetFillColor(Color);

    unsigned long xstep=(GetRenderWidth ()-1)/(cv_Width -1);
    unsigned long ystep=(GetRenderHeight()-1)/(cv_Height-1);

    // Make sure we don't get into an infinite loop.
    // E.g. RenderInfo.width==2 and cv_Width==3 *is* possible!
    if (xstep==0) xstep=1;
    if (ystep==0) ystep=1;

    for (unsigned long y=0; y<GetRenderHeight(); y+=ystep)
    {
        wxPoint Cur=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(0, y));

        for (unsigned long x=1; x<GetRenderWidth(); x++)
        {
            wxPoint Next=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(x, y));

            Renderer.DrawLine(Cur, Next);
            Cur=Next;
        }
    }

    for (unsigned long x=0; x<GetRenderWidth(); x+=xstep)
    {
        wxPoint Cur=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(x, 0));

        for (unsigned long y=1; y<GetRenderHeight(); y++)
        {
            wxPoint Next=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(x, y));

            Renderer.DrawLine(Cur, Next);
            Cur=Next;
        }
    }

    // Render the center X handle.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(150, 150, 150));
    Renderer.XHandle(Renderer.GetViewWin2D().WorldToTool(GetBB().GetCenter()));
}


void MapBezierPatchT::Render3D_Basic(MatSys::RenderMaterialT* RenderMat, const wxColour& MeshColor, const int MeshAlpha) const
{
    UpdateRenderMesh();

    BPRenderMesh->UpdateMeshColor(MeshColor.Red()/255.0, MeshColor.Green()/255.0, MeshColor.Blue()/255.0, MeshAlpha/255.0);

    // Backup previous rendermaterial and assign new one.
    MatSys::RenderMaterialT* tmp=BPRenderMesh->RenderMaterial;
    BPRenderMesh->RenderMaterial=RenderMat;

    // "Empty" vector is submitted, because this Vector isn't used in the method.
    BPRenderMesh->DrawAmbientContrib(Vector3dT());

    // Reload old previous render material.
    BPRenderMesh->RenderMaterial=tmp;
}


void MapBezierPatchT::Render3D(Renderer3DT& Renderer) const
{
    const ViewWindow3DT& ViewWin=Renderer.GetViewWin3D();

    switch (ViewWin.GetViewType())
    {
        case ViewWindowT::VT_3D_EDIT_MATS:
            // Note that the mesh color is ignored for most normal materials anyway... (they don't have the "useMeshColors" property).
            Render3D_Basic(Material!=NULL ? Material->GetRenderMaterial(false) : Renderer.GetRMatFlatShaded(), *wxWHITE, 255);
            break;

        case ViewWindowT::VT_3D_FULL_MATS:
            // Note that the mesh color is ignored for most normal materials anyway... (they don't have the "useMeshColors" property).
            Render3D_Basic(Material!=NULL ? Material->GetRenderMaterial(true) : Renderer.GetRMatFlatShaded(), *wxWHITE, 255);
            break;

        case ViewWindowT::VT_3D_LM_GRID:
        case ViewWindowT::VT_3D_LM_PREVIEW:
            // The concept of lightmaps does not really apply to bezier patches...
            Render3D_Basic(Renderer.GetRMatFlatShaded(), IsSelected() ? Options.colors.SelectedFace : *wxWHITE, 255);
            break;

        case ViewWindowT::VT_3D_FLAT:
            Render3D_Basic(Renderer.GetRMatFlatShaded(), IsSelected() ? Options.colors.SelectedFace : m_Color, 255);
            break;

        case ViewWindowT::VT_3D_WIREFRAME:
            Render3D_Basic(Renderer.GetRMatWireframe(), IsSelected() ? Options.colors.SelectedEdge : m_Color, 255);
            break;

        default:
            wxASSERT(0);
            break;
    }

    // If every MapElementT had it's pointer to the map document struct like this one wouldn't be necessary. For now it's the
    // only possibility to get to the surface dialog that holds information whether selected bezier patches should be rendered
    // with a selection mask or the toolmanager to check whether the surface editing tool is active.
    if (IsSelected())
        if (   ViewWin.GetMapDoc().GetChildFrame()->GetToolManager().GetActiveToolType()!=&ToolEditSurfaceT::TypeInfo
            || ViewWin.GetMapDoc().GetChildFrame()->GetSurfacePropsDialog()->WantSelectionOverlay())
            if ((ViewWin.GetViewType()==ViewWindowT::VT_3D_EDIT_MATS || ViewWin.GetViewType()==ViewWindowT::VT_3D_FULL_MATS))
            {
                Render3D_Basic(Renderer.GetRMatOverlay(), Options.colors.SelectedFace, 64);
                Render3D_Basic(Renderer.GetRMatWireframe_OffsetZ(), wxColour(255, 255, 0), 255);
            }
}


void MapBezierPatchT::SetMaterial(EditorMaterialI* Material_)
{
    Material = Material_;
}


void MapBezierPatchT::SetSurfaceInfo(const SurfaceInfoT& SI)
{
    SurfaceInfo=SI;
    UpdateTextureSpace();
}


void MapBezierPatchT::InvertPatch()
{
    // Loop over all rows of the matrix and invert each row.
    for (unsigned long y=0; y<cv_Height; y++)
    {
        for (unsigned long x=0; x<cv_Width/2; x++)
        {
            // Switch CVs.
            const Vector3fT cv_pos_tmp=GetCvPos(x, y);
            SetCvPos(x, y, GetCvPos(cv_Width-1-x, y));
            SetCvPos(cv_Width-1-x, y, cv_pos_tmp);
        }
    }
}


void MapBezierPatchT::UpdateTextureSpace()
{
    if (SurfaceInfo.TexCoordGenMode==PlaneProj) // Project material onto patch using UV axis.
    {
        for (unsigned long y=0; y<GetHeight(); y++)
        {
            for (unsigned long x=0; x<GetWidth(); x++)
            {
                const Vector3fT& cv_xyz=GetCvPos(x, y);
                Vector3fT cv_uvw;

                cv_uvw.x=dot(SurfaceInfo.UAxis, cv_xyz)*SurfaceInfo.Scale[0]+SurfaceInfo.Trans[0];
                cv_uvw.y=dot(SurfaceInfo.VAxis, cv_xyz)*SurfaceInfo.Scale[1]+SurfaceInfo.Trans[1];
                cv_uvw.z=0.0f;

                SetCvUV(x, y, cv_uvw);
            }
        }
    }

    if (SurfaceInfo.TexCoordGenMode==MatFit) // Calculate texture coordinates using BP specific MaterialFit mode.
    {
        if (cv_Width < 3 || cv_Height < 3)
            return;

        const float xs = 1.0f/(cv_Width-1);
        const float ys = 1.0f/(cv_Height-1);

        for (unsigned long y=0; y<cv_Height; y++)
        {
            for (unsigned int x=0; x<cv_Width; x++)
            {
                Vector3fT cv_uvw;

                cv_uvw.x=(x*xs)*SurfaceInfo.Scale[0]+SurfaceInfo.Trans[0];
                cv_uvw.y=(y*ys)*SurfaceInfo.Scale[1]+SurfaceInfo.Trans[1];
                cv_uvw.z=0.0f;

                cv_uvw=cv_uvw.GetRotZ(SurfaceInfo.Rotate);

                SetCvUV(x, y, cv_uvw);
            }
        }

        // Set projection plane vectors to zero, because they don't exist for this kind of "projection".
        SurfaceInfo.UAxis=Vector3fT();
        SurfaceInfo.VAxis=Vector3fT();
    }
}


void MapBezierPatchT::TrafoMove(const Vector3fT& Delta)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            SetCvPos(x, y, GetCvPos(x, y)+Delta);

    MapPrimitiveT::TrafoMove(Delta);
}


void MapBezierPatchT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
        {
            Vector3fT Vertex=GetCvPos(x, y)-RefPoint;

            if (Angles.x!=0.0f) Vertex=Vertex.GetRotX( Angles.x);
            if (Angles.y!=0.0f) Vertex=Vertex.GetRotY(-Angles.y);
            if (Angles.z!=0.0f) Vertex=Vertex.GetRotZ( Angles.z);

            SetCvPos(x, y, Vertex+RefPoint);
        }

    MapPrimitiveT::TrafoRotate(RefPoint, Angles);
}


void MapBezierPatchT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            SetCvPos(x, y, RefPoint + (GetCvPos(x, y)-RefPoint).GetScaled(Scale));

    MapPrimitiveT::TrafoScale(RefPoint, Scale);
}


void MapBezierPatchT::TrafoMirror(unsigned int NormalAxis, float Dist)
{
}


void MapBezierPatchT::Transform(const MatrixT& Matrix)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            SetCvPos(x, y, Matrix.Mul1(GetCvPos(x, y)));

    MapPrimitiveT::Transform(Matrix);
}


// This method is const, so that it can be called from other const methods.
void MapBezierPatchT::UpdateRenderMesh() const
{
    const float MaxCurveError=400.0f/cf::CA3DE_SCALE;

    // If either control vertex coordinates or UV coordinates have changed, the mesh needs to be updated.
    if (NeedsUpdate)
    {
        // 1. Create a new render mesh.
        assert(BPRenderMesh!=NULL);
        delete BPRenderMesh;
        BPRenderMesh=new cf::SceneGraph::BezierPatchNodeT(LMM, cv_Width, cv_Height, cv_Pos, cv_UVs, SubdivsHorz, SubdivsVert, Material->GetMaterial(), MaxCurveError);


        // 2. Create a new collision model.
        cf::math::BezierPatchT<float> CollisionBP(cv_Width, cv_Height, cv_Pos);

        if (SubdivsHorz>0 && SubdivsVert>0)
        {
            // The mapper may have provided an explicit number of subdivisions in order to avoid gaps between adjacent bezier patches.
            // The casts to unsigned long are needed in order to resolve ambiguity of the overloaded Subdivide() method.
            CollisionBP.Subdivide((unsigned long)SubdivsHorz, (unsigned long)SubdivsVert);
        }
        else
        {
            CollisionBP.Subdivide(MaxCurveError, -1.0f);
        }

        // Get a collision model from the curve mesh.
        ArrayT<Vector3dT> CoordsOnly;

        for (unsigned long VertexNr=0; VertexNr<CollisionBP.Mesh.Size(); VertexNr++)
            CoordsOnly.PushBack(CollisionBP.Mesh[VertexNr].Coord.AsVectorOfDouble());

        // We cannot use our own Material->GetMaterial() here, because it might clip nothing (ClipFlags==0)
        // and thus would wrongly not be considered in TraceRay().
        static MaterialT s_CollMaterial;
        s_CollMaterial.ClipFlags=MaterialT::Clip_AllBlocking;

        delete CollModel;
        CollModel=new cf::ClipSys::CollisionModelStaticT(CollisionBP.Width, CollisionBP.Height, CoordsOnly, &s_CollMaterial);


        // 3. Everything updated for now.
        NeedsUpdate=false;
    }
}


void MapBezierPatchT::SetSize(unsigned long width, unsigned long height)
{
    if (cv_Width==width && cv_Height==height) return;

    cv_Width =width;
    cv_Height=height;

    const unsigned long Total=cv_Width*cv_Height;

    cv_Pos.Clear(); cv_Pos.PushBackEmpty(Total);
    cv_UVs.Clear(); cv_UVs.PushBackEmpty(Total);

    if (Total>0)
    {
        // I don't think that we really need this, becaue a call to this method should *always* be followed
        // by calls to SetCvPos() and SetCvUV(), but who ever knows...
        SetCvPos(0, 0, Vector3fT());
        SetCvUV (0, 0, Vector3fT());
    }
}


Vector3fT MapBezierPatchT::GetRenderVertexPos(unsigned long x, unsigned long y) const
{
    assert(BPRenderMesh->Meshes.Size()>0);
    assert(x<=BPRenderMesh->Meshes.Size());
    assert(y<BPRenderMesh->Meshes[0]->Vertices.Size()/2);

    unsigned long c=0;

    // Special case when accessing the render meshes last line of vertices
    if (x==BPRenderMesh->Meshes.Size()) c=1;

    Vector3fT VertexPos;

    VertexPos.x=BPRenderMesh->Meshes[x-c]->Vertices[2*y+c].Origin[0];
    VertexPos.y=BPRenderMesh->Meshes[x-c]->Vertices[2*y+c].Origin[1];
    VertexPos.z=BPRenderMesh->Meshes[x-c]->Vertices[2*y+c].Origin[2];

    return VertexPos;
}
