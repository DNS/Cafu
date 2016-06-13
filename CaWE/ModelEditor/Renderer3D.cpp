/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Renderer3D.hpp"

#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

#include "wx/wx.h"

#include <algorithm>


ModelEditor::Renderer3DT::Renderer3DT()
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
    m_RMatWhite            =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/White"            ));
    m_RMatTexturedWireframe=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/TexturedWireframe"));
}


ModelEditor::Renderer3DT::~Renderer3DT()
{
    MatSys::Renderer->FreeMaterial(m_RMatWireframe        );
    MatSys::Renderer->FreeMaterial(m_RMatWireframeOZ      );
    MatSys::Renderer->FreeMaterial(m_RMatFlatShaded       );
    MatSys::Renderer->FreeMaterial(m_RMatFlatShadedOZ     );
    MatSys::Renderer->FreeMaterial(m_RMatOverlay          );
    MatSys::Renderer->FreeMaterial(m_RMatOverlayOZ        );
    MatSys::Renderer->FreeMaterial(m_RMatTerrainEdit      );
    MatSys::Renderer->FreeMaterial(m_RMatTerrainEyeDropper);
    MatSys::Renderer->FreeMaterial(m_RMatWhite            );
    MatSys::Renderer->FreeMaterial(m_RMatTexturedWireframe);
}


float ModelEditor::Renderer3DT::GetConstShade(const Vector3T<float>& Normal) const
{
    const static Vector3fT Light =Vector3fT(0.2f, 0.4f, 0.7f);
    const static float     LightL=length(Light);

    return std::min(dot(Normal, Light/LightL)*0.4f + 0.6f, 1.0f);
}


void ModelEditor::Renderer3DT::RenderBox(const BoundingBox3fT& BB, const wxColour& Color, bool Solid) const
{
    Vector3fT Vertices[8];

    BB.GetCornerVertices(Vertices);
    RenderBox(Vertices, Color, Solid);
}


void ModelEditor::Renderer3DT::RenderBox(const Vector3fT Vertices[], const wxColour& Color, bool Solid) const
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


void ModelEditor::Renderer3DT::RenderLine(const Vector3fT& A, const Vector3fT& B, const wxColour& Color) const
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


void ModelEditor::Renderer3DT::BasisVectors(const Vector3fT& Pos, const cf::math::Matrix3x3fT& Mat, float Length) const
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


void ModelEditor::Renderer3DT::RenderCrossHair(const wxPoint& Center) const
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
