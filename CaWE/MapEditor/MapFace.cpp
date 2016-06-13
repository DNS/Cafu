/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrameViewWin3D.hpp"
#include "MapFace.hpp"
#include "MapBrush.hpp"
#include "Renderer3D.hpp"
#include "MapDocument.hpp"
#include "ChildFrame.hpp"
#include "DialogEditSurfaceProps.hpp"

#include "../AppCaWE.hpp"
#include "../Camera.hpp"
#include "../EditorMaterial.hpp"
#include "../GameConfig.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"

#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

#include "wx/wx.h"


MapFaceT::MapFaceT(EditorMaterialI* Material)
    : m_Material(Material),
      m_IsSelected(false)
{
    m_PlanePoints.PushBackEmpty(3);

    // Note that surface info is not properly initialized here. This is only needed to avoid assertions.
    m_SurfaceInfo.TexCoordGenMode=PlaneProj;

    // Update the surface infos default scales. TODO: Can we improve this?
    const MapDocumentT* MapDoc=wxGetApp().GetParentFrame()->GetActiveMapDoc();

    m_SurfaceInfo.Scale[0]=1.0f/((MapDoc!=NULL ? MapDoc->GetGameConfig()->DefaultTextureScale : 1.0f)*(m_Material ? m_Material->GetWidth()  : 128));
    m_SurfaceInfo.Scale[1]=1.0f/((MapDoc!=NULL ? MapDoc->GetGameConfig()->DefaultTextureScale : 1.0f)*(m_Material ? m_Material->GetHeight() : 128));
    m_SurfaceInfo.LightmapScale=  MapDoc!=NULL ? MapDoc->GetGameConfig()->DefaultLightmapScale : 16.0f;
}


MapFaceT::MapFaceT(EditorMaterialI* Material, const Plane3fT& Plane, const Vector3fT* PlanePoints, bool FaceAligned)
    : m_Material(Material),
      m_SurfaceInfo(Plane, FaceAligned),
      m_Plane(Plane),
      m_IsSelected(false)
{
    // Update the surface infos default scales. TODO: Can we improve this?
    const MapDocumentT* MapDoc=wxGetApp().GetParentFrame()->GetActiveMapDoc();

    m_SurfaceInfo.Scale[0]=1.0f/((MapDoc!=NULL ? MapDoc->GetGameConfig()->DefaultTextureScale : 1.0f)*(m_Material ? m_Material->GetWidth()  : 128));
    m_SurfaceInfo.Scale[1]=1.0f/((MapDoc!=NULL ? MapDoc->GetGameConfig()->DefaultTextureScale : 1.0f)*(m_Material ? m_Material->GetHeight() : 128));
    m_SurfaceInfo.LightmapScale=  MapDoc!=NULL ? MapDoc->GetGameConfig()->DefaultLightmapScale : 16.0f;


    if (PlanePoints)
    {
        // Assert that the Plane was actually built from the PlanePoints - that's their entire purpose!
        wxASSERT(m_Plane==Plane3fT(PlanePoints[0], PlanePoints[1], PlanePoints[2], 0.0f));

        // Note that these PlanePoints are very important - they guarantee that after a map save/load cycle,
        // we're able to reconstruct the exact same plane again! (No loss of numerical precision.)
        m_PlanePoints.PushBack(PlanePoints[0]);
        m_PlanePoints.PushBack(PlanePoints[1]);
        m_PlanePoints.PushBack(PlanePoints[2]);
    }
    else
    {
        // Compute the m_PlanePoints from the m_Plane.
        Vector3fT Span1;
        Vector3fT Span2;

        m_Plane.GetSpanVectorsByRotation(Span1, Span2);

        // Assert that the output is as expected.
        wxASSERT(fabs(dot(m_Plane.Normal, Span1))<0.01f);
        wxASSERT(fabs(dot(m_Plane.Normal, Span2))<0.01f);
        wxASSERT(fabs(dot(Span1, Span2))<0.01f);

        m_PlanePoints.PushBack(m_Plane.Normal*m_Plane.Dist);
        m_PlanePoints.PushBack(m_PlanePoints[0]+Span1*1000.0f);
        m_PlanePoints.PushBack(m_PlanePoints[0]+Span2*1000.0f);
    }
}


