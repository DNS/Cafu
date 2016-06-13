/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogTerrainEdit.hpp"
#include "MapDocument.hpp"
#include "ToolTerrainEdit.hpp"
#include "DialogTerrainGeneration.hpp"

#include "../GameConfig.hpp"

#include "wx/filedlg.h"


TerrainEditorDialogT::TerrainEditorDialogT(wxWindow* Parent, const GameConfigT& GameConfig, ToolTerrainEditorT* ParentTool)
    : TerrainEditorDialogFB(Parent),
      m_GameConfig(GameConfig),
      m_ParentTool(ParentTool),
      m_TerrainGenerationDialog(new TerrainGenerationDialogT(this))
{
    m_ToolBar->ToggleTool(ID_TOOL_RAISE, true);

    // Notify the parent tool that this is its dialog.
    m_ParentTool->SetToolDialog(this);
}


void TerrainEditorDialogT::UpdateResolution(unsigned long Resolution)
{
    int ResSelection=m_ChoiceResolution->FindString(wxString::Format("%lux%lu", Resolution, Resolution), true);

    if (ResSelection==wxNOT_FOUND)
        ResSelection=m_ChoiceResolution->Append(wxString::Format("%lux%lu", Resolution, Resolution));

    m_ChoiceResolution->Select(ResSelection);
}


int TerrainEditorDialogT::GetActiveTool() const
{
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_RAISE  )) return ID_TOOL_RAISE;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_LOWER  )) return ID_TOOL_LOWER;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_FLATTEN)) return ID_TOOL_FLATTEN;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_FILL   )) return ID_TOOL_FILL;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_ABLATE )) return ID_TOOL_ABLATE;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_BLUR   )) return ID_TOOL_BLUR;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_SHARPEN)) return ID_TOOL_SHARPEN;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_NOISE  )) return ID_TOOL_NOISE;
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_ROAD   )) return ID_TOOL_ROAD;

    return -1;
}


bool TerrainEditorDialogT::OnMMouseUp()
{
    wxCommandEvent Event;

    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_RAISE))
    {
        Event.SetId(ID_TOOL_LOWER);
        OnToolClicked(Event);
        return true;
    }
    if (m_ToolBar->GetToolState(TerrainEditorDialogT::ID_TOOL_LOWER))
    {
        Event.SetId(ID_TOOL_RAISE);
        OnToolClicked(Event);
        return true;
    }

    return false;
}


void TerrainEditorDialogT::OnToolClicked(wxCommandEvent& event)
{
    // Reactivate "Tool effect" controls in case they were disabled.
    m_SpinCtrlToolEffect->Enable();
    m_SliderToolEffect->Enable();

    m_ToolBar->ToggleTool(ID_TOOL_RAISE,   false);
    m_ToolBar->ToggleTool(ID_TOOL_LOWER,   false);
    m_ToolBar->ToggleTool(ID_TOOL_FLATTEN, false);
    m_ToolBar->ToggleTool(ID_TOOL_FILL,    false);
    m_ToolBar->ToggleTool(ID_TOOL_ABLATE,  false);
    m_ToolBar->ToggleTool(ID_TOOL_BLUR,    false);
    m_ToolBar->ToggleTool(ID_TOOL_SHARPEN, false);
    m_ToolBar->ToggleTool(ID_TOOL_NOISE,   false);
    m_ToolBar->ToggleTool(ID_TOOL_ROAD,   false);

    m_ToolBar->ToggleTool(event.GetId(), true);

    // Disable "Tool effect" controls for tools that don't use it.
    if (event.GetId()==ID_TOOL_FLATTEN || event.GetId()==ID_TOOL_FILL || event.GetId()==ID_TOOL_ABLATE || event.GetId()==ID_TOOL_ROAD)
    {
        m_SpinCtrlToolEffect->Enable(false);
        m_SliderToolEffect->Enable(false);
    }
}


void TerrainEditorDialogT::OnSpinCtrlRadius(wxSpinEvent& event)
{
    m_SliderRadius->SetValue(event.GetPosition());

    m_ParentTool->UpdateModifyWeights();
    m_ParentTool->UpdateNoiseWeights ();
}


void TerrainEditorDialogT::OnSliderScrollRadius(wxScrollEvent& event)
{
    m_SpinCtrlRadius->SetValue(event.GetPosition());

    m_ParentTool->UpdateModifyWeights();
    m_ParentTool->UpdateNoiseWeights ();
}


void TerrainEditorDialogT::OnSpinCtrlHardness(wxSpinEvent& event)
{
    m_SliderHardness->SetValue(event.GetPosition());

    m_ParentTool->UpdateModifyWeights();
}


void TerrainEditorDialogT::OnSliderScrollHardness(wxScrollEvent& event)
{
    m_SpinCtrlHardness->SetValue(event.GetPosition());

    m_ParentTool->UpdateModifyWeights();
}


void TerrainEditorDialogT::OnSpinCtrlToolEffect(wxSpinEvent& event)
{
    m_SliderToolEffect->SetValue(event.GetPosition());

    m_ParentTool->UpdateGaussWeights();
}


