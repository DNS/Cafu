/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Renderer3D.hpp"
#include "ChildFrame.hpp"        // ONLY needed for   m_MapDoc.GetChildFrame()->GetToolManager()
#include "ChildFrameViewWin3D.hpp"
#include "MapDocument.hpp"
#include "MapPrimitive.hpp"
#include "Tool.hpp"
#include "ToolManager.hpp"

#include "../Camera.hpp"
#include "../EditorMaterialManager.hpp"

#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"

#include "wx/wx.h"

#include <algorithm>


/// The global counter that indicates the number of the 3D video frame that is currently rendered.
/// Copied into the map elements, it is used in order to avoid processing/rendering map elements twice.
/// TODO: Move and turn into a member of the document or ChildFrameT? Can have multiple open documents, each with multiple 3D views...
static unsigned int Renderer3D_FrameCount=0;


Renderer3DT::UseOrthoMatricesT::UseOrthoMatricesT(const wxWindow& Window)
{
    const wxSize CanvasSize=Window.GetClientSize();

    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );

    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, CanvasSize.GetWidth(), CanvasSize.GetHeight(), 0.0f, -1.0f, 1.0f));
}


Renderer3DT::UseOrthoMatricesT::~UseOrthoMatricesT()
{
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
}


Renderer3DT::Renderer3DT(ViewWindow3DT& ViewWin3D)
    : m_ViewWin3D(ViewWin3D),
      m_ActiveToolCache(NULL),
      m_VisElemsBackToFront()
{
    // These could equally well be static members of this class,
    // and be initialized and shutdown by the AppCaWE class that also deals with the MatSys itself.
    // However, it somehow feels more natural and safer to have them here,
    // and different Renderer3DTs could have different materials etc.
    m_RMatWireframe        =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/Wireframe"        ));
    m_RMatWireframeOZ      =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/WireframeOffsetZ" ));
    m_RMatFlatShaded       =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/FlatShaded"       ));
    m_RMatFlatShadedOZ     =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/FlatShadedOffsetZ"));
    m_RMatOverlay          =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/Overlay"          ));
    m_RMatOverlayOZ        =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/OverlayOffsetZ"   ));
    m_RMatTerrainEdit      =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/TerrainEditTool3D"));
    m_RMatTerrainEyeDropper=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/TerrainEyeDropper"));
}


Renderer3DT::~Renderer3DT()
{
    MatSys::Renderer->FreeMaterial(m_RMatWireframe        );
    MatSys::Renderer->FreeMaterial(m_RMatWireframeOZ      );
    MatSys::Renderer->FreeMaterial(m_RMatFlatShaded       );
    MatSys::Renderer->FreeMaterial(m_RMatFlatShadedOZ     );
    MatSys::Renderer->FreeMaterial(m_RMatOverlay          );
    MatSys::Renderer->FreeMaterial(m_RMatOverlayOZ        );
    MatSys::Renderer->FreeMaterial(m_RMatTerrainEdit      );
    MatSys::Renderer->FreeMaterial(m_RMatTerrainEyeDropper);
}


void Renderer3DT::InitFrame()
{
    Renderer3D_FrameCount++;

    m_ActiveToolCache=m_ViewWin3D.GetChildFrame()->GetToolManager().GetActiveTool();

    // Locally cache the view frustum planes: They're needed for culling the BSP tree nodes and
    // their map elements below, and even by some map elements for LoD computations (e.g. the terrains).
    m_ViewWin3D.GetViewFrustum(m_FrustumPlanesCache);

    // Determine the back-to-front ordered list of map elements that are in the view frustum and visible.
    m_VisElemsBackToFront.Overwrite();

    // COMPL_OUTSIDE doesn't make sense, COMPL_INSIDE would turn off view frustum culling.
    GetRenderList(m_ViewWin3D.GetMapDoc().GetBspTree()->GetRootNode(), INTERSECTS);

    // Set the active tool cache back to NULL.
    m_ActiveToolCache=NULL;
}


float Renderer3DT::GetConstShade(const Vector3T<float>& Normal) const
{
    const static Vector3fT Light =Vector3fT(0.2f, 0.4f, 0.7f);
    const static float     LightL=length(Light);

    return std::min(dot(Normal, Light/LightL)*0.4f + 0.6f, 1.0f);
}


