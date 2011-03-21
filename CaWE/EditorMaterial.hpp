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

#ifndef _EDITOR_MATERIAL_HPP_
#define _EDITOR_MATERIAL_HPP_

#include "wx/wx.h"


namespace MatSys { class RenderMaterialT; }

class MaterialT;
class wxDC;
class wxImage;
class wxRect;


class EditorMaterialI
{
    public:

    virtual ~EditorMaterialI() { }

    virtual int GetWidth() const=0;
    virtual int GetHeight() const=0;

    virtual const wxString& GetName() const=0;

    // Called to draw this texture onto dc into DestRect.
    // DestRect includes the NameBoxHeight, that is, the intended height of the texture is only DestRect.GetHeight()-NameBoxHeight.
    // The drawing of the name box (background rectangle + font) is setup by the caller.
    // On changes, make sure that the previous values for pens, brushes etc. are restored.
    virtual void Draw(wxDC& dc, const wxRect& DestRect, int NameBoxHeight, bool DrawNameBox) const=0;
    virtual const wxImage& GetImage() const=0;

    /// Returns the render material of this material.
    /// @param PreviewMode   If true, the render material for 3D preview mode is returned (as it would appear in Cafu).
    ///        If false, the render material for the 3D edit mode is returned (derived from the meta_editorImage, better for editing in CaWE).
    virtual MatSys::RenderMaterialT* GetRenderMaterial(bool PreviewMode) const=0;

    /// Returns the material object.
    virtual MaterialT* GetMaterial() const=0;

    /// Returns whether this material is rendered translucently.
    /// Translucent materials are typically implemented with "alpha blending" and require rendering in back-to-front order.
    /// Classes that derive from MapElementT typically use this to determine the return value for their MapElementT::IsTranslucent() method.
    /// @see MapElementT::IsTranslucent()
    virtual bool IsTranslucent() const=0;

    /// Returns whether the material should be shown for selection in the materials browser.
    /// Useful for letting sprite and model materials out.
    virtual bool ShowInMaterialBrowser() const=0;
};

#endif
