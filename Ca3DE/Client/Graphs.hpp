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

/******************************/
/*** Client Graphs (Header) ***/
/******************************/

#ifndef CAFU_CLIENT_GRAPHS_HPP_INCLUDED
#define CAFU_CLIENT_GRAPHS_HPP_INCLUDED

namespace MatSys { class RenderMaterialT; }


class GraphsT
{
    public:

    float          FPS[512];        // Frames Per Second
    unsigned short Heading[512];    // Heading
    unsigned short PosY[512];
    unsigned short PosZ[512];


    // Erzeugt ein neues GraphsT-Objekt.
    GraphsT();

    ~GraphsT();

    // Trägt für das Frame 'ClientFrameNr' für alle Graphen 0 ein.
    void ClearForFrame(unsigned long ClientFrameNr);

    // Zeichnet alle Graphen.
    void Draw(unsigned long ClientFrameNr);


    private:

    GraphsT(const GraphsT&);                        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const GraphsT&);               ///< Use of the Assignment Operator is not allowed.

    MatSys::RenderMaterialT* m_RMatWireframe;       ///< The render material for wire-frame rendering.
 // MatSys::RenderMaterialT* m_RMatWireframeOZ;     ///< The render material for wire-frame rendering (with polygon z-offset, e.g. for outlines).
 // MatSys::RenderMaterialT* m_RMatFlatShaded;      ///< The render material for flat shaded (single solid color) rendering.
 // MatSys::RenderMaterialT* m_RMatFlatShadedOZ;    ///< The render material for flat shaded (single solid color) rendering (with polygon z-offset, e.g. for decals).
 // MatSys::RenderMaterialT* m_RMatOverlay;         ///< The render material for selection overlays (added in a second pass).
 // MatSys::RenderMaterialT* m_RMatOverlayOZ;       ///< The render material for selection overlays (added in a second pass) (with polygon z-offset, e.g. for decals).
};

#endif