void TerrainEditorDialogT::OnSliderScrollToolEffect(wxScrollEvent& event)
{
    m_SpinCtrlToolEffect->SetValue(event.GetPosition());

    m_ParentTool->UpdateGaussWeights();
}


void TerrainEditorDialogT::OnChoiceResolution(wxCommandEvent& event)
{
    wxASSERT(m_ChoiceResolution->GetSelection()!=wxNOT_FOUND);

    // First translate choice selection into resolution.
    unsigned long ChoiceResolution=(1<<(m_ChoiceResolution->GetSelection()+6))+1;

    m_ParentTool->SetResolution(ChoiceResolution);
}


void TerrainEditorDialogT::OnButtonImport(wxCommandEvent& event)
{
    if (!m_ParentTool->IsTerrainSelected())
    {
        wxMessageBox("You can only import height data into a selected terrain.", "No terrain selected");
        return;
    }

    wxString HeightmapNameStr=wxFileSelector("Select a heightmap image", m_GameConfig.ModDir+"/Terrains/", "", "", "All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (HeightmapNameStr!="")
        m_ParentTool->ImportHeightMap(HeightmapNameStr);
}


void TerrainEditorDialogT::OnButtonExport(wxCommandEvent& event)
{
    if (!m_ParentTool->IsTerrainSelected())
    {
        wxMessageBox("You can only export height data if a terrain is selected.", "No terrain selected");
        return;
    }

    wxFileDialog ExportDialog(this,
                              "Export height data",
                              m_GameConfig.ModDir+"/Terrains",
                              "",
                              "Bitmap (*.bmp)|*.bmp"
                              "|Portable Network Graphic (*.png)|*.png"
                              "|JPEG (*.jpg)|*.jpg"
                              "|Portable Graymap ASCII (*.pgm)|*.pgm"
                              "|Portable Graymap binary (*.pgm)|*.pgm"
                              "|Terragen file (*.ter)|*.ter",
                              wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (ExportDialog.ShowModal()!=wxID_OK) return;

    wxString FileName=ExportDialog.GetFilename();
    ToolTerrainEditorT::ExportFileTypeE ExportFileType=ToolTerrainEditorT::BMP;

    // Since the user can choose the filetype to be exported either by entering a filename without extension and choosing a correct filter or by entering a
    // filename with extension (in which case the filter setting is ignored) we need to derive the correct file type here from the extension.
    if (FileName.EndsWith(".bmp"))
    {
       ExportFileType=ToolTerrainEditorT::BMP;
    }
    else if (FileName.EndsWith(".png"))
    {
       ExportFileType=ToolTerrainEditorT::PNG;
    }
    else if (FileName.EndsWith(".jpg"))
    {
       ExportFileType=ToolTerrainEditorT::JPG;
    }
    else if (FileName.EndsWith(".jpeg"))
    {
       ExportFileType=ToolTerrainEditorT::JPG;
    }
    else if (FileName.EndsWith(".pgm"))
    {
       switch (ExportDialog.GetFilterIndex())
       {
           case 3: //ASCII
               ExportFileType=ToolTerrainEditorT::PGM_ASCII;
               break;
           case 4: //BINARY
               ExportFileType=ToolTerrainEditorT::PGM_BINARY;
               break;
           default:
               ExportFileType=ToolTerrainEditorT::PGM_ASCII;
               break;
       }
    }
    else if (FileName.EndsWith(".ter"))
    {
       ExportFileType=ToolTerrainEditorT::TER;
    }
    else
    {
       // Ignore file extension and set the file type according to filter setting.
       switch (ExportDialog.GetFilterIndex())
       {
           case 0:
               ExportFileType=ToolTerrainEditorT::BMP;
               break;
           case 1:
               ExportFileType=ToolTerrainEditorT::PNG;
               break;
           case 2:
               ExportFileType=ToolTerrainEditorT::JPG;
               break;
           case 3:
               ExportFileType=ToolTerrainEditorT::PGM_ASCII;
               break;
           case 4:
               ExportFileType=ToolTerrainEditorT::PGM_BINARY;
               break;
           case 5:
               ExportFileType=ToolTerrainEditorT::TER;
               break;
       }
    }

    m_ParentTool->ExportHeightMap(ExportDialog.GetPath(), ExportFileType);
}


void TerrainEditorDialogT::OnButtonGenerate(wxCommandEvent& event)
{
    if (!m_ParentTool->IsTerrainSelected())
    {
        wxMessageBox("You can only generate height data for a selected terrain.", "No terrain selected");
        return;
    }

    if (m_TerrainGenerationDialog->ShowModal(m_ParentTool->GetResolution())!=wxID_OK) return;

    m_ParentTool->GenerateTerrain(m_TerrainGenerationDialog->m_SpinCtrlOctaves    ->GetValue(),
                                  m_TerrainGenerationDialog->m_SpinCtrlFrequency  ->GetValue(),
                                  m_TerrainGenerationDialog->m_SpinCtrlPersistence->GetValue(),
                                  m_TerrainGenerationDialog->m_SpinCtrlLacunarity ->GetValue(),
                                  m_TerrainGenerationDialog->m_SpinCtrlSeed       ->GetValue());
}
