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

#ifndef _MATERIAL_TREE_HPP_
#define _MATERIAL_TREE_HPP_

#include "wx/treectrl.h"

#include "Templates/Array.hpp"


class EditorMaterialI;
class MaterialBrowserDialogT;


class MaterialTreeT : public wxTreeCtrl
{
    public:

    MaterialTreeT(MaterialBrowserDialogT* Parent, const ArrayT<EditorMaterialI*>& Materials);

    void SelectMaterial(const EditorMaterialI* Material);
    void ShowEditorOnly(bool EditorOnly=true);


    private:

    MaterialBrowserDialogT* m_Parent;

    bool m_IsRecursiveSelfNotify;

    // Helper methods.
    wxTreeItemId GetChildByName(const wxTreeItemId& Parent, const wxString& Name, bool OnlyCategories, bool Recursive);
    wxTreeItemId GetItemByMaterial(const EditorMaterialI* Material);

    void OnSelectionChanged(wxTreeEvent& TE);

    DECLARE_EVENT_TABLE()
};

#endif
