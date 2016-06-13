/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATERIAL_TREE_HPP_INCLUDED
#define CAFU_MATERIAL_TREE_HPP_INCLUDED

#include "wx/treectrl.h"

#include "Templates/Array.hpp"


class EditorMaterialI;


namespace MaterialBrowser
{
    class DialogT;


    class MaterialTreeT : public wxTreeCtrl
    {
        public:

        MaterialTreeT(DialogT* Parent, const ArrayT<EditorMaterialI*>& Materials);

        void SelectMaterial(const EditorMaterialI* Material);
        void ShowEditorOnly(bool EditorOnly=true);


        private:

        DialogT* m_Parent;

        bool m_IsRecursiveSelfNotify;

        // Helper methods.
        wxTreeItemId GetChildByName(const wxTreeItemId& Parent, const wxString& Name, bool OnlyCategories, bool Recursive);
        wxTreeItemId GetItemByMaterial(const EditorMaterialI* Material);

        void OnSelectionChanged(wxTreeEvent& TE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
