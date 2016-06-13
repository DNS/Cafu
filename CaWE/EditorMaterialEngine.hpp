/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_EDITOR_MATERIAL_ENGINE_HPP_INCLUDED
#define CAFU_EDITOR_MATERIAL_ENGINE_HPP_INCLUDED

#include "wx/image.h"
#include "EditorMaterial.hpp"


/// This class implements the EditorMaterialI for materials that we
/// have a material for in the MaterialSystems material manager.
class EngineMaterialT : public EditorMaterialI
{
    public:

    EngineMaterialT(MaterialT* MatSysMaterial_);
    ~EngineMaterialT();

    int GetWidth() const;
    int GetHeight() const;

    const wxString& GetName() const { return Name; }

    void Draw(wxDC& dc, const wxRect& DestRect, int NameBoxHeight, bool DrawNameBox) const;
    const wxImage& GetImage() const;

    MatSys::RenderMaterialT* GetRenderMaterial(bool PreviewMode) const;
    MaterialT* GetMaterial() const { return MatSysMaterial; }
    bool IsTranslucent() const;
    bool ShowInMaterialBrowser() const;


    private:

    const wxString                   Name;
    MaterialT*                       MatSysMaterial;
    mutable MatSys::RenderMaterialT* MatSysRenderMaterial_Normal;
    mutable MatSys::RenderMaterialT* MatSysRenderMaterial_Editor;
    mutable MaterialT*               Material_Editor;               ///< The material with which the MatSysRenderMaterial_Editor is built.
    mutable wxImage*                 m_BrowserImage;
};

#endif
