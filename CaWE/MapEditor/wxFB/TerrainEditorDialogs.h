/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef CAFU_TERRAIN_EDITOR_DIALOGS_HPP_INCLUDED
#define CAFU_TERRAIN_EDITOR_DIALOGS_HPP_INCLUDED

// Turn off bogus warnings that occur with VC11's static code analysis.
// (Should move this to a better place though, e.g. some `compat.h` file...)
#if defined(_WIN32) && defined(_MSC_VER)
    // warning C6011: dereferencing NULL pointer <name>
    #pragma warning(disable:6011)
#endif

#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/toolbar.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class TerrainEditorDialogFB
///////////////////////////////////////////////////////////////////////////////
class TerrainEditorDialogFB : public wxPanel
{
	private:

	protected:
		enum
		{
			ID_TOOL_RAISE = 1000,
			ID_TOOL_LOWER,
			ID_TOOL_FLATTEN,
			ID_TOOL_FILL,
			ID_TOOL_ABLATE,
			ID_TOOL_BLUR,
			ID_TOOL_SHARPEN,
			ID_TOOL_NOISE,
			ID_TOOL_ROAD,
			ID_SPIN_CTRL_RADIUS,
			ID_SLIDER_RADIUS,
			ID_SPIN_CTRL_HARDNESS,
			ID_SLIDER_HARDNESS,
			ID_BUTTON_IMPORT,
			ID_BUTTON_EXPORT,
		};

		wxToolBar* m_ToolBar;
		wxSpinCtrl* m_SpinCtrlRadius;
		wxSlider* m_SliderRadius;
		wxSpinCtrl* m_SpinCtrlHardness;
		wxSlider* m_SliderHardness;
		wxSpinCtrl* m_SpinCtrlToolEffect;
		wxSlider* m_SliderToolEffect;
		wxChoice* m_ChoiceResolution;
		wxButton* m_ButtonImport;
		wxButton* m_ButtonExport;
		wxButton* m_ButtonGenerate;

		// Virtual event handlers, overide them in your derived class
		virtual void OnToolClicked( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnSpinCtrlRadius( wxSpinEvent& event ){ event.Skip(); }
		virtual void OnSliderScrollRadius( wxScrollEvent& event ){ event.Skip(); }
		virtual void OnSpinCtrlHardness( wxSpinEvent& event ){ event.Skip(); }
		virtual void OnSliderScrollHardness( wxScrollEvent& event ){ event.Skip(); }
		virtual void OnSpinCtrlToolEffect( wxSpinEvent& event ){ event.Skip(); }
		virtual void OnSliderScrollToolEffect( wxScrollEvent& event ){ event.Skip(); }
		virtual void OnChoiceResolution( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonImport( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonExport( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonGenerate( wxCommandEvent& event ){ event.Skip(); }


	public:
		TerrainEditorDialogFB( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL );
		~TerrainEditorDialogFB();

};

///////////////////////////////////////////////////////////////////////////////
/// Class TerrainGenerationDialogFB
///////////////////////////////////////////////////////////////////////////////
class TerrainGenerationDialogFB : public wxDialog
{
	private:

	protected:
		wxSlider* m_SliderFrequency;
		wxSlider* m_SliderOctaves;
		wxSlider* m_SliderLacunarity;
		wxSlider* m_SliderPersistence;
		wxPanel* m_PreviewPanel;
		wxButton* m_ButtonCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnSpinCtrlFrequency( wxSpinEvent& event ){ event.Skip(); }
		virtual void OnSliderFrequency( wxScrollEvent& event ){ event.Skip(); }
		virtual void OnSpinCtrlOctaves( wxSpinEvent& event ){ event.Skip(); }
		virtual void OnSliderOctaves( wxScrollEvent& event ){ event.Skip(); }
		virtual void OnSpinCtrlLacunarity( wxSpinEvent& event ){ event.Skip(); }
		virtual void OnSliderLacunarity( wxScrollEvent& event ){ event.Skip(); }
		virtual void OnSpinCtrlPersistence( wxSpinEvent& event ){ event.Skip(); }
		virtual void OnSliderPersistence( wxScrollEvent& event ){ event.Skip(); }
		virtual void OnSpinCtrlSeed( wxSpinEvent& event ){ event.Skip(); }


	public:
		wxSpinCtrl* m_SpinCtrlFrequency;
		wxSpinCtrl* m_SpinCtrlOctaves;
		wxSpinCtrl* m_SpinCtrlLacunarity;
		wxSpinCtrl* m_SpinCtrlPersistence;
		wxSpinCtrl* m_SpinCtrlSeed;
		TerrainGenerationDialogFB( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Generate Terrain"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE );
		~TerrainGenerationDialogFB();

};

#endif //__TerrainEditorDialogs__
