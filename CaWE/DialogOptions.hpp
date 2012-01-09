/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_DIALOG_OPTIONS_HPP_INCLUDED
#define CAFU_DIALOG_OPTIONS_HPP_INCLUDED

#include "wx/spinctrl.h"
#include "wx/wx.h"


class GameConfigT;


class OptionsDialogT : public wxDialog
{
    public:

    OptionsDialogT(wxWindow* Parent);


    private:

    wxChoice*    GameCfg_GameConfigChoice;      // In the "Game Configurations" tab, this is the "Game Config" choice ("combobox").
    GameConfigT* GameCfg_LastSelConfig;         // In the "Game Configurations" tab, this points to the last selected config.
    wxChoice*    GameCfg_DefaultPointEntity;
    wxChoice*    GameCfg_DefaultBrushEntity;
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
    void GameCfg_Update_EntityLists();              ///< (Re-)Inits the entity lists in the "Game Configurations" tab.

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
