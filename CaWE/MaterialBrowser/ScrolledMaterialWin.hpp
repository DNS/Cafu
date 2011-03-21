/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _SCROLLED_MATERIAL_WIN_HPP_
#define _SCROLLED_MATERIAL_WIN_HPP_

#include "Templates/Array.hpp"

#include "wx/scrolwin.h"


class MaterialBrowserDialogT;
class EditorMaterialI;


class ScrolledMaterialWindowT : public wxScrolledWindow
{
    public:

    // Constructor.
    ScrolledMaterialWindowT(MaterialBrowserDialogT* Parent, wxWindowID OurID, const ArrayT<EditorMaterialI*>& Materials);

    // Recomputes the size of the virtual window.
    void UpdateVirtualSize();

    // If NewMat==NULL, this function does nothing.
    // Otherwise NewMat is selected as the new current material, the material description text is updated,
    // the window scrolled to the selected material, and finally the window is redrawn.
    void SelectMaterial(EditorMaterialI* NewMat);

    /// A helper function exclusively for MaterialBrowserDialogT::OnButton_ExportDiffMaps().
    void ExportDiffuseMaps(const wxString& DestinationDir) const;


    private:

    struct TexPosEnumT
    {
        unsigned long    Index;         // Current index number of this material position.
        EditorMaterialI* Mat;           // Pointer to material.
        wxRect           MatRect;       // The coordinates of this material position in the virtual window.
        int              CurrentX;      // The current position. The next material position will be here.
        int              CurrentY;
        int              LargestY;      // The largest y-coordinate encountered so far. Needed to know where the next row starts.
        wxSize           ClientSize;    // The size of the client area (the visible fraction of the virtual window).
    };


    static const int SCROLL_INCREMENT_X;
    static const int SCROLL_INCREMENT_Y;
    static const int Padding;
    static const int MaterialNameBoxHeight;

    MaterialBrowserDialogT*         m_Parent;
    const ArrayT<EditorMaterialI*>& m_Materials;


    TexPosEnumT* EnumTexturePositions(bool Start) const;

    void OnDraw(wxDC& dc);                              // Overridden event handler.
    void OnSize(wxSizeEvent& Event);                    // My own event handler for resize events.
    void OnLeftButtonDown(wxMouseEvent& Event);         // Left mouse button click handler.
    void OnLeftButtonDoubleClick(wxMouseEvent& Event);  // Left mouse button double click handler.

    DECLARE_EVENT_TABLE()
};

#endif
