/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MorphPrim.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "MapBezierPatch.hpp"
#include "MapBrush.hpp"
#include "../Options.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "MaterialSystem/Renderer.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Polygon.hpp"


MP_EdgeT::MP_EdgeT()
{
    Vertices[0]=NULL;
    Vertices[1]=NULL;

    Faces[0]=NULL;
    Faces[1]=NULL;
}


MorphPrimT::MorphPrimT(const MapPrimitiveT* MapPrim)
    : m_MapPrim(MapPrim),
      m_Modified(false),
      m_RenderBP(NULL)
{
    const MapBrushT*       MapBrush=dynamic_cast<const MapBrushT*>      (m_MapPrim);
    const MapBezierPatchT* MapPatch=dynamic_cast<const MapBezierPatchT*>(m_MapPrim);

    wxASSERT(MapBrush!=NULL || MapPatch!=NULL);     // Assert they are not both NULL.
    wxASSERT(MapBrush==NULL || MapPatch==NULL);     // Assert they are not both non-NULL.

    if (MapBrush!=NULL)
    {
        // Blindly toss all vertices of all faces of MapBrush into the m_Vertices array,
        // the UpdateBrushFromVertices() will sort all duplicates out.
        for (unsigned long FaceNr=0; FaceNr<MapBrush->GetFaces().Size(); FaceNr++)
        {
            const MapFaceT& MapFace=MapBrush->GetFaces()[FaceNr];

            for (unsigned long VertexNr=0; VertexNr<MapFace.GetVertices().Size(); VertexNr++)
            {
                m_Vertices.PushBack(new MP_VertexT);
                m_Vertices[m_Vertices.Size()-1]->pos=MapFace.GetVertices()[VertexNr];
            }
        }

        UpdateBrushFromVertices();
    }

    if (MapPatch!=NULL)
    {
        for (unsigned long y=0; y<MapPatch->GetHeight(); y++)
        {
            for (unsigned long x=0; x<MapPatch->GetWidth(); x++)
            {
                m_Vertices.PushBack(new MP_VertexT);
                m_Vertices[m_Vertices.Size()-1]->pos=MapPatch->GetCvPos(x, y);
            }
        }

        m_RenderBP=MapPatch->Clone();
    }
}


MorphPrimT::~MorphPrimT()
{
    delete m_RenderBP;
    m_RenderBP=NULL;

    for (unsigned long VertexNr=0; VertexNr<m_Vertices.Size(); VertexNr++) delete m_Vertices[VertexNr];
    m_Vertices.Clear();

    for (unsigned long EdgeNr=0; EdgeNr<m_Edges.Size(); EdgeNr++) delete m_Edges[EdgeNr];
    m_Edges.Clear();

    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++) delete m_Faces[FaceNr];
    m_Faces.Clear();
}


MapPrimitiveT* MorphPrimT::GetMorphedMapPrim() const
{
    const MapBrushT*       MapBrush=dynamic_cast<const MapBrushT*>      (m_MapPrim);
    const MapBezierPatchT* MapPatch=dynamic_cast<const MapBezierPatchT*>(m_MapPrim);

    wxASSERT(MapBrush!=NULL || MapPatch!=NULL);     // Assert they are not both NULL.
    wxASSERT(MapBrush==NULL || MapPatch==NULL);     // Assert they are not both non-NULL.

    if (MapBrush!=NULL)
    {
        // Build the convex hull over the m_Vertices.
        ArrayT<Vector3fT> HullVertices;
        for (unsigned long VNr=0; VNr<m_Vertices.Size(); VNr++)
            HullVertices.PushBack(m_Vertices[VNr]->pos);

        MapBrushT* MorphedBrush=new MapBrushT(HullVertices, MapBrush->GetFaces()[0].GetMaterial(), Options.general.NewUVsFaceAligned, MapBrush);

        if (MorphedBrush->IsValid()) return MorphedBrush;

        delete MorphedBrush;
        return NULL;
    }

    if (MapPatch!=NULL && m_RenderBP!=NULL)
    {
        return m_RenderBP->Clone();
    }

    return NULL;
}


