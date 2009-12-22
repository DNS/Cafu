/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _EDITOR_MATERIAL_ENGINE_HPP_
#define _EDITOR_MATERIAL_ENGINE_HPP_

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