void Renderer3DT::RenderBox(const BoundingBox3fT& BB, const wxColour& Color, bool Solid) const
{
    Vector3fT Vertices[8];

    BB.GetCornerVertices(Vertices);
    RenderBox(Vertices, Color, Solid);
}


void Renderer3DT::RenderBox(const Vector3fT Vertices[], const wxColour& Color, bool Solid) const
{
    const float r=Color.Red()  /255.0f;
    const float g=Color.Green()/255.0f;
    const float b=Color.Blue() /255.0f;

    if (Solid)
    {
        static const unsigned long Faces[6][4]=
        {
            { 0, 2, 3, 1 },
            { 0, 1, 5, 4 },
            { 4, 5, 7, 6 },
            { 2, 6, 7, 3 },
            { 1, 3, 7, 5 },
            { 0, 4, 6, 2 }
        };

        MatSys::MeshT Mesh(MatSys::MeshT::TriangleFan);
        Mesh.Vertices.PushBackEmpty(4);

        for (unsigned long FaceNr=0; FaceNr<6; FaceNr++)
        {
            const Vector3fT& V1=Vertices[Faces[FaceNr][0]];
            const Vector3fT& V2=Vertices[Faces[FaceNr][1]];
            const Vector3fT& V3=Vertices[Faces[FaceNr][2]];
            const Vector3fT& V4=Vertices[Faces[FaceNr][3]];

            const Vector3fT Edge1 =V4-V1;
            const Vector3fT Edge2 =V2-V1;
            const Vector3fT Normal=normalizeOr0(cross(Edge1, Edge2));
            const float     Shade =GetConstShade(Normal);
            const float     rSh   =r*Shade;
            const float     gSh   =g*Shade;
            const float     bSh   =b*Shade;

            Mesh.Vertices[0].SetOrigin(V1); Mesh.Vertices[0].SetColor(rSh, gSh, bSh);
            Mesh.Vertices[1].SetOrigin(V2); Mesh.Vertices[1].SetColor(rSh, gSh, bSh);
            Mesh.Vertices[2].SetOrigin(V3); Mesh.Vertices[2].SetColor(rSh, gSh, bSh);
            Mesh.Vertices[3].SetOrigin(V4); Mesh.Vertices[3].SetColor(rSh, gSh, bSh);

            MatSys::Renderer->SetCurrentMaterial(GetRMatFlatShaded());
            MatSys::Renderer->RenderMesh(Mesh);
        }
    }
    else
    {
        // Draw the edges of the bottom face, one upleading edge, and the edges of the top face.
        MatSys::MeshT Mesh1(MatSys::MeshT::LineStrip);
        Mesh1.Vertices.PushBackEmpty(10);

        Mesh1.Vertices[0].SetOrigin(Vertices[0]); Mesh1.Vertices[0].SetColor(r, g, b);
        Mesh1.Vertices[1].SetOrigin(Vertices[4]); Mesh1.Vertices[1].SetColor(r, g, b);
        Mesh1.Vertices[2].SetOrigin(Vertices[5]); Mesh1.Vertices[2].SetColor(r, g, b);
        Mesh1.Vertices[3].SetOrigin(Vertices[1]); Mesh1.Vertices[3].SetColor(r, g, b);
        Mesh1.Vertices[4].SetOrigin(Vertices[0]); Mesh1.Vertices[4].SetColor(r, g, b);
        Mesh1.Vertices[5].SetOrigin(Vertices[2]); Mesh1.Vertices[5].SetColor(r, g, b);
        Mesh1.Vertices[6].SetOrigin(Vertices[6]); Mesh1.Vertices[6].SetColor(r, g, b);
        Mesh1.Vertices[7].SetOrigin(Vertices[7]); Mesh1.Vertices[7].SetColor(r, g, b);
        Mesh1.Vertices[8].SetOrigin(Vertices[3]); Mesh1.Vertices[8].SetColor(r, g, b);
        Mesh1.Vertices[9].SetOrigin(Vertices[2]); Mesh1.Vertices[9].SetColor(r, g, b);

        MatSys::Renderer->SetCurrentMaterial(GetRMatWireframe());
        MatSys::Renderer->RenderMesh(Mesh1);

        // Draw the three remaining edges between the top and the bottom face.
        MatSys::MeshT Mesh2(MatSys::MeshT::Lines);
        Mesh2.Vertices.PushBackEmpty(6);

        Mesh2.Vertices[0].SetOrigin(Vertices[4]); Mesh2.Vertices[0].SetColor(r, g, b);
        Mesh2.Vertices[1].SetOrigin(Vertices[6]); Mesh2.Vertices[1].SetColor(r, g, b);
        Mesh2.Vertices[2].SetOrigin(Vertices[5]); Mesh2.Vertices[2].SetColor(r, g, b);
        Mesh2.Vertices[3].SetOrigin(Vertices[7]); Mesh2.Vertices[3].SetColor(r, g, b);
        Mesh2.Vertices[4].SetOrigin(Vertices[1]); Mesh2.Vertices[4].SetColor(r, g, b);
        Mesh2.Vertices[5].SetOrigin(Vertices[3]); Mesh2.Vertices[5].SetColor(r, g, b);

        MatSys::Renderer->SetCurrentMaterial(GetRMatWireframe());
        MatSys::Renderer->RenderMesh(Mesh2);
    }
}