void MapFaceT::SetMaterial(EditorMaterialI* Material)
{
    m_Material=Material;
}


void MapFaceT::SetSurfaceInfo(const SurfaceInfoT& SI)
{
    m_SurfaceInfo=SI;
    UpdateTextureSpace();
}


Vector3fT MapFaceT::GetCenter() const
{
    Vector3fT Center;

    for (unsigned long VertexNr=0; VertexNr<m_Vertices.Size(); VertexNr++)
        Center+=m_Vertices[VertexNr];

    if (m_Vertices.Size())
        Center/=m_Vertices.Size();

    return Center;
}


bool MapFaceT::IsUVSpaceFaceAligned() const
{
    wxASSERT(m_SurfaceInfo.TexCoordGenMode==PlaneProj);
    if (m_SurfaceInfo.TexCoordGenMode!=PlaneProj) return false;

    // The uv-space of this face is face-aligned when both UAxis and VAxis are orthogonal to m_Plane.Normal.
    return fabs(dot(m_SurfaceInfo.UAxis, m_Plane.Normal))<0.001f &&
           fabs(dot(m_SurfaceInfo.VAxis, m_Plane.Normal))<0.001f;
}


bool MapFaceT::IsUVSpaceWorldAligned() const
{
    wxASSERT(m_SurfaceInfo.TexCoordGenMode==PlaneProj);
    if (m_SurfaceInfo.TexCoordGenMode!=PlaneProj) return false;

    // The uv-space of this face is world-aligned when both UAxis and VAxis are in the same principal plane.
    if (fabs(m_SurfaceInfo.UAxis.x)<0.001f && fabs(m_SurfaceInfo.VAxis.x)<0.001f) return true;
    if (fabs(m_SurfaceInfo.UAxis.y)<0.001f && fabs(m_SurfaceInfo.VAxis.y)<0.001f) return true;
    if (fabs(m_SurfaceInfo.UAxis.z)<0.001f && fabs(m_SurfaceInfo.VAxis.z)<0.001f) return true;

    return false;
}


void MapFaceT::Render3DBasic(MatSys::RenderMaterialT* RenderMat, const wxColour& MeshColor, const int MeshAlpha) const
{
    if (!RenderMat) return;

    // A triangle-fan instead of a polygon for wire-frame materials would also render the "inner" fan lines.
    static MatSys::MeshT Mesh;

    Mesh.Type=(MatSys::Renderer->GetMaterialFromRM(RenderMat)->PolygonMode==MaterialT::Filled) ? MatSys::MeshT::TriangleFan : MatSys::MeshT::Polygon;
    Mesh.Vertices.Overwrite();
    Mesh.Vertices.PushBackEmpty(m_Vertices.Size());

    for (unsigned long VertexNr=0; VertexNr<m_Vertices.Size(); VertexNr++)
    {
        Mesh.Vertices[VertexNr].SetTextureCoord (m_TextureCoords [VertexNr][0], m_TextureCoords [VertexNr][1]);
        Mesh.Vertices[VertexNr].SetLightMapCoord(m_LightmapCoords[VertexNr][0], m_LightmapCoords[VertexNr][1]);
        Mesh.Vertices[VertexNr].SetColor(MeshColor.Red()/255.0f, MeshColor.Green()/255.0f, MeshColor.Blue()/255.0f, MeshAlpha/255.0f);

        Mesh.Vertices[VertexNr].SetOrigin(m_Vertices[VertexNr]);

        Mesh.Vertices[VertexNr].SetNormal(m_Plane.Normal);
        Mesh.Vertices[VertexNr].SetTangent(m_Tangents[VertexNr]);
        Mesh.Vertices[VertexNr].SetBiNormal(m_BiTangents[VertexNr]);
    }

    MatSys::Renderer->SetCurrentMaterial(RenderMat);
    MatSys::Renderer->RenderMesh(Mesh);
}


static wxColour ScaleColor(const wxColour& Color, float Scale)
{
    return wxColour((unsigned char)(Color.Red()*Scale), (unsigned char)(Color.Green()*Scale), (unsigned char)(Color.Blue()*Scale));
}


