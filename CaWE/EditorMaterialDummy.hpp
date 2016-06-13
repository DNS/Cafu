/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_EDITOR_MATERIAL_DUMMY_HPP_INCLUDED
#define CAFU_EDITOR_MATERIAL_DUMMY_HPP_INCLUDED

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