void Renderer3DT::RenderLine(const Vector3fT& A, const Vector3fT& B, const wxColour& Color) const
{
    const float r=Color.Red()  /255.0f;
    const float g=Color.Green()/255.0f;
    const float b=Color.Blue() /255.0f;

    MatSys::MeshT Mesh(MatSys::MeshT::Lines);
    Mesh.Vertices.PushBackEmpty(2);

    Mesh.Vertices[0].SetOrigin(A); Mesh.Vertices[0].SetColor(r, g, b);
    Mesh.Vertices[1].SetOrigin(B); Mesh.Vertices[1].SetColor(r, g, b);

    MatSys::Renderer->SetCurrentMaterial(GetRMatWireframe());
    MatSys::Renderer->RenderMesh(Mesh);
}


void Renderer3DT::RenderSplitPlanes(const OrthoBspTreeT::NodeT* Node, int Depth) const
{
    if (Depth<=0) return;
    if (Node->GetPlaneType()==OrthoBspTreeT::NodeT::NONE) return;

    const unsigned int PlaneType=Node->GetPlaneType();
    const unsigned int NearChild=m_ViewWin3D.GetCamera().Pos[PlaneType] > Node->GetPlaneDist() ? 0 : 1;
    const unsigned int FarChild =1-NearChild;

    // 1. Render the far child.
    RenderSplitPlanes(Node->GetChild(FarChild), Depth-1);

    // 2. Render the split plane.
    // TODO: Sorry, the ClipBB should really not be hard-coded...
    const BoundingBox3fT  ClipBB(Vector3fT(-2*1024, -2*1024, -3*1024), Vector3fT(5*1024, 2*1024, 2*1024));
    const BoundingBox3fT& NodeBB=Node->GetBB();
    Vector3fT             Points[4]={ NodeBB.Min, NodeBB.Min, NodeBB.Max, NodeBB.Max };

    for (unsigned int PointNr=0; PointNr<4; PointNr++)
    {
        Points[PointNr][PlaneType]=Node->GetPlaneDist();

        const unsigned int a=(PlaneType+1) % 3;
        if (Points[PointNr][a] < ClipBB.Min[a]) Points[PointNr][a]=ClipBB.Min[a];
        if (Points[PointNr][a] > ClipBB.Max[a]) Points[PointNr][a]=ClipBB.Max[a];

        const unsigned int b=(PlaneType+2) % 3;
        if (Points[PointNr][b] < ClipBB.Min[b]) Points[PointNr][b]=ClipBB.Min[b];
        if (Points[PointNr][b] > ClipBB.Max[b]) Points[PointNr][b]=ClipBB.Max[b];
    }

    // Note that without the "+FarChild" here, the resulting Points array would be correct,
    // but the order of the points would be oblivious of the camera position.
    // Adding FarChild happens to yield an order in which the Points are always front-facing, and thus the
    // rendered polygon is, as desired, never backside-culled even though it uses a single-sided material.
    std::swap(Points[1][(PlaneType+1+FarChild) % 3], Points[3][(PlaneType+1+FarChild) % 3]);

    for (int Pass=1; Pass<=2; Pass++)
    {
        // Note that in the wire-frame pass, we must use a MatSys::MeshT::Polygon rather than a MatSys::MeshT::LineLoop
        // mesh, or else the depth offset won't work right (they are probably differently rasterized).
        MatSys::MeshT Mesh(Pass==1 ? MatSys::MeshT::Polygon : MatSys::MeshT::TriangleFan);
        Mesh.Vertices.PushBackEmpty(4);

        float color[3]={ 0.0f, 0.0f, 0.0f };
        color[Depth % 3]=(Pass==1) ? 1.0f : 0.4f;

        for (unsigned long VertexNr=0; VertexNr<4; VertexNr++)
        {
            Mesh.Vertices[VertexNr].SetOrigin(Points[VertexNr]);
            Mesh.Vertices[VertexNr].SetColor(color[0], color[1], color[2], Pass==1 ? 1.0f : 0.4f);
        }

        MatSys::Renderer->SetCurrentMaterial(Pass==1 ? GetRMatWireframe_OffsetZ() : GetRMatFlatShaded());
        MatSys::Renderer->RenderMesh(Mesh);
    }

    // 3. Render the near child.
    RenderSplitPlanes(Node->GetChild(NearChild), Depth-1);
}