MP_EdgeT* MorphPrimT::FindEdge(const MP_VertexT* v1, const MP_VertexT* v2) const
{
    for (unsigned long EdgeNr=0; EdgeNr<m_Edges.Size(); EdgeNr++)
    {
        MP_EdgeT* Edge=m_Edges[EdgeNr];

        if ((Edge->Vertices[0]==v1 && Edge->Vertices[1]==v2) ||
            (Edge->Vertices[0]==v2 && Edge->Vertices[1]==v1))
        {
            return Edge;
        }
    }

    return NULL;
}


MP_VertexT* MorphPrimT::FindClosestVertex(const Vector3fT& Point) const
{
    if (m_Vertices.Size()==0) return NULL;

    float BestDist=(m_Vertices[0]->pos-Point).GetLengthSqr();
    int   BestIdx =0;

    for (unsigned long VertexNr=1; VertexNr<m_Vertices.Size(); VertexNr++)
    {
        const float Dist=(m_Vertices[VertexNr]->pos-Point).GetLengthSqr();

        if (Dist<BestDist)
        {
            BestDist=Dist;
            BestIdx =VertexNr;
        }
    }

    return m_Vertices[BestIdx];
}


void MorphPrimT::Render(Renderer2DT& Renderer, bool RenderVertexHandles, bool RenderEdgeHandles) const
{
    if (m_RenderBP)
    {
        m_RenderBP->Render2D(Renderer);
    }


    // Draw the outline of the morph primitive.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(255, 0, 0));

    for (unsigned long EdgeNr=0; EdgeNr<m_Edges.Size(); EdgeNr++)
    {
        const MP_EdgeT* Edge=m_Edges[EdgeNr];

        Renderer.DrawLine(Renderer.GetViewWin2D().WorldToTool(Edge->Vertices[0]->pos),
                          Renderer.GetViewWin2D().WorldToTool(Edge->Vertices[1]->pos));
    }


    // Now draw all handles.
    // As handles frequently overlap each other, we choose the draw order such that:
    // a) vertex handles render over edge handles, and
    // b) selected handles render over unselected handles.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(0, 0, 0));

    for (int Pass=0; Pass<2; Pass++)
    {
        const bool IsSelectedPass=(Pass==1);

        if (RenderEdgeHandles)
        {
            for (unsigned long EdgeNr=0; EdgeNr<m_Edges.Size(); EdgeNr++)
            {
                const MP_EdgeT* Edge=m_Edges[EdgeNr];

                if (Edge->m_Selected!=IsSelectedPass) continue;

                const wxPoint Point=Renderer.GetViewWin2D().WorldToTool(Edge->GetPos());

                Renderer.SetFillColor(Edge->m_Selected ? Options.colors.Selection : wxColour(255, 255, 0));
                Renderer.DrawPoint(Point, 3);
            }
        }

        if (RenderVertexHandles)
        {
            for (unsigned long VertexNr=0; VertexNr<m_Vertices.Size(); VertexNr++)
            {
                const MP_VertexT* Vertex=m_Vertices[VertexNr];

                if (Vertex->m_Selected!=IsSelectedPass) continue;

                const wxPoint Point=Renderer.GetViewWin2D().WorldToTool(Vertex->pos);

                Renderer.SetFillColor(Vertex->m_Selected ? Options.colors.Selection : Options.colors.ToolHandle);
                Renderer.DrawPoint(Point, 3);
            }
        }
    }
}


MP_VertexT* MorphPrimT::GetConnectionVertex(MP_EdgeT* Edge1, MP_EdgeT* Edge2) const
{
    if ((Edge1->Vertices[0]==Edge2->Vertices[0]) || (Edge1->Vertices[0]==Edge2->Vertices[1])) return Edge1->Vertices[0];
    if ((Edge1->Vertices[1]==Edge2->Vertices[0]) || (Edge1->Vertices[1]==Edge2->Vertices[1])) return Edge1->Vertices[1];

    return NULL;
}


