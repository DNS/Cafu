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

#include "DialogOptions.hpp"
#include "EntityClass.hpp"
#include "GameConfig.hpp"
#include "Options.hpp"
#include "AppCaWE.hpp"
#include "ParentFrame.hpp"
#include "EditorMaterialManager.hpp"

#include "wx/clrpicker.h"
#include "wx/notebook.h"
#include "wx/statline.h"
#include "wx/stdpaths.h"
#include "wx/spinctrl.h"
#include "wx/valgen.h"


/// This validator is for validating the initial grid spacing combobox in the Options dialog.
class InitialGridSpacingValidatorT : public wxValidator
{
    public:

    /// Constructor.
    InitialGridSpacingValidatorT(int& i)
        : m_Int(i)
    {
    }

    /// Copy constructor.
    InitialGridSpacingValidatorT(const InitialGridSpacingValidatorT& Val)
        : wxValidator(),
          m_Int(Val.m_Int)
    {
    }

    // Override all relevant wxValidator methods.
    wxObject* Clone() const { return new InitialGridSpacingValidatorT(*this); }

    bool Validate(wxWindow* Parent)
    {
        const wxString s=((wxComboBox*)m_validatorWindow)->GetValue();
        const int      i=wxAtoi(s);

        if (!s.IsNumber() || i<2 || i>2048)
        {
            wxMessageBox("Please enter a valid number for the initial grid spacing\n"
                         "(an integer in range 2 ... 2048).", "Initial grid spacing", wxOK | wxICON_ERROR, Parent);
            return false;
        }

        return true;
    }

    bool TransferToWindow()
    {
        if (!m_validatorWindow) return false;
        if (!m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox))) return false;

        ((wxComboBox*)m_validatorWindow)->SetValue(wxString::Format("%i", m_Int));
        return true;
    }

    bool TransferFromWindow()
    {
        if (!m_validatorWindow) return false;
        if (!m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox))) return false;

        // Same code as in Validate()...
        const wxString s=((wxComboBox*)m_validatorWindow)->GetValue();
        const int      i=wxAtoi(s);

        if (!s.IsNumber()) return false;
        if (i<2 || i>2048) return false;

        m_Int=i;
        return true;
    }


    private:

    int& m_Int;
};


/// This is a validator for colors used with wxColorPickerCtrl instances.
class ColorValidatorT : public wxValidator
{
    public:

    /// Constructor.
    ColorValidatorT(wxColour& Color)
        : m_Color(Color)
    {
    }

    /// Copy constructor.
    ColorValidatorT(const ColorValidatorT& Val)
        : wxValidator(),
          m_Color(Val.m_Color)
    {
    }

    // Override all relevant wxValidator methods.
    wxObject* Clone() const { return new ColorValidatorT(*this); }

    bool Validate(wxWindow* WXUNUSED(parent)) { return true; }

    bool TransferToWindow()
    {
        if (!m_validatorWindow) return false;
        if (!m_validatorWindow->IsKindOf(CLASSINFO(wxColourPickerCtrl))) return false;

        ((wxColourPickerCtrl*)m_validatorWindow)->SetColour(m_Color);
        return true;
    }

    bool TransferFromWindow()
    {
        if (!m_validatorWindow) return false;
        if (!m_validatorWindow->IsKindOf(CLASSINFO(wxColourPickerCtrl))) return false;

        m_Color=((wxColourPickerCtrl*)m_validatorWindow)->GetColour();
        return true;
    }


    private:

    wxColour& m_Color;
};


