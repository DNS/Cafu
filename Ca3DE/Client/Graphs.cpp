/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Graphs.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"


GraphsT::GraphsT()
{
#ifndef DEBUG
    // Simply use the Guis default material, assuming that the script it is defined in has already been registered.
    m_RMatWireframe = MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("wireframe"));
#else
    // Use the CaWE materials.
    // The problem here is that the ./CaWE/res/CaWE.cmat not necessarily ships with the public demo packages...
    MaterialManager->RegisterMaterialScript("./CaWE/res/CaWE.cmat", "./CaWE/res/");

    m_RMatWireframe    = MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/Wireframe"        ));
 // m_RMatWireframeOZ  = MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/WireframeOffsetZ" ));
 // m_RMatFlatShaded   = MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/FlatShaded"       ));
 // m_RMatFlatShadedOZ = MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/FlatShadedOffsetZ"));
 // m_RMatOverlay      = MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/Overlay"          ));
 // m_RMatOverlayOZ    = MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/OverlayOffsetZ"   ));
#endif

    assert(m_RMatWireframe);

    for (unsigned long FrameIndex=0; FrameIndex<512; FrameIndex++)
        ClearForFrame(FrameIndex);
}


GraphsT::~GraphsT()
{
    MatSys::Renderer->FreeMaterial(m_RMatWireframe   );
 // MatSys::Renderer->FreeMaterial(m_RMatWireframeOZ );
 // MatSys::Renderer->FreeMaterial(m_RMatFlatShaded  );
 // MatSys::Renderer->FreeMaterial(m_RMatFlatShadedOZ);
 // MatSys::Renderer->FreeMaterial(m_RMatOverlay     );
 // MatSys::Renderer->FreeMaterial(m_RMatOverlayOZ   );
}


void GraphsT::ClearForFrame(unsigned long ClientFrameNr)
{
    const unsigned long FrameIndex = ClientFrameNr & (512 - 1);

    FPS    [FrameIndex] = 0;
    Heading[FrameIndex] = 0;
    PosY   [FrameIndex] = 0;
    PosZ   [FrameIndex] = 0;
}


void GraphsT::Draw(unsigned long ClientFrameNr, unsigned int fbWidth, unsigned int fbHeight) const
{
    int FrameNr = 512;

    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );

    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, float(fbWidth), float(fbHeight), 0.0f, -1.0f, 1.0f));

    static MatSys::MeshT LinesMesh (MatSys::MeshT::Lines ); LinesMesh .Vertices.Overwrite();
    static MatSys::MeshT PointsMesh(MatSys::MeshT::Points); PointsMesh.Vertices.Overwrite();

    while (FrameNr--)
    {
        const int x = (fbWidth - 512) / 2 + FrameNr;
        const int y = fbHeight - 10;

        const unsigned long FrameIndex = ClientFrameNr & (512 - 1);

        LinesMesh.Vertices.PushBackEmpty();
        LinesMesh.Vertices[LinesMesh.Vertices.Size()-1].SetColor(0.5, 0.5, 0.5);
        LinesMesh.Vertices[LinesMesh.Vertices.Size()-1].SetOrigin(x, y);

        LinesMesh.Vertices.PushBackEmpty();
        LinesMesh.Vertices[LinesMesh.Vertices.Size()-1].SetColor(0.5, 0.5, 0.5);
        LinesMesh.Vertices[LinesMesh.Vertices.Size()-1].SetOrigin(x, y - FPS[FrameIndex]);


     // PointsMesh.Vertices.PushBackEmpty();
     // PointsMesh.Vertices[PointsMesh.Vertices.Size()-1].SetColor(1.0, 0.0, 1.0);
     // PointsMesh.Vertices[PointsMesh.Vertices.Size()-1].SetOrigin(x, y - Heading[FrameIndex]);

        PointsMesh.Vertices.PushBackEmpty();
        PointsMesh.Vertices[PointsMesh.Vertices.Size()-1].SetColor(0.0, 1.0, 0.0);
        PointsMesh.Vertices[PointsMesh.Vertices.Size()-1].SetOrigin(x, y - PosY[FrameIndex]);

        PointsMesh.Vertices.PushBackEmpty();
        PointsMesh.Vertices[PointsMesh.Vertices.Size()-1].SetColor(0.0, 0.0, 1.0);
        PointsMesh.Vertices[PointsMesh.Vertices.Size()-1].SetOrigin(x, y - PosZ[FrameIndex]);

        ClientFrameNr--;
    }

    MatSys::Renderer->SetCurrentMaterial(m_RMatWireframe);
    MatSys::Renderer->RenderMesh(LinesMesh );
    MatSys::Renderer->RenderMesh(PointsMesh);

    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
}