void MorphPrimT::RenderHandle(Renderer3DT& Renderer, const wxPoint& ClientPos, const float* color) const
{
    MatSys::MeshT Mesh(MatSys::MeshT::TriangleFan);
    Mesh.Vertices.PushBackEmpty(4);

    const int HANDLE_SIZE=3;

    // Beware that the GetRMatFlatShaded() material does *not* have the "twoSided" keyword,
    // that is, it is not drawn if the mesh orientation towards the viewer is backwards!
    Mesh.Vertices[0].SetOrigin(ClientPos.x-HANDLE_SIZE, ClientPos.y-HANDLE_SIZE, 0);
    Mesh.Vertices[1].SetOrigin(ClientPos.x+HANDLE_SIZE, ClientPos.y-HANDLE_SIZE, 0);
    Mesh.Vertices[2].SetOrigin(ClientPos.x+HANDLE_SIZE, ClientPos.y+HANDLE_SIZE, 0);
    Mesh.Vertices[3].SetOrigin(ClientPos.x-HANDLE_SIZE, ClientPos.y+HANDLE_SIZE, 0);

    Mesh.Vertices[0].SetColor(color[0], color[1], color[2], color[3]);
    Mesh.Vertices[1].SetColor(color[0], color[1], color[2], color[3]);
    Mesh.Vertices[2].SetColor(color[0], color[1], color[2], color[3]);
    Mesh.Vertices[3].SetColor(color[0], color[1], color[2], color[3]);

    MatSys::Renderer->SetCurrentMaterial(Renderer.GetRMatFlatShaded());
    MatSys::Renderer->RenderMesh(Mesh);


    Mesh.Type=MatSys::MeshT::LineLoop;

    Mesh.Vertices[0].SetColor(0.08f, 0.08f, 0.08f);
    Mesh.Vertices[1].SetColor(0.08f, 0.08f, 0.08f);
    Mesh.Vertices[2].SetColor(0.08f, 0.08f, 0.08f);
    Mesh.Vertices[3].SetColor(0.08f, 0.08f, 0.08f);

    MatSys::Renderer->SetCurrentMaterial(Renderer.GetRMatWireframe());
    MatSys::Renderer->RenderMesh(Mesh);
}


void MorphPrimT::Render(Renderer3DT& Renderer, bool RenderVertexHandles, bool RenderEdgeHandles) const
{
    if (m_RenderBP)
    {
        m_RenderBP->Render3D(Renderer);
    }


    for (int Pass=1; Pass<=2; Pass++)
    {
        for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
        {
            const MP_FaceT* Face=m_Faces[FaceNr];

            // Note that in the wire-frame pass, we must use a MatSys::MeshT::Polygon rather than a MatSys::MeshT::LineLoop
            // mesh, or else the depth offset won't work right (they are probably differently rasterized).
            MatSys::MeshT Mesh(Pass==1 ? MatSys::MeshT::Polygon : MatSys::MeshT::TriangleFan);
            Mesh.Vertices.PushBackEmpty(Face->Edges.Size());

            const float color[2][4]={ { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.2f, 0.2f, 0.2f, 0.5f } };

            for (unsigned long Edge=0; Edge<Face->Edges.Size(); Edge++)
            {
                MP_VertexT* Vertex=GetConnectionVertex(Face->Edges[Edge], Face->Edges[(Edge+1) % Face->Edges.Size()]);

                if (!Vertex) return;

                Mesh.Vertices[Edge].SetOrigin(Vertex->pos);
                Mesh.Vertices[Edge].SetColor(color[Pass-1][0], color[Pass-1][1], color[Pass-1][2], color[Pass-1][3]);
            }

            MatSys::Renderer->SetCurrentMaterial(Pass==1 ? Renderer.GetRMatWireframe_OffsetZ() : Renderer.GetRMatFlatShaded());
            MatSys::Renderer->RenderMesh(Mesh);
        }
    }


    Renderer3DT::UseOrthoMatricesT UseOrtho(Renderer.GetViewWin3D());

    if (RenderVertexHandles)
    {
        for (unsigned long i=0; i<m_Vertices.Size(); i++)
        {
            // Compute the position of the vertex handle in window coordinates.
            const wxPoint HandlePos=Renderer.GetViewWin3D().WorldToWindow(m_Vertices[i]->pos, true /*Yes, check against view frustum.*/);

            if (HandlePos.x>0 && HandlePos.y>0)
            {
                const float ColorSel[4]={ 0.8f, 0.0f, 0.0f, 1.0f };
                const float ColorUnS[4]={ 1.0f, 1.0f, 1.0f, 1.0f };

                RenderHandle(Renderer, HandlePos, m_Vertices[i]->m_Selected ? ColorSel : ColorUnS);
            }
        }
    }

    if (RenderEdgeHandles)
    {
        for (unsigned long i=0; i<m_Edges.Size(); i++)
        {
            // Compute the position of the edge handle in window coordinates.
            const wxPoint HandlePos=Renderer.GetViewWin3D().WorldToWindow(m_Edges[i]->GetPos(), true /*Yes, check against view frustum.*/);

            if (HandlePos.x>0 && HandlePos.y>0)
            {
                const float ColorSel[4]={ 0.8f, 0.0f, 0.0f, 1.0f };
                const float ColorUnS[4]={ 1.0f, 1.0f, 0.0f, 1.0f };

                RenderHandle(Renderer, HandlePos, m_Edges[i]->m_Selected ? ColorSel : ColorUnS);
            }
        }
    }
}


