/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "EditorWindow.hpp"

#include "GuiSys/GuiImpl.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"


using namespace GuiEditor;


EditorWindowT::EditorWindowT(IntrusivePtrT<cf::GuiSys::WindowT> Win)
    : m_Win(Win),
      m_IsSelected(false)
{
}


void EditorWindowT::Render() const
{
    // Render selection state of this window.
    if (m_IsSelected)
    {
        const float SelectionColor[] = { 1.0, 0.0, 0.0, 1.0 };
        const float SelectionBorder  = 2;

        // Render selection border.
        const float x1 = 0.0f;
        const float y1 = 0.0f;

        const float x2 = m_Win->GetTransform()->GetSize().x;
        const float y2 = m_Win->GetTransform()->GetSize().y;

        MatSys::Renderer->SetCurrentMaterial(m_Win->GetGui().GetDefaultRM());

        static MatSys::MeshT BorderMesh(MatSys::MeshT::Quads);
        BorderMesh.Vertices.Overwrite();
        BorderMesh.Vertices.PushBackEmpty(4*4);     // One rectangle for each side of the background.

        for (unsigned long VertexNr=0; VertexNr<BorderMesh.Vertices.Size(); VertexNr++)
        {
            for (unsigned long i=0; i<4; i++)
                BorderMesh.Vertices[VertexNr].Color[i]=SelectionColor[i];
        }

        // Left border rectangle.
        BorderMesh.Vertices[ 0].SetOrigin(x1,                 y1); BorderMesh.Vertices[ 0].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 1].SetOrigin(x1+SelectionBorder, y1); BorderMesh.Vertices[ 1].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 2].SetOrigin(x1+SelectionBorder, y2); BorderMesh.Vertices[ 2].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 3].SetOrigin(x1,                 y2); BorderMesh.Vertices[ 3].SetTextureCoord(0.0f, 1.0f);

        // Top border rectangle.
        BorderMesh.Vertices[ 4].SetOrigin(x1+SelectionBorder, y1                ); BorderMesh.Vertices[ 4].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 5].SetOrigin(x2-SelectionBorder, y1                ); BorderMesh.Vertices[ 5].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 6].SetOrigin(x2-SelectionBorder, y1+SelectionBorder); BorderMesh.Vertices[ 6].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 7].SetOrigin(x1+SelectionBorder, y1+SelectionBorder); BorderMesh.Vertices[ 7].SetTextureCoord(0.0f, 1.0f);

        // Right border rectangle.
        BorderMesh.Vertices[ 8].SetOrigin(x2-SelectionBorder, y1); BorderMesh.Vertices[ 8].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 9].SetOrigin(x2,                 y1); BorderMesh.Vertices[ 9].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[10].SetOrigin(x2,                 y2); BorderMesh.Vertices[10].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[11].SetOrigin(x2-SelectionBorder, y2); BorderMesh.Vertices[11].SetTextureCoord(0.0f, 1.0f);

        // Bottom border rectangle.
        BorderMesh.Vertices[12].SetOrigin(x1+SelectionBorder, y2-SelectionBorder); BorderMesh.Vertices[12].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[13].SetOrigin(x2-SelectionBorder, y2-SelectionBorder); BorderMesh.Vertices[13].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[14].SetOrigin(x2-SelectionBorder, y2                ); BorderMesh.Vertices[14].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[15].SetOrigin(x1+SelectionBorder, y2                ); BorderMesh.Vertices[15].SetTextureCoord(0.0f, 1.0f);

        MatSys::Renderer->RenderMesh(BorderMesh);
    }
}
