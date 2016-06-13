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

#include "TerrainEditorDialogs.h"

///////////////////////////////////////////////////////////////////////////

TerrainEditorDialogFB::TerrainEditorDialogFB( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	this->SetExtraStyle( wxWS_EX_BLOCK_EVENTS );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_ToolBar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTB_FLAT );
	m_ToolBar->SetToolBitmapSize( wxSize( 20,20 ) );
	m_ToolBar->SetToolSeparation( 1 );
	m_ToolBar->AddTool( ID_TOOL_RAISE, wxT("Raise"), wxBitmap( wxT("CaWE/res/TerrainEditToolRaise.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Raise"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_LOWER, wxT("Lower"), wxBitmap( wxT("CaWE/res/TerrainEditToolLower.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Lower"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_FLATTEN, wxT("Flatten"), wxBitmap( wxT("CaWE/res/TerrainEditToolFlatten.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Flatten"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_FILL, wxT("Fill"), wxBitmap( wxT("CaWE/res/TerrainEditToolFill.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Fill"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_ABLATE, wxT("Ablate"), wxBitmap( wxT("CaWE/res/TerrainEditToolAblate.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Ablate"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_BLUR, wxT("Blur"), wxBitmap( wxT("CaWE/res/TerrainEditToolBlur.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Blur"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_SHARPEN, wxT("Sharpen"), wxBitmap( wxT("CaWE/res/TerrainEditToolSharpen.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Sharpen"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_NOISE, wxT("Noise"), wxBitmap( wxT("CaWE/res/TerrainEditToolNoise.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Noise"), wxEmptyString );
	m_ToolBar->AddTool( ID_TOOL_ROAD, wxT("Create Road"), wxBitmap( wxT("CaWE/res/TerrainEditToolRoad.png"), wxBITMAP_TYPE_ANY ), wxNullBitmap, wxITEM_CHECK, wxT("Create Road"), wxEmptyString );
	m_ToolBar->Realize();
	
	bSizer7->Add( m_ToolBar, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Tool Options") ), wxVERTICAL );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText1;
	m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("Radius:"), wxDefaultPosition, wxSize( 60,-1 ), 0 );
	m_staticText1->Wrap( -1 );
	bSizer4->Add( m_staticText1, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SpinCtrlRadius = new wxSpinCtrl( this, ID_SPIN_CTRL_RADIUS, wxT("100"), wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 1, 10000, 1 );
	bSizer4->Add( m_SpinCtrlRadius, 0, wxRIGHT|wxLEFT, 5 );
	
	bSizer2->Add( bSizer4, 0, wxEXPAND, 5 );
	
	m_SliderRadius = new wxSlider( this, ID_SLIDER_RADIUS, 100, 1, 10000, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer2->Add( m_SliderRadius, 0, wxEXPAND|wxBOTTOM, 5 );
	
	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText2;
	m_staticText2 = new wxStaticText( this, wxID_ANY, wxT("Hardness:"), wxDefaultPosition, wxSize( 60,-1 ), 0 );
	m_staticText2->Wrap( -1 );
	bSizer5->Add( m_staticText2, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SpinCtrlHardness = new wxSpinCtrl( this, ID_SPIN_CTRL_HARDNESS, wxT("50"), wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 1, 100, 51 );
	bSizer5->Add( m_SpinCtrlHardness, 0, wxRIGHT|wxLEFT, 5 );
	
	bSizer3->Add( bSizer5, 0, wxEXPAND, 5 );
	
	m_SliderHardness = new wxSlider( this, ID_SLIDER_HARDNESS, 50, 1, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer3->Add( m_SliderHardness, 0, wxEXPAND|wxBOTTOM, 5 );
	
	bSizer1->Add( bSizer3, 1, wxEXPAND, 5 );
	
	sbSizer1->Add( bSizer1, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer112;
	bSizer112 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText11;
	m_staticText11 = new wxStaticText( this, wxID_ANY, wxT("Tool effect:"), wxDefaultPosition, wxSize( 60,-1 ), 0 );
	m_staticText11->Wrap( -1 );
	bSizer13->Add( m_staticText11, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SpinCtrlToolEffect = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 1, 100, 20 );
	bSizer13->Add( m_SpinCtrlToolEffect, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer12->Add( bSizer13, 1, wxEXPAND, 5 );
	
	m_SliderToolEffect = new wxSlider( this, wxID_ANY, 20, 1, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer12->Add( m_SliderToolEffect, 0, wxEXPAND, 5 );
	
	bSizer112->Add( bSizer12, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );
	
	bSizer112->Add( bSizer14, 1, wxEXPAND, 5 );
	
	sbSizer1->Add( bSizer112, 1, wxEXPAND, 5 );
	
	bSizer7->Add( sbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Terrain Options") ), wxVERTICAL );
	
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText5;
	m_staticText5 = new wxStaticText( this, wxID_ANY, wxT("Resolution:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	bSizer10->Add( m_staticText5, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_ChoiceResolutionChoices[] = { wxT("65x65"), wxT("129x129"), wxT("257x257"), wxT("513x513"), wxT("1025x1025"), wxT("2049x2049") };
	int m_ChoiceResolutionNChoices = sizeof( m_ChoiceResolutionChoices ) / sizeof( wxString );
	m_ChoiceResolution = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ChoiceResolutionNChoices, m_ChoiceResolutionChoices, 0 );
	m_ChoiceResolution->SetSelection( 0 );
	bSizer10->Add( m_ChoiceResolution, 0, wxALL, 5 );
	
	sbSizer4->Add( bSizer10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );
	
	m_ButtonImport = new wxButton( this, ID_BUTTON_IMPORT, wxT("Import"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_ButtonImport, 1, wxALL, 5 );
	
	m_ButtonExport = new wxButton( this, ID_BUTTON_EXPORT, wxT("Export"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_ButtonExport, 1, wxALL, 5 );
	
	m_ButtonGenerate = new wxButton( this, wxID_ANY, wxT("Generate"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer11->Add( m_ButtonGenerate, 1, wxALL, 5 );
	
	sbSizer4->Add( bSizer11, 0, wxEXPAND, 5 );
	
	bSizer7->Add( sbSizer4, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );
	
	// Connect Events
	this->Connect( ID_TOOL_RAISE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_LOWER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_FLATTEN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_FILL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_ABLATE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_BLUR, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_SHARPEN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_NOISE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Connect( ID_TOOL_ROAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	m_SpinCtrlRadius->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainEditorDialogFB::OnSpinCtrlRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SpinCtrlHardness->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainEditorDialogFB::OnSpinCtrlHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SpinCtrlToolEffect->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainEditorDialogFB::OnSpinCtrlToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_ChoiceResolution->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( TerrainEditorDialogFB::OnChoiceResolution ), NULL, this );
	m_ButtonImport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnButtonImport ), NULL, this );
	m_ButtonExport->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnButtonExport ), NULL, this );
	m_ButtonGenerate->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnButtonGenerate ), NULL, this );
}

TerrainEditorDialogFB::~TerrainEditorDialogFB()
{
	// Disconnect Events
	this->Disconnect( ID_TOOL_RAISE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_LOWER, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_FLATTEN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_FILL, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_ABLATE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_BLUR, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_SHARPEN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_NOISE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	this->Disconnect( ID_TOOL_ROAD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnToolClicked ) );
	m_SpinCtrlRadius->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainEditorDialogFB::OnSpinCtrlRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SliderRadius->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollRadius ), NULL, this );
	m_SpinCtrlHardness->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainEditorDialogFB::OnSpinCtrlHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SliderHardness->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollHardness ), NULL, this );
	m_SpinCtrlToolEffect->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainEditorDialogFB::OnSpinCtrlToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_SliderToolEffect->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainEditorDialogFB::OnSliderScrollToolEffect ), NULL, this );
	m_ChoiceResolution->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( TerrainEditorDialogFB::OnChoiceResolution ), NULL, this );
	m_ButtonImport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnButtonImport ), NULL, this );
	m_ButtonExport->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnButtonExport ), NULL, this );
	m_ButtonGenerate->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TerrainEditorDialogFB::OnButtonGenerate ), NULL, this );
}

TerrainGenerationDialogFB::TerrainGenerationDialogFB( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer15;
	bSizer15 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText15;
	m_staticText15 = new wxStaticText( this, wxID_ANY, wxT("Frequency:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	bSizer17->Add( m_staticText15, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SpinCtrlFrequency = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 1, 200, 10 );
	bSizer17->Add( m_SpinCtrlFrequency, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer16->Add( bSizer17, 1, wxEXPAND, 5 );
	
	m_SliderFrequency = new wxSlider( this, wxID_ANY, 10, 1, 200, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer16->Add( m_SliderFrequency, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer171;
	bSizer171 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText151;
	m_staticText151 = new wxStaticText( this, wxID_ANY, wxT("Octaves:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText151->Wrap( -1 );
	bSizer171->Add( m_staticText151, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SpinCtrlOctaves = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 1, 8, 4 );
	bSizer171->Add( m_SpinCtrlOctaves, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer16->Add( bSizer171, 1, wxEXPAND, 5 );
	
	m_SliderOctaves = new wxSlider( this, wxID_ANY, 4, 1, 8, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer16->Add( m_SliderOctaves, 0, wxEXPAND|wxBOTTOM, 5 );
	
	bSizer31->Add( bSizer16, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bSizer32;
	bSizer32 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer174;
	bSizer174 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText154;
	m_staticText154 = new wxStaticText( this, wxID_ANY, wxT("Lacunarity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText154->Wrap( -1 );
	bSizer174->Add( m_staticText154, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SpinCtrlLacunarity = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 10, 40, 20 );
	bSizer174->Add( m_SpinCtrlLacunarity, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer32->Add( bSizer174, 0, wxEXPAND, 5 );
	
	m_SliderLacunarity = new wxSlider( this, wxID_ANY, 20, 10, 40, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer32->Add( m_SliderLacunarity, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxBoxSizer* bSizer172;
	bSizer172 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText152;
	m_staticText152 = new wxStaticText( this, wxID_ANY, wxT("Persistence:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText152->Wrap( -1 );
	bSizer172->Add( m_staticText152, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_SpinCtrlPersistence = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 0, 100, 50 );
	bSizer172->Add( m_SpinCtrlPersistence, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer32->Add( bSizer172, 0, wxEXPAND, 5 );
	
	m_SliderPersistence = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL );
	bSizer32->Add( m_SliderPersistence, 0, wxEXPAND|wxBOTTOM, 5 );
	
	bSizer31->Add( bSizer32, 0, wxEXPAND|wxLEFT, 5 );
	
	bSizer15->Add( bSizer31, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer173;
	bSizer173 = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* m_staticText153;
	m_staticText153 = new wxStaticText( this, wxID_ANY, wxT("Seed:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText153->Wrap( -1 );
	bSizer173->Add( m_staticText153, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_SpinCtrlSeed = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 55,-1 ), wxSP_ARROW_KEYS, 1, 1000, 1 );
	bSizer173->Add( m_SpinCtrlSeed, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	bSizer15->Add( bSizer173, 0, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM, 5 );
	
	m_PreviewPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( 192,192 ), wxTAB_TRAVERSAL );
	bSizer15->Add( m_PreviewPanel, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	wxBoxSizer* bSizer33;
	bSizer33 = new wxBoxSizer( wxHORIZONTAL );
	
	wxButton* m_button4;
	m_button4 = new wxButton( this, wxID_OK, wxT("Generate Terrain"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer33->Add( m_button4, 1, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_ButtonCancel = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer33->Add( m_ButtonCancel, 1, wxALL, 5 );
	
	bSizer15->Add( bSizer33, 0, wxEXPAND|wxBOTTOM, 5 );
	
	this->SetSizer( bSizer15 );
	this->Layout();
	bSizer15->Fit( this );
	
	// Connect Events
	m_SpinCtrlFrequency->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SpinCtrlOctaves->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SpinCtrlLacunarity->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SpinCtrlPersistence->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SpinCtrlSeed->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlSeed ), NULL, this );
}

TerrainGenerationDialogFB::~TerrainGenerationDialogFB()
{
	// Disconnect Events
	m_SpinCtrlFrequency->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SliderFrequency->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderFrequency ), NULL, this );
	m_SpinCtrlOctaves->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SliderOctaves->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderOctaves ), NULL, this );
	m_SpinCtrlLacunarity->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SliderLacunarity->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderLacunarity ), NULL, this );
	m_SpinCtrlPersistence->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SliderPersistence->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TerrainGenerationDialogFB::OnSliderPersistence ), NULL, this );
	m_SpinCtrlSeed->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TerrainGenerationDialogFB::OnSpinCtrlSeed ), NULL, this );
}