void MorphPrimT::MoveSelectedHandles(const Vector3fT& Delta)
{
    ArrayT<MP_VertexT*> MoveVertices;

    for (unsigned long VertexNr=0; VertexNr<m_Vertices.Size(); VertexNr++)
    {
        if (m_Vertices[VertexNr]->m_Selected)
        {
            MoveVertices.PushBack(m_Vertices[VertexNr]);
        }
    }

    for (unsigned long EdgeNr=0; EdgeNr<m_Edges.Size(); EdgeNr++)
    {
        MP_EdgeT* Edge=m_Edges[EdgeNr];

        if (Edge->m_Selected)
        {
            if (MoveVertices.Find(Edge->Vertices[0])==-1) MoveVertices.PushBack(Edge->Vertices[0]);
            if (MoveVertices.Find(Edge->Vertices[1])==-1) MoveVertices.PushBack(Edge->Vertices[1]);
        }
    }

    if (MoveVertices.Size()==0) return;

    for (unsigned long VertexNr=0; VertexNr<MoveVertices.Size(); VertexNr++) MoveVertices[VertexNr]->pos+=Delta;
    m_Modified=true;

    if (m_MapPrim->GetType()==&MapBezierPatchT::TypeInfo) UpdatePatch();
    if (m_MapPrim->GetType()==      &MapBrushT::TypeInfo) UpdateBrushFromVertices();
}


