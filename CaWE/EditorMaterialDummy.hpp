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

#ifndef _EDITOR_MATERIAL_DUMMY_HPP_
#define _EDITOR_MATERIAL_DUMMY_HPP_

#include "wx/image.h"
#include "EditorMaterial.hpp"


/// The class implements the EditorMaterialI interface.
/// It is used whenever a map file contains a material name for which
/// we have no matching material in the MaterialSystems material manager.
class DummyMaterialT : public EditorMaterialI
{
    public:

    DummyMaterialT(const wxString& Name);

    // Instead of 0, return reasonable numbers for the width and height, so that the dummy is properly displayed in the material browser.
    int GetWidth () const { return 256; }
    int GetHeight() const { return  64; }

    const wxString& GetName() const { return m_Name; }

    void Draw(wxDC& dc, const wxRect& DestRect, int NameBoxHeight, bool DrawNameBox) const;
    const wxImage& GetImage() const { static wxImage DummyImage(2, 2); return DummyImage; }

    MatSys::RenderMaterialT* GetRenderMaterial(bool PreviewMode) const;
    MaterialT* GetMaterial() const;
    bool IsTranslucent() const { return false; }
    bool ShowInMaterialBrowser() const { return true; }


    protected:

    wxString m_Name;
};

#endif
