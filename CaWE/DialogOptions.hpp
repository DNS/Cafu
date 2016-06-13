/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_OPTIONS_HPP_INCLUDED
#define CAFU_DIALOG_OPTIONS_HPP_INCLUDED

#include "wx/wx.h"


class GameConfigT;
class wxSpinDoubleEvent;


class OptionsDialogT : public wxDialog
{
    public:

    OptionsDialogT(wxWindow* Parent);


    private:

    wxChoice*    GameCfg_GameConfigChoice;      // In the "Game Configurations" tab, this is the "Game Config" choice ("combobox").
    GameConfigT* GameCfg_LastSelConfig;         // In the "Game Configurations" tab, this points to the last selected config.
    wxTextCtrl*  GameCfg_DefaultTextureScale;
    wxTextCtrl*  GameCfg_DefaultLightmapScale;
    wxTextCtrl*  GameCfg_CordonTexture;
    wxTextCtrl*  GameCfg_EngineExe;
    wxTextCtrl*  GameCfg_BSPExe;
    wxTextCtrl*  GameCfg_PVSExe;
    wxTextCtrl*  GameCfg_LightExe;


    // Helper functions for the constructor.
    wxSizer* OptionsDialogInit        (wxWindow *parent, bool call_fit=true, bool set_sizer=true);
    wxSizer* OptionsGameConfigsTabInit(wxWindow *parent, bool call_fit=true, bool set_sizer=true);
    wxSizer* OptionsGeneralTabInit    (wxWindow *parent, bool call_fit=true, bool set_sizer=true);
    wxSizer* Options2DViewsTabInit    (wxWindow *parent, bool call_fit=true, bool set_sizer=true);
    wxSizer* Options3DViewsTabInit    (wxWindow *parent, bool call_fit=true, bool set_sizer=true);

    // Helper functions for the event handlers of the GameConfigs tab.
    void GameCfg_SaveInfo(GameConfigT* Config);     ///< Saves all data from the GameConfigs tab in the config pointed to by GameCfg.
    void GameCfg_Update_ConfigsList();              ///< (Re-)Inits the entire "Game Configurations" tab. Needed on dialog init, and after game config edits.

    // Event handlers.
    void OnOK(wxCommandEvent& Event);
    void OnHelp(wxCommandEvent& Event);
    void OnButton_General_BrowseCafuExe(wxCommandEvent& Event);
    void OnButton_General_BrowseCaBSPExe(wxCommandEvent& Event);
    void OnButton_General_BrowseCaPVSExe(wxCommandEvent& Event);
    void OnButton_General_BrowseCaLightExe(wxCommandEvent& Event);
    void OnChoice_GameCfg_GameConfigs(wxCommandEvent& Event);
    void OnButton_GameCfg_PickCordonTexture(wxCommandEvent& Event);
    void OnSpinCtrl_3DViews_MouseSensitivity(wxSpinDoubleEvent& Event);

    // IDs for the controls whose events we are interested in.
    enum
    {
        ID_CHECKBOX_GENERAL_INDEPENDENTWINDOWS=wxID_HIGHEST+1,
        ID_BUTTON_GENERAL_BrowseCafuExe,
        ID_BUTTON_GENERAL_BrowseCaBSPExe,
        ID_BUTTON_GENERAL_BrowseCaPVSExe,
        ID_BUTTON_GENERAL_BrowseCaLightExe,
        ID_CHOICE_GAMECFG_GameConfigs,
        ID_BUTTON_GAMECFG_PickCordonTexture,
        ID_SPINCTRL_3DVIEWS_MouseSensitivity
    };

    DECLARE_EVENT_TABLE()
};

#endif
