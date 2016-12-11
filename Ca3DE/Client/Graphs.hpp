/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

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
    void Draw(unsigned long ClientFrameNr, unsigned int fbWidth, unsigned int fbHeight) const;


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
