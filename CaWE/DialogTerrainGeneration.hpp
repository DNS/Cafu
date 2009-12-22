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

#ifndef _DIALOG_TERRAIN_GENERATION_HPP_
#define _DIALOG_TERRAIN_GENERATION_HPP_

#include "wxFB/TerrainEditorDialogs.h"


class TerrainGenerationDialogT : public TerrainGenerationDialogFB
{
    public:

    TerrainGenerationDialogT(wxWindow* Parent);

    int ShowModal(unsigned long TerrainResolution);


    protected:

    // TerrainGenerationDialogFB implementation.
    void OnSpinCtrlFrequency  (wxSpinEvent&   event);
    void OnSliderFrequency    (wxScrollEvent& event);
    void OnSpinCtrlOctaves    (wxSpinEvent&   event);
    void OnSliderOctaves      (wxScrollEvent& event);
    void OnSpinCtrlPersistence(wxSpinEvent&   event);
    void OnSliderPersistence  (wxScrollEvent& event);
    void OnSpinCtrlLacunarity (wxSpinEvent&   event);
    void OnSliderLacunarity   (wxScrollEvent& event);
    void OnSpinCtrlSeed       (wxSpinEvent&   event);


    private:

    const unsigned long m_PreviewResolution; ///< Resolution of the preview window.
          unsigned long m_TerrainResolution; ///< Resolution of the terrain that is generated.

    void UpdatePreview();
};

#endif
