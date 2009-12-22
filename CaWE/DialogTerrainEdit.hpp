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

#ifndef _DIALOG_TERRAIN_EDIT_HPP_
#define _DIALOG_TERRAIN_EDIT_HPP_

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
