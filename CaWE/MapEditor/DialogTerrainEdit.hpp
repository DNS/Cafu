/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_TERRAIN_EDIT_HPP_INCLUDED
#define CAFU_DIALOG_TERRAIN_EDIT_HPP_INCLUDED

#include "wxFB/TerrainEditorDialogs.h"
#include "Math3D/BoundingBox.hpp"


class GameConfigT;
class ToolTerrainEditorT;
class TerrainGenerationDialogT;


class TerrainEditorDialogT : public TerrainEditorDialogFB
{
    public:

    /// Constructor.
    TerrainEditorDialogT(wxWindow* Parent, const GameConfigT& GameConfig, ToolTerrainEditorT* ParentTool);

    /// Updates the dialogs resolution combobox.
    /// @param Resolution New resolution value.
    void UpdateResolution(unsigned long Resolution);

    /// Returns the the currently active tool.
    /// @return ID of the currently active tool.
    int GetActiveTool() const;

    /// Gets the current radius value.
    int GetRadius() const { return m_SpinCtrlRadius->GetValue(); }

    /// Gets the current hardness value.
    int GetHardness() const { return m_SpinCtrlHardness->GetValue(); }

    /// Gets the current tool effect value.
    int GetToolEffect() const { return m_SpinCtrlToolEffect->GetValue(); }

    /// React on middle mouse clicks from terrain editor tool.
    /// @return Whether the middle mouse click has been handled by the dialog or not.
    bool OnMMouseUp();


    protected:

    void OnToolClicked           (wxCommandEvent& event);
    void OnSpinCtrlRadius        (wxSpinEvent&    event);
    void OnSliderScrollRadius    (wxScrollEvent&  event);
    void OnSpinCtrlHardness      (wxSpinEvent&    event);
    void OnSliderScrollHardness  (wxScrollEvent&  event);
    void OnSpinCtrlToolEffect    (wxSpinEvent&    event);
    void OnSliderScrollToolEffect(wxScrollEvent&  event);
    void OnChoiceResolution      (wxCommandEvent& event);
    void OnButtonImport          (wxCommandEvent& event);
    void OnButtonExport          (wxCommandEvent& event);
    void OnButtonGenerate        (wxCommandEvent& event);


    private:

    friend class ToolTerrainEditorT;

    const GameConfigT&  m_GameConfig;
    ToolTerrainEditorT* m_ParentTool;   ///< The terrain editor tool related to this dialog.

    TerrainGenerationDialogT* m_TerrainGenerationDialog;
};

#endif