void MapFaceT::Render3D(Renderer3DT& Renderer, const MapBrushT* ParentBrush) const
{
    if (m_Vertices.Size()==0) return;

    // Do some culling...  TODO: Take the following conditions into account:
    //   - !MatSys::Renderer->GetMaterialFromRM(RenderMat).TwoSided
    //   - MatSys::Renderer->GetMaterialFromRM(RenderMat).PolygonMode!=MaterialT::Wireframe
    //   - The current MODEL_TO_WORLD matrix must be the identity matrix (because the camera is given in world-space).
    //     (Alternatively use the viewpoint in modelspace(!) for the test.)
    if (m_Plane.GetDistance(Renderer.GetViewWin3D().GetCamera().Pos)<0 /*&& mode!=RENDER_MODE_WIREFRAME*/ /*&& MeshAlpha==255*/) return;

    const bool  RenderAsSelected=ParentBrush->IsSelected() || (m_IsSelected && Renderer.GetViewWin3D().GetChildFrame()->GetSurfacePropsDialog()->WantSelectionOverlay());
    const bool  FullMats        =(Renderer.GetViewWin3D().GetViewType()==ViewWindowT::VT_3D_FULL_MATS);
    const float Shade           =Renderer.GetConstShade(m_Plane.Normal);

    switch (Renderer.GetViewWin3D().GetViewType())
    {
        case ViewWindowT::VT_3D_EDIT_MATS:
        case ViewWindowT::VT_3D_FULL_MATS:
            // Note that the mesh color is ignored for most normal materials anyway... (they don't have the "useMeshColors" property).
            Render3DBasic(m_Material!=NULL ? m_Material->GetRenderMaterial(FullMats) : Renderer.GetRMatFlatShaded(),
                ScaleColor(*wxWHITE, Shade), 255);

            if (RenderAsSelected)
            {
                Render3DBasic(Renderer.GetRMatOverlay(), Options.colors.SelectedFace, 64);
                Render3DBasic(Renderer.GetRMatWireframe_OffsetZ(), wxColour(255, 255, 0), 255);
            }
            break;

        case ViewWindowT::VT_3D_LM_GRID:
        case ViewWindowT::VT_3D_LM_PREVIEW:
            Render3DBasic(Renderer.GetRMatFlatShaded(),
                ScaleColor(RenderAsSelected ? Options.colors.SelectedFace : (m_SurfaceInfo.LightmapScale!=Renderer.GetViewWin3D().GetMapDoc().GetGameConfig()->DefaultLightmapScale ? wxColour(255, 255, 100) : *wxWHITE), Shade), 255);
            break;

        case ViewWindowT::VT_3D_FLAT:
            Render3DBasic(Renderer.GetRMatFlatShaded(),
                ScaleColor(RenderAsSelected ? Options.colors.SelectedFace : ParentBrush->GetColor(Options.view2d.UseGroupColors), Shade), 255);
            break;

        case ViewWindowT::VT_3D_WIREFRAME:
            Render3DBasic(Renderer.GetRMatWireframe(),
                ScaleColor(RenderAsSelected ? Options.colors.SelectedEdge : ParentBrush->GetColor(Options.view2d.UseGroupColors), Shade), 255);
            break;

        default:
            wxASSERT(0);
            break;
    }

    // Render the texture-space UV axes.
    if (m_IsSelected && m_SurfaceInfo.TexCoordGenMode==PlaneProj)
    {
        // Getting this right regarding z-offset is pretty difficult, because for applying z-offset, we cannot simply use a mesh
        // of type MatSys::MeshT::Lines in order to draw the u- and v-axis, but have to use type MatSys::MeshT::Triangles instead.
        // This is because z-offset can only be used with "real" polygons, but not with OpenGL lines or points.
        // (It seems that the polygons must be non-degenerate as well.)
        // I experimented with triangles for drawing the axes, which however did not yield visually satisfactory results.
        // Therefore, I now employ "spatial" offset along the planes normal vector rather than z-offset, obtaining good results.
        const Vector3fT Center=GetCenter() + m_Plane.Normal*0.5f;

        MatSys::MeshT Mesh(MatSys::MeshT::Lines);
        Mesh.Vertices.PushBackEmpty(4);

        Mesh.Vertices[0].SetColor(1, 1, 0);
        Mesh.Vertices[0].SetOrigin(Center);

        Mesh.Vertices[1].SetColor(1, 1, 0);
        Mesh.Vertices[1].SetOrigin(Center + m_SurfaceInfo.UAxis*10.0f);

        Mesh.Vertices[2].SetColor(0, 1, 0);
        Mesh.Vertices[2].SetOrigin(Center);

        Mesh.Vertices[3].SetColor(0, 1, 0);
        Mesh.Vertices[3].SetOrigin(Center + m_SurfaceInfo.VAxis*10.0f);

        MatSys::Renderer->SetCurrentMaterial(Renderer.GetRMatWireframe());
        MatSys::Renderer->RenderMesh(Mesh);
    }
}