void Renderer3DT::BasisVectors(const Vector3fT& Pos, const cf::math::Matrix3x3fT& Mat, float Length) const
{
    static MatSys::MeshT Mesh(MatSys::MeshT::Lines);

    if (Mesh.Vertices.Size()==0) Mesh.Vertices.PushBackEmpty(6);

    Mesh.Vertices[0].SetColor(1, 0, 0);
    Mesh.Vertices[0].SetOrigin(Pos);

    Mesh.Vertices[1].SetColor(1, 0, 0);
    Mesh.Vertices[1].SetOrigin(Pos + Vector3fT(Mat[0][0], Mat[1][0], Mat[2][0])*Length);

    Mesh.Vertices[2].SetColor(0, 1, 0);
    Mesh.Vertices[2].SetOrigin(Pos);

    Mesh.Vertices[3].SetColor(0, 1, 0);
    Mesh.Vertices[3].SetOrigin(Pos + Vector3fT(Mat[0][1], Mat[1][1], Mat[2][1])*Length);

    Mesh.Vertices[4].SetColor(0, 0, 1);
    Mesh.Vertices[4].SetOrigin(Pos);

    Mesh.Vertices[5].SetColor(0, 0, 1);
    Mesh.Vertices[5].SetOrigin(Pos + Vector3fT(Mat[0][2], Mat[1][2], Mat[2][2])*Length);

    MatSys::Renderer->SetCurrentMaterial(GetRMatWireframe());
    MatSys::Renderer->RenderMesh(Mesh);
}