void MorphPrimT::UpdateBrushFromVertices()
{
    if (dynamic_cast<const MapBrushT*>(m_MapPrim)==NULL) return;

    const float Epsilon=0.1f;


    // Remove all duplicates from the m_Vertices array.
    for (unsigned long V1Nr=0; V1Nr+1<m_Vertices.Size(); V1Nr++)
        for (unsigned long V2Nr=V1Nr+1; V2Nr<m_Vertices.Size(); V2Nr++)
            if (m_Vertices[V1Nr]->pos.IsEqual(m_Vertices[V2Nr]->pos, Epsilon))
            {
                delete m_Vertices[V2Nr];
                m_Vertices.RemoveAt(V2Nr);
                V2Nr--;
            }


    // Build the convex hull over the m_Vertices.
    ArrayT<Vector3fT> Points;
    for (unsigned long VNr=0; VNr<m_Vertices.Size(); VNr++)
        Points.PushBack(m_Vertices[VNr]->pos);

    ArrayT< Plane3T<float> > HullPlanes;
    Plane3T<float>::ConvexHull(Points, HullPlanes, NULL, Epsilon);


    // Compute the center and radius of a bounding-sphere of this brush.
    // These values help with reducing rounding errors in the Polygon3T<>::Complete() function below.
    const BoundingBox3T<float> BBox(Points);
    const Vector3fT            BoundingSphereCenter=(BBox.Min+BBox.Max)/2.0f;
    const float                BoundingSphereRadius=length(BBox.Max-BBox.Min)/2.0f;


    // Compute a complete brush from the HullPlanes.
    ArrayT< Polygon3T<float> > Brush;

    Brush.PushBackEmpty(HullPlanes.Size());
    for (unsigned long PlaneNr=0; PlaneNr<Brush.Size(); PlaneNr++) Brush[PlaneNr].Plane=HullPlanes[PlaneNr];

    Polygon3T<float>::Complete(Brush, Epsilon, BoundingSphereCenter, BoundingSphereRadius);


    // Remove invalid faces from Brush.
    for (unsigned long FaceNr=0; FaceNr<Brush.Size(); FaceNr++)
        if (!Brush[FaceNr].IsValid(Epsilon, Epsilon))
        {
            Brush.RemoveAt(FaceNr);
            FaceNr--;
        }


    // Now regenerate the m_Edges and m_Faces arrays.
    ArrayT<MP_EdgeT*> OldEdges=m_Edges;      // For reconstructing selected edges.
    m_Edges.Clear();

    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++) delete m_Faces[FaceNr];
    m_Faces.Clear();

    for (unsigned long FaceNr=0; FaceNr<Brush.Size(); FaceNr++)
    {
        m_Faces.PushBack(new MP_FaceT);

        const Polygon3T<float>& Poly=Brush[FaceNr];
        MP_FaceT&               Face=*m_Faces[FaceNr];

        for (unsigned long VertexNr=0; VertexNr<Poly.Vertices.Size(); VertexNr++)
        {
            const unsigned long PrevVertexNr=(VertexNr==0) ? Poly.Vertices.Size()-1 : VertexNr-1;

            // Find the vertices in m_Vertices that match VertexNr and PrevVertexNr.
            MP_VertexT* v1=FindClosestVertex(Poly.Vertices[PrevVertexNr]);
            MP_VertexT* v2=FindClosestVertex(Poly.Vertices[VertexNr    ]);

            wxASSERT(v1!=NULL);
            wxASSERT(v2!=NULL);

            // Note that if the planes have sharp angles against each other, small rounding errors
            // in the normal-vector may yield large errors when the vertices are recomputed.
         // wxASSERT(length(v1->pos-Poly.Vertices[PrevVertexNr])<1.0f);
         // wxASSERT(length(v2->pos-Poly.Vertices[VertexNr    ])<1.0f);

            // This should never happen, as the convex hull algorithm cannot introduce new vertices and rounding errors should be neglectible.
            if (v1==NULL || v2==NULL) continue;

            MP_EdgeT* Edge=FindEdge(v1, v2);

            if (Edge==NULL)
            {
                Edge=new MP_EdgeT;
                m_Edges.PushBack(Edge);

                Edge->Vertices[0]=v1;
                Edge->Vertices[1]=v2;
            }
            else
            {
                // Must have assigned exactly one face before.
                const bool IsValid=(Edge->Faces[0]!=NULL && Edge->Faces[1]==NULL);

                wxASSERT(IsValid);
                if (!IsValid) continue;
            }

            // Add a pointer to the edge to the face.
            Face.Edges.PushBack(Edge);

            // Set edge's face array.
                 if (Edge->Faces[0]==NULL) Edge->Faces[0]=&Face;
            else if (Edge->Faces[1]==NULL) Edge->Faces[1]=&Face;
            else continue;
        }
    }


    // Restore the m_Selected member of the new edges as well as possible.
    for (unsigned long EdgeNr=0; EdgeNr<m_Edges.Size(); EdgeNr++)
        for (unsigned long OldEdgeNr=0; OldEdgeNr<OldEdges.Size(); OldEdgeNr++)
            if ((m_Edges[EdgeNr]->Vertices[0]==OldEdges[OldEdgeNr]->Vertices[0] && m_Edges[EdgeNr]->Vertices[1]==OldEdges[OldEdgeNr]->Vertices[1]) ||
                (m_Edges[EdgeNr]->Vertices[0]==OldEdges[OldEdgeNr]->Vertices[1] && m_Edges[EdgeNr]->Vertices[1]==OldEdges[OldEdgeNr]->Vertices[0]))
            {
                m_Edges[EdgeNr]->m_Selected=OldEdges[OldEdgeNr]->m_Selected;
                break;
            }

    for (unsigned long EdgeNr=0; EdgeNr<OldEdges.Size(); EdgeNr++) delete OldEdges[EdgeNr];
    OldEdges.Clear();
}


void MorphPrimT::UpdatePatch()
{
    for (unsigned long y=0; y<m_RenderBP->GetHeight(); y++)
    {
        for (unsigned long x=0; x<m_RenderBP->GetWidth(); x++)
        {
            m_RenderBP->SetCvPos(x, y, m_Vertices[y*m_RenderBP->GetWidth()+x]->pos);
        }
    }
}