BEGIN_EVENT_TABLE(OptionsDialogT, wxDialog)
    EVT_BUTTON(wxID_OK, OptionsDialogT::OnOK)
    EVT_BUTTON(wxID_HELP, OptionsDialogT::OnHelp)
    EVT_BUTTON(OptionsDialogT::ID_BUTTON_GENERAL_BrowseCafuExe,     OptionsDialogT::OnButton_General_BrowseCafuExe)
    EVT_BUTTON(OptionsDialogT::ID_BUTTON_GENERAL_BrowseCaBSPExe,    OptionsDialogT::OnButton_General_BrowseCaBSPExe)
    EVT_BUTTON(OptionsDialogT::ID_BUTTON_GENERAL_BrowseCaPVSExe,    OptionsDialogT::OnButton_General_BrowseCaPVSExe)
    EVT_BUTTON(OptionsDialogT::ID_BUTTON_GENERAL_BrowseCaLightExe,  OptionsDialogT::OnButton_General_BrowseCaLightExe)
    EVT_CHOICE(OptionsDialogT::ID_CHOICE_GAMECFG_GameConfigs,       OptionsDialogT::OnChoice_GameCfg_GameConfigs)
    EVT_BUTTON(OptionsDialogT::ID_BUTTON_GAMECFG_PickCordonTexture, OptionsDialogT::OnButton_GameCfg_PickCordonTexture)
END_EVENT_TABLE()


////////////////////////////////////////////////////////////////////
// Start of code generated by wxDesiger (and modified by me, CF). //
////////////////////////////////////////////////////////////////////