void MapFaceT::UpdateTextureSpace()
{
    wxASSERT(m_SurfaceInfo.TexCoordGenMode==PlaneProj);

    m_TextureCoords .Overwrite(); m_TextureCoords .PushBackEmpty(m_Vertices.Size());
    m_LightmapCoords.Overwrite(); m_LightmapCoords.PushBackEmpty(m_Vertices.Size());
    m_Tangents      .Overwrite(); m_Tangents      .PushBackEmpty(m_Vertices.Size());
    m_BiTangents    .Overwrite(); m_BiTangents    .PushBackEmpty(m_Vertices.Size());

    if (m_SurfaceInfo.UAxis.GetLengthSqr()<0.01*0.01 || m_SurfaceInfo.VAxis.GetLengthSqr()<0.01*0.01)
    {
        // We can legitimately get here for example as a result of the Clip-Tool generating new, clipped brushes.
        // Without this check however, normalize(m_SurfaceInfo.VAxis) below would throw an uncaught exception.
        // I think that the proper long-term solution is to revise the creation of MapFaces,
        // which should be more compact (e.g. in ctors), not scattered across several methods,
        // in between of which the MapFace is in an inconsistent state (a "zombie").
        // In any case, revised Face construction should make sure that consistency is maintained at all times.
     // wxLogDebug("MapFaceT::UpdateTextureSpace() has been called with bad UAxis or VAxis!");
        return;
    }

    // See FaceNodeT::InitRenderMeshesAndMats() in Libs/SceneGraph/FaceNode.cpp for details.
    const Vector3fT UxV          =cross(m_SurfaceInfo.UAxis, m_SurfaceInfo.VAxis);
    const Vector3fT FaceTangent  =normalizeOr0(m_SurfaceInfo.UAxis + scale(UxV, -dot(m_Plane.Normal, m_SurfaceInfo.UAxis)/dot(m_Plane.Normal, UxV)));
    const Vector3fT FaceBiTangent=normalizeOr0(m_SurfaceInfo.VAxis + scale(UxV, -dot(m_Plane.Normal, m_SurfaceInfo.VAxis)/dot(m_Plane.Normal, UxV)));

    for (unsigned long VertexNr=0; VertexNr<m_Vertices.Size(); VertexNr++)
    {
        m_TextureCoords[VertexNr][0]=dot(m_SurfaceInfo.UAxis, m_Vertices[VertexNr]) * m_SurfaceInfo.Scale[0] + m_SurfaceInfo.Trans[0];
        m_TextureCoords[VertexNr][1]=dot(m_SurfaceInfo.VAxis, m_Vertices[VertexNr]) * m_SurfaceInfo.Scale[1] + m_SurfaceInfo.Trans[1];

        m_LightmapCoords[VertexNr][0]=dot(m_SurfaceInfo.UAxis, m_Vertices[VertexNr]) / m_SurfaceInfo.LightmapScale + 0.5f;
        m_LightmapCoords[VertexNr][1]=dot(m_SurfaceInfo.VAxis, m_Vertices[VertexNr]) / m_SurfaceInfo.LightmapScale + 0.5f;

        m_Tangents  [VertexNr]=FaceTangent;
        m_BiTangents[VertexNr]=FaceBiTangent;
    }
}
