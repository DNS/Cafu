/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogTerrainGeneration.hpp"

#include "wx/dcclient.h"
#include "wx/settings.h"
#include "wx/rawbmp.h"

#include "noise.h"


TerrainGenerationDialogT::TerrainGenerationDialogT(wxWindow* Parent)
    : TerrainGenerationDialogFB(Parent),
      m_PreviewResolution(192),
      m_TerrainResolution(65)
{
}


int TerrainGenerationDialogT::ShowModal(unsigned long TerrainResolution)
{
    m_TerrainResolution=TerrainResolution;

    Show(true);
    UpdatePreview();

    return TerrainGenerationDialogFB::ShowModal();
}


void TerrainGenerationDialogT::OnSpinCtrlFrequency(wxSpinEvent& event)
{
    m_SliderFrequency->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSliderFrequency(wxScrollEvent& event)
{
    m_SpinCtrlFrequency->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSpinCtrlOctaves(wxSpinEvent& event)
{
    m_SliderOctaves->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSliderOctaves(wxScrollEvent& event)
{
    m_SpinCtrlOctaves->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSpinCtrlPersistence(wxSpinEvent& event)
{
    m_SliderPersistence->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSliderPersistence(wxScrollEvent& event)
{
    m_SpinCtrlPersistence->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSpinCtrlLacunarity(wxSpinEvent& event)
{
    m_SliderLacunarity->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSliderLacunarity(wxScrollEvent& event)
{
    m_SpinCtrlLacunarity->SetValue(event.GetPosition());
    UpdatePreview();
}


void TerrainGenerationDialogT::OnSpinCtrlSeed(wxSpinEvent& event)
{
    UpdatePreview();
}


void TerrainGenerationDialogT::UpdatePreview()
{
    // Create preview bitmap and data access iterator for it.
    wxBitmap                    PreviewBitmap(m_PreviewResolution, m_PreviewResolution, 24);
    wxNativePixelData           BitmapData(PreviewBitmap);
    wxNativePixelData::Iterator It        (BitmapData);

    noise::module::Perlin PerlinModule;
    PerlinModule.SetNoiseQuality(noise::QUALITY_BEST                                       );
    PerlinModule.SetOctaveCount (m_SpinCtrlOctaves    ->GetValue()                         );
    PerlinModule.SetFrequency   (m_SpinCtrlFrequency  ->GetValue()/10.0/m_TerrainResolution);
    PerlinModule.SetPersistence (m_SpinCtrlPersistence->GetValue()/100.0                   );
    PerlinModule.SetLacunarity  (m_SpinCtrlLacunarity ->GetValue()/10.0                    );
    PerlinModule.SetSeed        (m_SpinCtrlSeed       ->GetValue()                         );

    double ScaleFactor=double(m_TerrainResolution)/double(m_PreviewResolution);

    It.Offset(BitmapData, 0, 0);

    for (unsigned long y=0; y<m_PreviewResolution; y++)
    {
        wxNativePixelData::Iterator RowStart=It;

        double YPos=y*ScaleFactor;

        for (unsigned long x=0; x<m_PreviewResolution; x++, It++)
        {
            float PerlinValue=PerlinModule.GetValue(x*ScaleFactor, YPos, 1.0);

            if (PerlinValue> 1.0f) PerlinValue= 1.0f;
            if (PerlinValue<-1.0f) PerlinValue=-1.0f;

            unsigned char ColorValue=(PerlinValue+1.0f)/2.0f*255.0f;

            It.Red()  =ColorValue;
            It.Green()=ColorValue;
            It.Blue() =ColorValue;
        }

        It=RowStart;
        It.OffsetY(BitmapData, 1);
    }

    wxClientDC PreviewDC(m_PreviewPanel);

    // Clear the background.
    PreviewDC.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE)));
    PreviewDC.Clear();

    // Draw the preview bitmap.
    PreviewDC.DrawBitmap(PreviewBitmap, 0, 0, false);
}