void Renderer3DT::RenderCrossHair(const wxPoint& Center) const
{
    const int RADIUS=5;
    static MatSys::MeshT Mesh(MatSys::MeshT::Lines);

    if (Mesh.Vertices.Size()==0) Mesh.Vertices.PushBackEmpty(12);

    // Black background crosshair (rendered first).
    Mesh.Vertices[ 0].SetOrigin(Center.x-RADIUS-1, Center.y-1, 0); Mesh.Vertices[ 0].SetColor(0, 0, 0);
    Mesh.Vertices[ 1].SetOrigin(Center.x+RADIUS,   Center.y-1, 0); Mesh.Vertices[ 1].SetColor(0, 0, 0);

    Mesh.Vertices[ 2].SetOrigin(Center.x-RADIUS-1, Center.y+1, 0); Mesh.Vertices[ 2].SetColor(0, 0, 0);
    Mesh.Vertices[ 3].SetOrigin(Center.x+RADIUS,   Center.y+1, 0); Mesh.Vertices[ 3].SetColor(0, 0, 0);

    Mesh.Vertices[ 4].SetOrigin(Center.x-1, Center.y-RADIUS,   0); Mesh.Vertices[ 4].SetColor(0, 0, 0);
    Mesh.Vertices[ 5].SetOrigin(Center.x-1, Center.y+RADIUS+1, 0); Mesh.Vertices[ 5].SetColor(0, 0, 0);

    Mesh.Vertices[ 6].SetOrigin(Center.x+1, Center.y-RADIUS,   0); Mesh.Vertices[ 6].SetColor(0, 0, 0);
    Mesh.Vertices[ 7].SetOrigin(Center.x+1, Center.y+RADIUS+1, 0); Mesh.Vertices[ 7].SetColor(0, 0, 0);

    // White foreground crosshair (rendered second, on top of black background).
    Mesh.Vertices[ 8].SetOrigin(Center.x-RADIUS-1, Center.y, 0); Mesh.Vertices[ 8].SetColor(1, 1, 1);
    Mesh.Vertices[ 9].SetOrigin(Center.x+RADIUS,   Center.y, 0); Mesh.Vertices[ 9].SetColor(1, 1, 1);

    Mesh.Vertices[10].SetOrigin(Center.x, Center.y-RADIUS,   0); Mesh.Vertices[10].SetColor(1, 1, 1);
    Mesh.Vertices[11].SetOrigin(Center.x, Center.y+RADIUS+1, 0); Mesh.Vertices[11].SetColor(1, 1, 1);

    MatSys::Renderer->SetCurrentMaterial(GetRMatWireframe());
    MatSys::Renderer->RenderMesh(Mesh);
}


/// Determines where the given bounding-box BB is located in relation to the view frustum:
/// Completely inside, completely outside, or intersecting one of the view frustum planes.
Renderer3DT::RelLocT Renderer3DT::RelFrustum(const BoundingBox3fT& BB) const
{
    for (unsigned int PlaneNr=0; PlaneNr<6; PlaneNr++)
    {
        const BoundingBox3fT::SideT Side=BB.WhatSide(m_FrustumPlanesCache[PlaneNr]);

        if (Side==BoundingBox3fT::Front) return COMPL_OUTSIDE;
        if (Side==BoundingBox3fT::Both ) return INTERSECTS;
    }

    return COMPL_INSIDE;
}


void Renderer3DT::GetRenderList(const OrthoBspTreeT::NodeT* Node, RelLocT ParentLoc)
{
    const RelLocT NodeLoc=(ParentLoc==COMPL_INSIDE) ? COMPL_INSIDE : RelFrustum(Node->GetBB());

    if (NodeLoc==COMPL_OUTSIDE) return;

    // 1. Render the far child.
    if (Node->GetPlaneType()!=OrthoBspTreeT::NodeT::NONE)
    {
        const unsigned int NearChild=m_ViewWin3D.GetCamera().Pos[Node->GetPlaneType()] > Node->GetPlaneDist() ? 0 : 1;
        const unsigned int FarChild =1-NearChild;

        GetRenderList(Node->GetChild(FarChild), NodeLoc);
    }

    // 2. Render the map elements in the node.
    for (unsigned long ElemNr=0; ElemNr<Node->GetElems().Size(); ElemNr++)
    {
        MapElementT* Elem = Node->GetElems()[ElemNr];

        if (Elem->GetFrameCount()==Renderer3D_FrameCount) continue;     // Already rendered in this frame?
        Elem->SetFrameCount(Renderer3D_FrameCount);                     // Flag as processed (rendered or found invisible) in this frame.

        if (!Elem->IsVisible()) continue;                                                 // Hidden by group?
        if (m_ActiveToolCache->IsHiddenByTool(Elem)) continue;                            // Hidden by tool?
        if (NodeLoc!=COMPL_INSIDE && RelFrustum(Elem->GetBB())==COMPL_OUTSIDE) continue;  // Outside view frustum?

        m_VisElemsBackToFront.PushBack(Elem);
    }

    // 3. Render the near child.
    if (Node->GetPlaneType()!=OrthoBspTreeT::NodeT::NONE)
    {
        const unsigned int NearChild=m_ViewWin3D.GetCamera().Pos[Node->GetPlaneType()] > Node->GetPlaneDist() ? 0 : 1;

        GetRenderList(Node->GetChild(NearChild), NodeLoc);
    }
}
