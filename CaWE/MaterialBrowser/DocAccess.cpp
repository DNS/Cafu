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

#include "DocAccess.hpp"
#include "../DialogReplaceMaterials.hpp"
#include "../EditorMaterial.hpp"
#include "../GameConfig.hpp"
#include "../MapDocument.hpp"
#include "../GuiEditor/GuiDocument.hpp"
#include "../MapCommands/ReplaceMat.hpp"
#include "../ModelEditor/ModelDocument.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"


/*********************/
/*** MapDocAccessT ***/
/*********************/

MaterialBrowser::MapDocAccessT::MapDocAccessT(MapDocumentT& MapDoc)
    : m_MapDoc(MapDoc)
{
}


const ArrayT<EditorMaterialI*>& MaterialBrowser::MapDocAccessT::GetMaterials() const
{
    return m_MapDoc.GetGameConfig()->GetMatMan().GetMaterials();
}


void MaterialBrowser::MapDocAccessT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
{
    m_MapDoc.GetUsedMaterials(UsedMaterials);
}


void MaterialBrowser::MapDocAccessT::OnMarkMaterial(EditorMaterialI* Mat) const
{
    CommandReplaceMatT* Command=new CommandReplaceMatT(
        m_MapDoc,
        m_MapDoc.GetSelection(),
        Mat->GetName().c_str(),
        "",
        CommandReplaceMatT::ExactMatches,
        true,   // Only mark found faces/brushes/patches.
        false,  // Search whole world, not only in selection.
        true,   // Include brushes in the search.
        true,   // Include bezier patches in the search.
        false); // Do not include hidden objects (makes no sense for marking anyway).

    m_MapDoc.GetHistory().SubmitCommand(Command);
    wxMessageBox(Command->GetResultString());
}


void MaterialBrowser::MapDocAccessT::OnReplaceMaterial(EditorMaterialI* Mat) const
{
    ReplaceMaterialsDialogT ReplaceMatsDlg(m_MapDoc.GetSelection().Size()>0, m_MapDoc, Mat->GetName());

    ReplaceMatsDlg.ShowModal();
}


/*********************/
/*** GuiDocAccessT ***/
/*********************/

MaterialBrowser::GuiDocAccessT::GuiDocAccessT(GuiEditor::GuiDocumentT& GuiDoc)
    : m_GuiDoc(GuiDoc)
{
}


const ArrayT<EditorMaterialI*>& MaterialBrowser::GuiDocAccessT::GetMaterials() const
{
    return m_GuiDoc.GetEditorMaterials();
}


void MaterialBrowser::GuiDocAccessT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
{
    UsedMaterials.Overwrite();

    const ArrayT<EditorMaterialI*>&              EditorMaterials=m_GuiDoc.GetEditorMaterials();
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > GuiWindows;

    GuiWindows.PushBack(m_GuiDoc.GetRootWindow());
    m_GuiDoc.GetRootWindow()->GetChildren(GuiWindows, true);

    for (unsigned long WinNr=0; WinNr<GuiWindows.Size(); WinNr++)
    {
        for (unsigned long EMNr=0; EMNr<EditorMaterials.Size(); EMNr++)
        {
            EditorMaterialI* EM=EditorMaterials[EMNr];

            if (EM->GetMaterial()->Name == GuiWindows[WinNr]->BackRenderMatName)
            {
                if (UsedMaterials.Find(EM)==-1)
                    UsedMaterials.PushBack(EM);
                break;
            }
        }
    }
}


void MaterialBrowser::GuiDocAccessT::OnMarkMaterial(EditorMaterialI* Mat) const
{
}


void MaterialBrowser::GuiDocAccessT::OnReplaceMaterial(EditorMaterialI* Mat) const
{
}


/***********************/
/*** ModelDocAccessT ***/
/***********************/

MaterialBrowser::ModelDocAccessT::ModelDocAccessT(ModelEditor::ModelDocumentT& ModelDoc)
    : m_ModelDoc(ModelDoc)
{
}


const ArrayT<EditorMaterialI*>& MaterialBrowser::ModelDocAccessT::GetMaterials() const
{
    return m_ModelDoc.GetEditorMaterials();
}


void MaterialBrowser::ModelDocAccessT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
{
    UsedMaterials.Overwrite();

    const ArrayT<CafuModelT::MeshT>& Meshes         =m_ModelDoc.GetModel()->GetMeshes();
    const ArrayT<EditorMaterialI*>&  EditorMaterials=m_ModelDoc.GetEditorMaterials();

    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        for (unsigned long EMNr=0; EMNr<EditorMaterials.Size(); EMNr++)
        {
            EditorMaterialI* EM=EditorMaterials[EMNr];

            if (EM->GetMaterial() == Meshes[MeshNr].Material)
            {
                if (UsedMaterials.Find(EM)==-1)
                    UsedMaterials.PushBack(EM);
                break;
            }
        }
    }
}


void MaterialBrowser::ModelDocAccessT::OnMarkMaterial(EditorMaterialI* Mat) const
{
}


void MaterialBrowser::ModelDocAccessT::OnReplaceMaterial(EditorMaterialI* Mat) const
{
}