wxSizer* OptionsDialogT::OptionsDialogInit( wxWindow *parent, bool call_fit, bool set_sizer )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxNotebook *item2 = new wxNotebook( parent, -1, wxDefaultPosition, wxDefaultSize, 0 );

    wxPanel *item3 = new wxPanel( item2, -1 );
    OptionsGameConfigsTabInit( item3, false );
    item2->AddPage( item3, wxT("Game Configurations") );

    wxPanel *item4 = new wxPanel( item2, -1 );
    OptionsGeneralTabInit( item4, false );
    item2->AddPage( item4, wxT("General") );

    wxPanel *item5 = new wxPanel( item2, -1 );
    Options2DViewsTabInit( item5, false );
    item2->AddPage( item5, wxT("2D Views") );

    wxPanel *item6 = new wxPanel( item2, -1 );
    Options3DViewsTabInit( item6, false );
    item2->AddPage( item6, wxT("3D Views") );

    // Leave this panel away for a while - we currently have no use for it anyway.
    // wxPanel *item7 = new wxPanel( item2, -1 );
    // OptionsMaterialsTabInit( item7, false );
    // item2->AddPage( item7, wxT("Materials") );

    item0->Add( item2, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item8 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item9 = new wxButton( parent, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->SetDefault();
    item8->Add( item9, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item10 = new wxButton( parent, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->Add( item10, 0, wxALIGN_CENTER|wxALL, 5 );

 // wxButton *item11 = new wxButton( parent, wxID_APPLY, wxT("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
 // item8->Add( item11, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item12 = new wxButton( parent, wxID_HELP, wxT("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->Add( item12, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item8, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    return item0;
}

wxSizer* OptionsDialogT::OptionsGameConfigsTabInit( wxWindow *parent, bool call_fit, bool set_sizer )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticText *item1 = new wxStaticText( parent, -1, wxT("Configuration:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );


    wxBoxSizer *item2 = new wxBoxSizer( wxHORIZONTAL );

    GameCfg_GameConfigChoice = new wxChoice( parent, ID_CHOICE_GAMECFG_GameConfigs, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT /*FIXME: Temp. work-around that works on Win32. Other platforms? Note: Choice!=ComboBox*/);
    item2->Add(GameCfg_GameConfigChoice, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item0->Add( item2, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );



    wxStaticLine *item5 = new wxStaticLine( parent, -1, wxDefaultPosition, wxSize(20,-1), wxLI_HORIZONTAL );
    item0->Add( item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item21 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer *item22 = new wxBoxSizer( wxVERTICAL );

    wxStaticText *item23 = new wxStaticText( parent, -1, wxT("Default Point Entity class:"), wxDefaultPosition, wxDefaultSize, 0 );
    item22->Add( item23, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

    GameCfg_DefaultPointEntity = new wxChoice( parent, -1, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT /*FIXME: Temp. work-around that works on Win32. Other platforms? Note: Choice!=ComboBox*/);
    item22->Add(GameCfg_DefaultPointEntity, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item21->Add( item22, 1, wxALIGN_CENTER, 5 );

    wxBoxSizer *item25 = new wxBoxSizer( wxVERTICAL );

    wxStaticText *item26 = new wxStaticText( parent, -1, wxT("Default Brush Entity class:"), wxDefaultPosition, wxDefaultSize, 0 );
    item25->Add( item26, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

    GameCfg_DefaultBrushEntity = new wxChoice( parent, -1, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT /*FIXME: Temp. work-around that works on Win32. Other platforms? Note: Choice!=ComboBox*/);
    item25->Add(GameCfg_DefaultBrushEntity, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item21->Add( item25, 1, wxALIGN_CENTER, 5 );

    item0->Add( item21, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item28 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item29 = new wxStaticText( parent, -1, wxT("Default texture scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    item28->Add( item29, 0, wxALIGN_CENTER|wxALL, 5 );

    GameCfg_DefaultTextureScale = new wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    item28->Add(GameCfg_DefaultTextureScale, 1, wxALIGN_CENTER|wxALL, 5 );

    wxStaticText *item31 = new wxStaticText( parent, -1, wxT("Default lightmap scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    item28->Add( item31, 0, wxALIGN_CENTER|wxALL, 5 );

    GameCfg_DefaultLightmapScale = new wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    item28->Add(GameCfg_DefaultLightmapScale, 1, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item28, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item33 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item34 = new wxStaticText( parent, -1, wxT("Cordon texture:"), wxDefaultPosition, wxDefaultSize, 0 );
    item33->Add( item34, 0, wxALIGN_CENTER|wxALL, 5 );

    GameCfg_CordonTexture = new wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    item33->Add(GameCfg_CordonTexture, 1, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item36 = new wxButton( parent, ID_BUTTON_GAMECFG_PickCordonTexture, wxT("Pick..."), wxDefaultPosition, wxDefaultSize, 0 );
    // I'm permanently disabling this button, because the ModDir might not be set yet, or the ModDir might not be mounted with the FileSys yet, etc.
    // Especially on first-time configurations no materials will be available for browsing when the button is pressed, and anyways,
    // the control for which material is used for the cordon texture should either be in the tool options bar of the cordon tool,
    // or somewhere hidden deeply in an advanced settings tab, or in a convar or script...
    item36->Disable();
    item33->Add( item36, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item33, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    GameCfg_Update_ConfigsList();
    return item0;
}

wxSizer* OptionsDialogT::OptionsGeneralTabInit( wxWindow *parent, bool call_fit, bool set_sizer )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item1 = new wxStaticBox( parent, -1, wxT("Options") );
    wxStaticBoxSizer *item2 = new wxStaticBoxSizer( item1, wxVERTICAL );

    wxBoxSizer *item5 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item6 = new wxStaticText( parent, -1, wxT("Undo levels"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->Add( item6, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *item7 = new wxSpinCtrl( parent, -1, wxT("0"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 5, 999);
    item7->SetValidator(wxGenericValidator(&Options.general.UndoLevels));
    item5->Add( item7, 0, wxALIGN_CENTER|wxALL, 5 );

    item2->Add( item5, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );

    // Maybe later...
    // wxCheckBox *item9 = new wxCheckBox( parent, -1, wxT("Stretch arches to fit original bounding rectangle"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.general.StretchArches));
    // item2->Add( item9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item0->Add( item2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    //item0->Add( 20, 20, 1, wxALIGN_CENTER|wxALL, 5 );

    wxStaticBox *item10 = new wxStaticBox( parent, -1, wxT("Executables") );
    wxStaticBoxSizer *item11 = new wxStaticBoxSizer( item10, wxVERTICAL );

    wxStaticText *item12 = new wxStaticText( parent, -1, wxT("Engine executable:"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add( item12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

    wxBoxSizer *item13 = new wxBoxSizer( wxHORIZONTAL );

    GameCfg_EngineExe = new wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    GameCfg_EngineExe->SetValidator(wxGenericValidator(&Options.general.EngineExe));
    item13->Add(GameCfg_EngineExe, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxButton *item14 = new wxButton( parent, ID_BUTTON_GENERAL_BrowseCafuExe, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item13->Add( item14, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item11->Add( item13, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxStaticText *item15 = new wxStaticText( parent, -1, wxT("CaBSP executable:"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add( item15, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

    wxBoxSizer *item16 = new wxBoxSizer( wxHORIZONTAL );

    GameCfg_BSPExe = new wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    GameCfg_BSPExe->SetValidator(wxGenericValidator(&Options.general.BSPExe));
    item16->Add(GameCfg_BSPExe, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxButton *item17 = new wxButton( parent, ID_BUTTON_GENERAL_BrowseCaBSPExe, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item16->Add( item17, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item11->Add( item16, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxStaticText *item18 = new wxStaticText( parent, -1, wxT("CaPVS executable:"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add( item18, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

    wxBoxSizer *item19 = new wxBoxSizer( wxHORIZONTAL );

    GameCfg_PVSExe = new wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    GameCfg_PVSExe->SetValidator(wxGenericValidator(&Options.general.PVSExe));
    item19->Add(GameCfg_PVSExe, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxButton *item20 = new wxButton( parent, ID_BUTTON_GENERAL_BrowseCaPVSExe, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item19->Add( item20, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item11->Add( item19, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxStaticText *item21 = new wxStaticText( parent, -1, wxT("CaLight executable:"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add( item21, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

    wxBoxSizer *item22 = new wxBoxSizer( wxHORIZONTAL );

    GameCfg_LightExe = new wxTextCtrl( parent, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    GameCfg_LightExe->SetValidator(wxGenericValidator(&Options.general.LightExe));
    item22->Add(GameCfg_LightExe, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxButton *item23 = new wxButton( parent, ID_BUTTON_GENERAL_BrowseCaLightExe, wxT("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
    item22->Add( item23, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item11->Add( item22, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item11, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    // Need the additional "\n\n", because the size of the wxStaticText doesn't take the auto-line-wrapping into account.
    wxStaticText *item24 = new wxStaticText( parent, -1, wxString("Your CaWE configuration files are located at:\n")+wxStandardPaths::Get().GetUserDataDir()+"\n\n", wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item24, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    return item0;
}

wxSizer* OptionsDialogT::Options2DViewsTabInit( wxWindow *parent, bool call_fit, bool set_sizer )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item2 = new wxStaticBox( parent, -1, wxT("Options") );
    wxStaticBoxSizer *item1 = new wxStaticBoxSizer( item2, wxVERTICAL );

    wxCheckBox *item6 = new wxCheckBox( parent, -1, wxT("Draw Vertices"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view2d.DrawVertices));
    item1->Add( item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxCheckBox *item10 = new wxCheckBox( parent, -1, wxT("Use group colors for object lines"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view2d.UseGroupColors));
    item1->Add( item10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    // Maybe later...
    // wxCheckBox *item12 = new wxCheckBox( parent, -1, wxT("Reorient primitives on creation in the current 2D view"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view2d.OrientPrimitives));
    // item1->Add( item12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxCheckBox *item14 = new wxCheckBox( parent, -1, wxT("Selection box selects by center handles only"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view2d.SelectByHandles));
    item1->Add( item14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxCheckBox* ShowEntInfoCB = new wxCheckBox( parent, -1, wxT("Show entity info (class, name, ...)"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view2d.ShowEntityInfo));
    item1->Add(ShowEntInfoCB, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxCheckBox* ShowEntTargetsCB = new wxCheckBox( parent, -1, wxT("Show entity targets for game scripts and code"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view2d.ShowEntityTargets));
    item1->Add(ShowEntTargetsCB, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item0->Add( item1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticBox *item16 = new wxStaticBox( parent, -1, wxT("Grid") );
    wxStaticBoxSizer *item15 = new wxStaticBoxSizer( item16, wxVERTICAL );


    wxBoxSizer *item17 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item18 = new wxStaticText( parent, -1, wxT("Initial spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
    item17->Add( item18, 0, wxALIGN_CENTER|wxALL, 5 );

    wxString strs19[] =
    {
        wxT("4"),
        wxT("8"),
        wxT("16"),
        wxT("32"),
        wxT("64"),
        wxT("128"),
        wxT("256")
    };
    wxComboBox *item19 = new wxComboBox( parent, -1, "", wxDefaultPosition, wxSize(60, -1), 7, strs19, 0 );
    item19->SetValidator(InitialGridSpacingValidatorT(Options.Grid.InitialSpacing));
    item17->Add( item19, 0, wxALIGN_CENTER|wxALL, 1 );

    wxStaticText *item20 = new wxStaticText( parent, -1, wxT("world units:     "), wxDefaultPosition, wxDefaultSize, 0 );
    item17->Add( item20, 0, wxALIGN_CENTER|wxALL, 5 );

    wxColourPickerCtrl* item21=new wxColourPickerCtrl(parent, -1, Options.Grid.ColorBaseGrid, wxDefaultPosition, wxDefaultSize, 0, ColorValidatorT(Options.Grid.ColorBaseGrid));
    item17->Add( item21, 0, wxALIGN_CENTER|wxALL, 5 );

    item15->Add( item17, 0, wxALIGN_CENTER_VERTICAL, 5 );


    wxBoxSizer *item25 = new wxBoxSizer( wxHORIZONTAL );

    wxCheckBox *item26 = new wxCheckBox( parent, -1, wxT("Highlight every"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.Grid.ShowHighlight1));
    item25->Add( item26, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *item27 = new wxSpinCtrl( parent, -1, wxEmptyString, wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 4, 2048);
    item27->SetValidator(wxGenericValidator(&Options.Grid.SpacingHighlight1));
    item25->Add( item27, 0, wxALIGN_CENTER|wxALL, 1 );

    wxStaticText *item28 = new wxStaticText( parent, -1, wxT("world unit:"), wxDefaultPosition, wxDefaultSize, 0 );
    item25->Add( item28, 0, wxALIGN_CENTER|wxALL, 5 );

    wxColourPickerCtrl* item28a=new wxColourPickerCtrl(parent, -1, Options.Grid.ColorHighlight1, wxDefaultPosition, wxDefaultSize, 0, ColorValidatorT(Options.Grid.ColorHighlight1));
    item25->Add( item28a, 0, wxALIGN_CENTER|wxALL, 5 );

    item15->Add( item25, 0, wxALIGN_CENTER_VERTICAL, 5 );


    wxBoxSizer *itemB25 = new wxBoxSizer( wxHORIZONTAL );

    wxCheckBox *itemB26 = new wxCheckBox( parent, -1, wxT("Highlight every"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.Grid.ShowHighlight2));
    itemB25->Add( itemB26, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *itemB27 = new wxSpinCtrl( parent, -1, wxEmptyString, wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 4, 2048);
    itemB27->SetValidator(wxGenericValidator(&Options.Grid.SpacingHighlight2));
    itemB25->Add( itemB27, 0, wxALIGN_CENTER|wxALL, 1 );

    wxStaticText *itemB28 = new wxStaticText( parent, -1, wxT("world unit:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemB25->Add( itemB28, 0, wxALIGN_CENTER|wxALL, 5 );

    wxColourPickerCtrl* itemB28a=new wxColourPickerCtrl(parent, -1, Options.Grid.ColorHighlight2, wxDefaultPosition, wxDefaultSize, 0, ColorValidatorT(Options.Grid.ColorHighlight2));
    itemB25->Add( itemB28a, 0, wxALIGN_CENTER|wxALL, 5 );

    item15->Add( itemB25, 0, wxALIGN_CENTER_VERTICAL, 5 );


    wxCheckBox *item30 = new wxCheckBox( parent, -1, wxT("Dotted Grid"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.Grid.UseDottedGrid));
    item15->Add( item30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


    wxBoxSizer *HideGrid_Sizer = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *HideGrid_Text1 = new wxStaticText( parent, -1, wxT("Hide grid when the spacing is less than"), wxDefaultPosition, wxDefaultSize, 0 );
    HideGrid_Sizer->Add( HideGrid_Text1, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *HideGrid_Spin = new wxSpinCtrl( parent, -1, wxEmptyString, wxDefaultPosition, wxSize(40, -1), wxSP_ARROW_KEYS, 2, 10);
    HideGrid_Spin->SetValidator(wxGenericValidator(&Options.Grid.MinPixelSpacing));
    HideGrid_Sizer->Add( HideGrid_Spin, 0, wxALIGN_CENTER|wxALL, 1 );

    wxStaticText *HideGrid_Text2 = new wxStaticText( parent, -1, wxT("pixels."), wxDefaultPosition, wxDefaultSize, 0 );
    HideGrid_Sizer->Add( HideGrid_Text2, 0, wxALIGN_CENTER|wxALL, 5 );

    item15->Add( HideGrid_Sizer, 0, wxALIGN_CENTER_VERTICAL, 5 );


    item0->Add( item15, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    return item0;
}

wxSizer* OptionsDialogT::Options3DViewsTabInit( wxWindow *parent, bool call_fit, bool set_sizer )
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item2 = new wxStaticBox( parent, -1, wxT("Performance") );
    wxStaticBoxSizer *item1 = new wxStaticBoxSizer( item2, wxVERTICAL );

    wxCheckBox *item4 = new wxCheckBox( parent, -1, wxT("Animate models"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view3d.AnimateModels));
    item1->Add( item4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxFlexGridSizer *item5 = new wxFlexGridSizer( 2, 0, 0 );

    wxStaticText *item6 = new wxStaticText( parent, -1, wxT("Back clipping plane:"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->Add( item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    // TODO: Logarithmic slider behaviour would be cool...
    wxSlider *item7 = new wxSlider( parent, -1, 0, 500, 100000, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS, wxGenericValidator(&Options.view3d.BackPlane));
    item5->Add( item7, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

    wxStaticText *item8 = new wxStaticText( parent, -1, wxT("Model render distance:"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->Add( item8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxSlider *item9 = new wxSlider( parent, -1, 0, 0, 10000, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS, wxGenericValidator(&Options.view3d.ModelDistance));
    item5->Add( item9, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

    item1->Add( item5, 0, wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticBox *item11 = new wxStaticBox( parent, -1, wxT("Navigation") );
    wxStaticBoxSizer *item10 = new wxStaticBoxSizer( item11, wxVERTICAL );

    wxCheckBox *item13 = new wxCheckBox( parent, -1, wxT("Reverse mouse Y axis (aircraft style)"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&Options.view3d.ReverseY));
    item10->Add( item13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxFlexGridSizer *item14 = new wxFlexGridSizer( 2, 0, 0 );

    wxStaticText *item15 = new wxStaticText( parent, -1, wxT("Max. camera velocity:"), wxDefaultPosition, wxDefaultSize, 0 );
    item14->Add( item15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxSlider *item16 = new wxSlider( parent, -1, 0, 100, 10000, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS, wxGenericValidator(&Options.view3d.MaxCameraVelocity));
    item14->Add( item16, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

    wxStaticText *item17 = new wxStaticText( parent, -1, wxT("Time to top speed (msec):"), wxDefaultPosition, wxDefaultSize, 0 );
    item14->Add( item17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxSlider *item18 = new wxSlider( parent, -1, 0, 0, 10000, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS, wxGenericValidator(&Options.view3d.TimeToMaxSpeed));
    item14->Add( item18, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

    item10->Add( item14, 0, wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    return item0;
}

//////////////////////////////////////////////////////////////////
// End of code generated by wxDesiger (and modified by me, CF). //
//////////////////////////////////////////////////////////////////


OptionsDialogT::OptionsDialogT(wxWindow* Parent)
    : wxDialog(Parent, -1, wxString("Configure CaWE Options"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    GameCfg_LastSelConfig=NULL;

    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    OptionsDialogInit(this);
    Center(wxBOTH);
}


void OptionsDialogT::GameCfg_SaveInfo(GameConfigT* Config)
{
    if (Config==NULL) return;

    Config->DefaultPointEntity=GameCfg_DefaultPointEntity->GetStringSelection();
    Config->DefaultSolidEntity=GameCfg_DefaultBrushEntity->GetStringSelection();

    // Save the default scale for textures.
    Config->DefaultTextureScale=wxAtof(GameCfg_DefaultTextureScale->GetValue());
    if (Config->DefaultTextureScale==0.0f) Config->DefaultTextureScale=1.0f;

    // Save the default scale for lightmaps.
    Config->DefaultLightmapScale=wxAtof(GameCfg_DefaultLightmapScale->GetValue());
    if (Config->DefaultLightmapScale==0.0f) Config->DefaultLightmapScale=16.0f;

    Config->CordonTexture=GameCfg_CordonTexture->GetValue();
}


// (Re-)Inits the entire "Game Configurations" tab. Needed on dialog init, and after game config edits.
void OptionsDialogT::GameCfg_Update_ConfigsList()
{
    GameCfg_LastSelConfig=NULL;

    // Get the pointer to the currently selected config.
    int          CurSel=GameCfg_GameConfigChoice->GetSelection();
    GameConfigT* SelCfg =(CurSel!=-1) ? (GameConfigT*)GameCfg_GameConfigChoice->GetClientData(CurSel) : NULL;

    GameCfg_GameConfigChoice->Clear();

    // Add configs to the choice box.
    for (unsigned long i=0; i<Options.GameConfigs.Size(); i++)
    {
        GameCfg_GameConfigChoice->Append(Options.GameConfigs[i]->Name, Options.GameConfigs[i]);

        // Found the config that was previously selected? Then make sure it stays selected.
        // Must select via the string (not index i) because the choicebox is auto-sorted.
        if (i==0 || Options.GameConfigs[i]==SelCfg) GameCfg_GameConfigChoice->SetStringSelection(Options.GameConfigs[i]->Name);
    }

    wxCommandEvent CE; OnChoice_GameCfg_GameConfigs(CE);
}


// (Re-)Inits the entity lists in the "Game Configurations" tab.
void OptionsDialogT::GameCfg_Update_EntityLists()
{
    if (!GameCfg_LastSelConfig) return;

    GameCfg_DefaultPointEntity->Clear();
    GameCfg_DefaultBrushEntity->Clear();

    const ArrayT<const EntityClassT*>& Classes=GameCfg_LastSelConfig->GetEntityClasses();

    for (unsigned long ClassNr=0; ClassNr<Classes.Size(); ClassNr++)
    {
        if (Classes[ClassNr]->IsSolidClass()) GameCfg_DefaultBrushEntity->Append(Classes[ClassNr]->GetName());
                                         else GameCfg_DefaultPointEntity->Append(Classes[ClassNr]->GetName());
    }


    if (!GameCfg_DefaultBrushEntity->IsEmpty())
    {
        const int Pos1=GameCfg_DefaultBrushEntity->FindString(GameCfg_LastSelConfig->DefaultSolidEntity);

        if (Pos1!=wxNOT_FOUND) GameCfg_DefaultBrushEntity->SetSelection(Pos1);
                          else GameCfg_DefaultBrushEntity->SetSelection(0);
    }

    if (!GameCfg_DefaultPointEntity->IsEmpty())
    {
        const int Pos1=GameCfg_DefaultPointEntity->FindString(GameCfg_LastSelConfig->DefaultPointEntity);

        if (Pos1!=wxNOT_FOUND) GameCfg_DefaultPointEntity->SetSelection(Pos1);
                          else GameCfg_DefaultPointEntity->SetSelection(0);
    }
}


/*******************************/
/*** Event Handler Functions ***/
/*******************************/

void OptionsDialogT::OnOK(wxCommandEvent& Event)
{
    if (Validate() && TransferDataFromWindow())
    {
        GameCfg_SaveInfo(GameCfg_LastSelConfig);

        // This should be enough to apply the new settings...
        wxGetApp().GetParentFrame()->Refresh();

        // Quit the dialog.
        EndModal(wxID_OK);
    }
}


void OptionsDialogT::OnHelp(wxCommandEvent& Event)
{
    if (!wxLaunchDefaultBrowser("http://www.cafu.de/wiki"))
        wxMessageBox("Sorry, I could not open www.cafu.de/wiki in your default browser automatically.");
}


void OptionsDialogT::OnChoice_GameCfg_GameConfigs(wxCommandEvent& Event)
{
    GameCfg_SaveInfo(GameCfg_LastSelConfig);

    const int CurrentSelection=GameCfg_GameConfigChoice->GetSelection();
    GameCfg_LastSelConfig=CurrentSelection!=-1 ? (GameConfigT*)GameCfg_GameConfigChoice->GetClientData(CurrentSelection) : NULL;

    GameCfg_DefaultPointEntity  ->Enable(GameCfg_LastSelConfig!=NULL);
    GameCfg_DefaultBrushEntity  ->Enable(GameCfg_LastSelConfig!=NULL);
    GameCfg_DefaultTextureScale ->Enable(GameCfg_LastSelConfig!=NULL);
    GameCfg_DefaultLightmapScale->Enable(GameCfg_LastSelConfig!=NULL);
    GameCfg_CordonTexture       ->Enable(GameCfg_LastSelConfig!=NULL);

    if (GameCfg_LastSelConfig==NULL) return;

    GameCfg_DefaultTextureScale ->SetValue(wxString::Format("%g", GameCfg_LastSelConfig->DefaultTextureScale));
    GameCfg_DefaultLightmapScale->SetValue(wxString::Format("%g", GameCfg_LastSelConfig->DefaultLightmapScale));
    GameCfg_CordonTexture       ->SetValue(GameCfg_LastSelConfig->CordonTexture);

    // Set the "Default Point Entity" and "Default Brush Entity" choice boxes.
    GameCfg_Update_EntityLists();
}


void OptionsDialogT::OnButton_GameCfg_PickCordonTexture(wxCommandEvent& Event)
{
    // I've disabled this button from being clicked - see comment near item36->Disable() above for details.
#if 0
    MaterialBrowserDialogT MatBrowser(this, NULL /*no document*/, GameCfg_LastSelConfig->GetMatMan().FindMaterial(GameCfg_CordonTexture->GetValue(), false), "", false);

    if (MatBrowser.ShowModal()==wxID_OK)
        if (MatBrowser.GetCurrentMaterial()!=NULL)
            GameCfg_CordonTexture->SetValue(MatBrowser.GetCurrentMaterial()->GetName());
#endif
}


#ifdef __WXMSW__
static const wxString ExeWildcard="Program Executable (*.exe)|*.exe";
#else
static const wxString ExeWildcard="All Files|*";
#endif


void OptionsDialogT::OnButton_General_BrowseCafuExe(wxCommandEvent& Event)
{
    wxString File=wxFileSelector("Select Cafu Executable", "", "", "", ExeWildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST /*| wxFD_HIDE_READONLY NYI! */);

    if (File=="") return;
    GameCfg_EngineExe->SetValue(File);
}


void OptionsDialogT::OnButton_General_BrowseCaBSPExe(wxCommandEvent& Event)
{
    wxString File=wxFileSelector("Select CaBSP Executable", "", "", "", ExeWildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST /*| wxFD_HIDE_READONLY NYI! */);

    if (File=="") return;
    GameCfg_BSPExe->SetValue(File);
}


void OptionsDialogT::OnButton_General_BrowseCaPVSExe(wxCommandEvent& Event)
{
    wxString File=wxFileSelector("Select CaPVS Executable", "", "", "", ExeWildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST /*| wxFD_HIDE_READONLY NYI! */);

    if (File=="") return;
    GameCfg_PVSExe->SetValue(File);
}


void OptionsDialogT::OnButton_General_BrowseCaLightExe(wxCommandEvent& Event)
{
    wxString File=wxFileSelector("Select CaLight Executable", "", "", "", ExeWildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST /*| wxFD_HIDE_READONLY NYI! */);

    if (File=="") return;
    GameCfg_LightExe->SetValue(File);
}
