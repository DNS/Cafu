/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "DocumentAdapter.hpp"
#include "DialogReplaceMaterials.hpp"
#include "EditorMaterial.hpp"
#include "GameConfig.hpp"
#include "MapDocument.hpp"
#include "GuiEditor/GuiDocument.hpp"
#include "MapCommands/ReplaceMat.hpp"
#include "ModelEditor/ModelDocument.hpp"

#include "GuiSys/CompImage.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"


/**********************/
/*** MapDocAdapterT ***/
/**********************/

MapDocAdapterT::MapDocAdapterT(MapDocumentT& MapDoc)
    : m_MapDoc(MapDoc)
{
}


const ArrayT<EditorMaterialI*>& MapDocAdapterT::GetMaterials() const
{
    return m_MapDoc.GetGameConfig()->GetMatMan().GetMaterials();
}


void MapDocAdapterT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
{
    m_MapDoc.GetUsedMaterials(UsedMaterials);
}


void MapDocAdapterT::OnMarkMaterial(EditorMaterialI* Mat) const
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


void MapDocAdapterT::OnReplaceMaterial(EditorMaterialI* Mat) const
{
    ReplaceMaterialsDialogT ReplaceMatsDlg(m_MapDoc.GetSelection().Size()>0, m_MapDoc, Mat->GetName());

    ReplaceMatsDlg.ShowModal();
}


void MapDocAdapterT::UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var)
{
    m_MapDoc.UpdateAllObservers_VarChanged(Var);
}


/**********************/
/*** GuiDocAdapterT ***/
/**********************/

GuiDocAdapterT::GuiDocAdapterT(GuiEditor::GuiDocumentT& GuiDoc)
    : m_GuiDoc(GuiDoc)
{
}


const ArrayT<EditorMaterialI*>& GuiDocAdapterT::GetMaterials() const
{
    return m_GuiDoc.GetEditorMaterials();
}


void GuiDocAdapterT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
{
    UsedMaterials.Overwrite();

    const ArrayT<EditorMaterialI*>&              EditorMaterials=m_GuiDoc.GetEditorMaterials();
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > GuiWindows;

    GuiWindows.PushBack(m_GuiDoc.GetRootWindow());
    m_GuiDoc.GetRootWindow()->GetChildren(GuiWindows, true);

    for (unsigned long WinNr=0; WinNr<GuiWindows.Size(); WinNr++)
    {
        IntrusivePtrT<cf::GuiSys::ComponentImageT> CompImage = dynamic_pointer_cast<cf::GuiSys::ComponentImageT>(GuiWindows[WinNr]->GetComponent("Image"));

        if (CompImage == NULL) continue;

        for (unsigned long EMNr=0; EMNr<EditorMaterials.Size(); EMNr++)
        {
            EditorMaterialI* EM = EditorMaterials[EMNr];

            if (EM->GetMaterial()->Name == CompImage->GetMatName())
            {
                if (UsedMaterials.Find(EM)==-1)
                    UsedMaterials.PushBack(EM);
                break;
            }
        }
    }
}


void GuiDocAdapterT::OnMarkMaterial(EditorMaterialI* Mat) const
{
}


void GuiDocAdapterT::OnReplaceMaterial(EditorMaterialI* Mat) const
{
}


void GuiDocAdapterT::UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var)
{
    m_GuiDoc.UpdateAllObservers_Modified(Var);
}


/************************/
/*** ModelDocAdapterT ***/
/************************/

ModelDocAdapterT::ModelDocAdapterT(ModelEditor::ModelDocumentT& ModelDoc)
    : m_ModelDoc(ModelDoc)
{
}


const ArrayT<EditorMaterialI*>& ModelDocAdapterT::GetMaterials() const
{
    return m_ModelDoc.GetEditorMaterials();
}


void ModelDocAdapterT::GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const
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


void ModelDocAdapterT::OnMarkMaterial(EditorMaterialI* Mat) const
{
}


void ModelDocAdapterT::OnReplaceMaterial(EditorMaterialI* Mat) const
{
}


void ModelDocAdapterT::UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var)
{
}
