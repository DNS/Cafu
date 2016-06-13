/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MaterialTree.hpp"
#include "MaterialBrowserDialog.hpp"
#include "ScrolledMaterialWin.hpp"

#include "../EditorMaterial.hpp"

#include "wx/artprov.h"
#include "wx/imaglist.h"
#include "wx/wupdlock.h"


namespace
{
    ArrayT<wxString> GetMaterialNameParts(wxString MaterialName)
    {
        ArrayT<wxString> NameParts;

        int Index=MaterialName.Find('/');

        // Split material with delimiter '/' to get name parts.
        while (Index!=wxNOT_FOUND)
        {
            // Remove this part from the string and add it to our name parts.
            NameParts.PushBack(MaterialName.substr(0, Index));
            MaterialName.erase(0, Index+1);

            Index=MaterialName.Find('/');
        }

        Index=MaterialName.Len();

        // Get the last part.
        NameParts.PushBack(MaterialName.substr(0, Index));

        return NameParts;
    }


    class MaterialTreeItemT : public wxTreeItemData
    {
        public:

        MaterialTreeItemT(EditorMaterialI* Material_) : Material(Material_)
        {
        }

        EditorMaterialI* Material;
    };
}


using namespace MaterialBrowser;


BEGIN_EVENT_TABLE(MaterialTreeT, wxTreeCtrl)
    EVT_TREE_SEL_CHANGED(wxID_ANY, MaterialTreeT::OnSelectionChanged)
END_EVENT_TABLE()


MaterialTreeT::MaterialTreeT(DialogT* Parent, const ArrayT<EditorMaterialI*>& Materials)
    : wxTreeCtrl(Parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxSUNKEN_BORDER),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    // Build list of material tree icons.
    wxImageList* TreeIcons=new wxImageList(16, 16);

    TreeIcons->Add(wxArtProvider::GetBitmap("folder", wxART_MENU));
    TreeIcons->Add(wxArtProvider::GetBitmap("image-x-generic", wxART_MENU));

    AssignImageList(TreeIcons); // Note: wxTreeCtrl takes ownership of this list and deletes it on window destroy.

    wxTreeItemId RootItem=AddRoot("All", 0);

    for (unsigned long MatNr=0; MatNr<Materials.Size(); MatNr++)
    {
        ArrayT<wxString> NameParts=GetMaterialNameParts(Materials[MatNr]->GetName());

        wxTreeItemId ParentItem=RootItem;

        // Now consider the first parts categories and the last part a unique material name
        // and add everything to the tree.
        for (unsigned long PartNr=0; PartNr<NameParts.Size(); PartNr++)
        {
            if (PartNr==NameParts.Size()-1)
            {
                // Add to the parent tree node for this material.
                ParentItem=AppendItem(ParentItem, NameParts[PartNr], 1, -1, new MaterialTreeItemT(Materials[MatNr]));
            }
            else
            {
                // Look for sub folder with this name.
                wxTreeItemId SubFolder=GetChildByName(ParentItem, NameParts[PartNr], true, false);

                // Create sub folder if not yet existent.
                if (!SubFolder.IsOk())
                    SubFolder=AppendItem(ParentItem, NameParts[PartNr], 0);

                ParentItem=SubFolder;
            }
        }
    }

    Expand(RootItem);
}


void MaterialTreeT::SelectMaterial(const EditorMaterialI* Material)
{
    if (m_IsRecursiveSelfNotify) return;

    if (!Material)
    {
        // I'm not quite sure if we need the m_IsRecursiveSelfNotify brace here...
        m_IsRecursiveSelfNotify=true;
        UnselectAll();
        m_IsRecursiveSelfNotify=false;
        return;
    }

    wxTreeItemId MaterialItem=GetItemByMaterial(Material);

    if (MaterialItem.IsOk())
    {
        m_IsRecursiveSelfNotify=true;
        SelectItem(MaterialItem);
        m_IsRecursiveSelfNotify=false;
    }
}


wxTreeItemId MaterialTreeT::GetChildByName(const wxTreeItemId& Parent, const wxString& Name, bool OnlyCategories, bool Recursive)
{
    // If the parent is invalid, return it so the result of this function call is invalid too.
    if (!Parent.IsOk()) return Parent;

    wxTreeItemIdValue Cookie; // This cookie is needed for iteration over a trees children.
    wxTreeItemId      Child;

    // Check all children.
    for (Child=GetFirstChild(Parent, Cookie); Child.IsOk(); Child=GetNextChild(Parent, Cookie))
    {
        if (GetItemText(Child)==Name && (OnlyCategories ? GetItemData(Child)==NULL : true)) return Child; // Found a child with this name.

        if (Recursive)
        {
            Child=GetChildByName(Child, Name, OnlyCategories, Recursive);

            if (Child.IsOk()) return Child; // Found a grand-child with this name.
        }
    }

    wxASSERT(!Child.IsOk());

    return Child; // No match return the invalid result.
}


wxTreeItemId MaterialTreeT::GetItemByMaterial(const EditorMaterialI* Material)
{
    if (!Material) return wxTreeItemId();   // Return an "invalid" wxTreeItemId instance (IsOk()==false).

    ArrayT<wxString> NameParts=GetMaterialNameParts(Material->GetName());

    wxTreeItemId Parent=GetRootItem();

    for (unsigned long PartNr=0; PartNr<NameParts.Size(); PartNr++)
    {
        if (PartNr==NameParts.Size()-1)
        {
            return GetChildByName(Parent, NameParts[PartNr], false, false);
        }
        else
        {
            Parent=GetChildByName(Parent, NameParts[PartNr], false, false);

            if (!Parent.IsOk()) return Parent;
        }
    }

    return wxTreeItemId();  // Return an "invalid" wxTreeItemId instance (IsOk()==false).
}


void MaterialTreeT::OnSelectionChanged(wxTreeEvent& TE)
{
    // Necessary because wxTreeCtrl::SetItem() triggers a selection event when called from
    // SelectMaterial().
    if (m_IsRecursiveSelfNotify) return;

    MaterialTreeItemT* Data=dynamic_cast<MaterialTreeItemT*>(GetItemData(TE.GetItem()));

    // Use the item as folder if it is a folder or the parent item if it is a material.
    wxTreeItemId FolderItem=Data ? GetItemParent(TE.GetItem()) : TE.GetItem();

    // Set folder as filter.
    wxString MatFolderFilter="";

    while (FolderItem!=GetRootItem())
    {
        MatFolderFilter.Prepend(GetItemText(FolderItem)+"/");
        FolderItem=GetItemParent(FolderItem);
    }

    m_Parent->MatFolderFilter=MatFolderFilter;
    m_Parent->m_ScrolledMatWin->UpdateVirtualSize();

    if (Data)
    {
        m_IsRecursiveSelfNotify=true;
        m_Parent->SelectMaterial(Data->Material);
        m_IsRecursiveSelfNotify=false;
    }
    else
    {
        m_Parent->m_ScrolledMatWin->Refresh();
    }
}
