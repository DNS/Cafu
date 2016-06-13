/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_TERRAIN_GENERATION_HPP_INCLUDED
#define CAFU_DIALOG_TERRAIN_GENERATION_HPP_INCLUDED

#include "Templates/Array.hpp"
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
