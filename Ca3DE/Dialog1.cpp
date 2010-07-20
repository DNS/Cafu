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

/* This legacy struct is now replaced by convars.
class Ca3DE_OptionsT
{
    public:

    // Constants
    const static char CLIENT_RUNMODE;
    const static char SERVER_RUNMODE;

    // Overall options
    char           RunMode;

    // Client options
    char           DeathMatchPlayerName[128];
    char           DeathMatchModelName[128];
    bool           ClientFullScreen;
    unsigned short ClientWindowSizeX;
    unsigned short ClientWindowSizeY;
    std::string    ClientDesiredRenderer;
    unsigned short ClientPortNr;
    char           ClientRemoteName[128];
    unsigned short ClientRemotePortNr;
    char           ClientDisplayRes;    // Argh! This is not consistent with the 'ClientWindowSizeX/Y' members...
    char           ClientDisplayBPP;
    char           ClientTextureDetail;

    // Server options
    char           ServerGameName[128];
    char           ServerWorldName[128];
    unsigned short ServerPortNr;

    // Initial StatusBar text (will be set on WM_INITDIALOG)
    char           StatusBar[128];


    // Constructor
    Ca3DE_OptionsT()
    {
#ifdef CAFU_DEDICATED_SERVER
        RunMode=SERVER_RUNMODE;
#else
        RunMode=CLIENT_RUNMODE | SERVER_RUNMODE;
#endif

        strcpy(DeathMatchPlayerName, "Player");
        strcpy(DeathMatchModelName, "James");
        ClientFullScreen=true;
        ClientWindowSizeX=1024;
        ClientWindowSizeY=768;
        ClientDesiredRenderer="";
        ClientPortNr=33000;
        strcpy(ClientRemoteName, "192.168.1.1");
        ClientRemotePortNr=30000;
        ClientDisplayRes=0;
        ClientDisplayBPP=1;
        ClientTextureDetail=0;

        strcpy(ServerGameName, "DeathMatch");
        strcpy(ServerWorldName, "TechDemo");
        ServerPortNr=30000;

        strcpy(StatusBar, "Status - OK");
    }
} Ca3DE_Options;

const char Ca3DE_OptionsT::CLIENT_RUNMODE=1;
const char Ca3DE_OptionsT::SERVER_RUNMODE=2; */


const char* StripExtension(char* PathName)
{
    size_t Length=strlen(PathName);

    if (Length==0) return PathName;

    for (Length=Length-1; Length>0 && PathName[Length]!='.'; Length--)
        if (PathName[Length]=='/' || PathName[Length]=='\\') return PathName;

    if (Length) PathName[Length]=0;
    return PathName;
}


#ifdef _WIN32
bool IsGameDirectory(const WIN32_FIND_DATA& FindFileData)
{
    // Ist dieses File wirklich ein Verzeichnis?
    if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0) return false;

    // Verzeichnisname nicht zu lang?
    if (strlen(FindFileData.cFileName)>64) return false;

    // Nicht eines der Verzeichnisse "." oder ".."?
    if (strcmp(FindFileData.cFileName,  ".")==0) return false;
    if (strcmp(FindFileData.cFileName, "..")==0) return false;

    // Existiert auch die DLL 'Games/Verzeichnisname/Code/Verzeichnisname.dll'?
#ifdef SCONS_BUILD_DIR
    #define QUOTE(str) QUOTE_HELPER(str)
    #define QUOTE_HELPER(str) #str

    std::string DllName=std::string("Games/")+FindFileData.cFileName+"/Code/"+QUOTE(SCONS_BUILD_DIR)+"/"+FindFileData.cFileName+".dll";
    FILE* DllFile=fopen(DllName.c_str(), "rb");

    #undef QUOTE
    #undef QUOTE_HELPER
#else
    char DllName[256];
    sprintf(DllName, "Games/%s/Code/%s%s.dll", FindFileData.cFileName, FindFileData.cFileName, PlatformAux::GetEnvFileSuffix().c_str());
    FILE* DllFile=fopen(DllName, "rb");
#endif
    if (!DllFile) return false;
    fclose (DllFile);

    return true;
}


bool ReadDialogSettings(HWND hDialog)
{
    BOOL ConversionOK;
    char ReadStr[1024]; ReadStr[1023]=0;

    // 1. Allgemein: Run-Mode
         if (IsDlgButtonChecked(hDialog, DLG1_RM_CLIENTONLY)) Options_RunMode=CLIENT_RUNMODE;
    else if (IsDlgButtonChecked(hDialog, DLG1_RM_SERVERONLY)) Options_RunMode=SERVER_RUNMODE;
    else                                                      Options_RunMode=CLIENT_RUNMODE | SERVER_RUNMODE;

    // 2.1. Client: DeathMatch-PlayerName
    GetDlgItemText(hDialog, DLG1_CL_PLAYERNAME_IN, ReadStr, sizeof(ReadStr)-1);
    Options_DeathMatchPlayerName=std::string(ReadStr);

    // 2.2. Client: DeathMatch-ModelName
    GetDlgItemText(hDialog, DLG1_CL_MODELNAME_IN, ReadStr, sizeof(ReadStr)-1);
    Options_DeathMatchModelName=std::string(ReadStr);

    // 2.3. Client: PortNr
    Options_ClientPortNr=int(GetDlgItemInt(hDialog, DLG1_CL_PORT_IN, &ConversionOK, FALSE));
    if (!ConversionOK)
    {
        SetDlgItemText(hDialog, DLG1_STATUSBAR, "Conversion of client port number failed!");
        return false;
    }

    // 2.4. Client: Remote ServerName
    GetDlgItemText(hDialog, DLG1_CL_SV_NAME_IN, ReadStr, sizeof(ReadStr)-1);
    Options_ClientRemoteName=std::string(ReadStr);

    // 2.5. Client: Remote PortNr
    Options_ClientRemotePortNr=int(GetDlgItemInt(hDialog, DLG1_CL_SV_PORT_IN, &ConversionOK, FALSE));
    if (!ConversionOK)
    {
        SetDlgItemText(hDialog, DLG1_STATUSBAR, "Conversion of remote server port number failed!");
        return false;
    }

    // 2.6. Client: Display resolution, BPP, and texture detail.
    LRESULT Result1=SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_RES_IN   , CB_GETCURSEL, 0, 0); if (Result1==CB_ERR) Result1=0;
    LRESULT Result2=SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_BPP_IN   , CB_GETCURSEL, 0, 0); if (Result2==CB_ERR) Result2=0;
    LRESULT Result3=SendDlgItemMessage(hDialog, DLG1_CL_TEXTURE_DETAIL_IN, CB_GETCURSEL, 0, 0); if (Result3==CB_ERR) Result3=0;

 // Options_ClientDisplayRes   =int(Result1);
    Options_ClientDisplayBPP   =int(Result2);
    Options_ClientTextureDetail=int(Result3);

    // 3.1. Server: Game Name
    GetDlgItemText(hDialog, DLG1_SV_GAME_IN, ReadStr, sizeof(ReadStr)-1);
    Options_ServerGameName=std::string(ReadStr);

    // 3.2. Server: World Name
    GetDlgItemText(hDialog, DLG1_SV_WORLDNAME_IN, ReadStr, sizeof(ReadStr)-1);
    Options_ServerWorldName=std::string(ReadStr);

    // 3.3. Server: PortNr
    Options_ServerPortNr=int(GetDlgItemInt(hDialog, DLG1_SV_PORT_IN, &ConversionOK, FALSE));
    if (!ConversionOK)
    {
        SetDlgItemText(hDialog, DLG1_STATUSBAR, "Conversion of local server port number failed!");
        return false;
    }

    // 4. Sanity-Checks
    if ((Options_RunMode.GetValueInt()==(CLIENT_RUNMODE | SERVER_RUNMODE)) && (Options_ClientPortNr.GetValueInt()==Options_ServerPortNr.GetValueInt()))
    {
        SetDlgItemText(hDialog, DLG1_STATUSBAR, "Client and server port numbers must be different!");
        return false;
    }

    return true;
}


BOOL CALLBACK CafuMainDialogProcedure(HWND hDialog, UINT MessageID, WPARAM wParam, LPARAM)
{
    HANDLE          SearchHandle;
    WIN32_FIND_DATA FindFileData;

    switch (MessageID)
    {
        case WM_INITDIALOG:
            // 1. Button-Auswahl setzen und dementsprechend die anderen Felder enablen/disablen
            switch (Options_RunMode.GetValueInt())
            {
                // Rufe hier CafuMainDialogProcedure() rekursiv auf, um den gleichen Effekt zu erzielen,
                // als ob schon eine Auswahl getroffen worden wäre. Auf CheckRadioButton() können wir dennoch nicht
                // verzichten, da die automatische Knopfumschaltung bei dieser Rekursion nicht funktionieren kann!
                case CLIENT_RUNMODE:
                    CheckRadioButton(hDialog, DLG1_RM_CLIENTONLY, DLG1_RM_CLIENTANDSERVER, DLG1_RM_CLIENTONLY     );
                    CafuMainDialogProcedure(hDialog, WM_COMMAND, DLG1_RM_CLIENTONLY, 0);
                    break;

                case SERVER_RUNMODE:
                    CheckRadioButton(hDialog, DLG1_RM_CLIENTONLY, DLG1_RM_CLIENTANDSERVER, DLG1_RM_SERVERONLY     );
                    CafuMainDialogProcedure(hDialog, WM_COMMAND, DLG1_RM_SERVERONLY, 0);
                    break;

                default:
                    CheckRadioButton(hDialog, DLG1_RM_CLIENTONLY, DLG1_RM_CLIENTANDSERVER, DLG1_RM_CLIENTANDSERVER);
                    CafuMainDialogProcedure(hDialog, WM_COMMAND, DLG1_RM_CLIENTANDSERVER, 0);
                    break;
            }


            // 2.1. Client: DeathMatch-PlayerName setzen
            SetDlgItemText(hDialog, DLG1_CL_PLAYERNAME_IN, Options_DeathMatchPlayerName.GetValueString().c_str());

            // 2.2. Client: DeathMatch-Models: Setze die Namen der Models im Games/DeathMatch/Models/Players Verzeichnis in die ComboBox ein
            SearchHandle=FindFirstFile("Games/DeathMatch/Models/Players/*.mdl", &FindFileData);
            if (SearchHandle==INVALID_HANDLE_VALUE) EndDialog(hDialog, -2);

            SendDlgItemMessage(hDialog, DLG1_CL_MODELNAME_IN, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDialog, DLG1_CL_MODELNAME_IN, CB_ADDSTRING, 0, (LPARAM)StripExtension(FindFileData.cFileName));

            while (FindNextFile(SearchHandle, &FindFileData))
                SendDlgItemMessage(hDialog, DLG1_CL_MODELNAME_IN, CB_ADDSTRING, 0, (LPARAM)StripExtension(FindFileData.cFileName));

            FindClose(SearchHandle);
            SendDlgItemMessage(hDialog, DLG1_CL_MODELNAME_IN, CB_SELECTSTRING, -1, (LPARAM)Options_DeathMatchModelName.GetValueString().c_str());

            // 2.3. Client: Client Port, Server Name und Port
            SetDlgItemInt (hDialog, DLG1_CL_PORT_IN,       Options_ClientPortNr.GetValueInt(), FALSE);
            SetDlgItemText(hDialog, DLG1_CL_SV_NAME_IN,    Options_ClientRemoteName.GetValueString().c_str());
            SetDlgItemInt (hDialog, DLG1_CL_SV_PORT_IN,    Options_ClientRemotePortNr.GetValueInt(), FALSE);

            // 2.4. Client: Display resolution, BPP, und texture detail.
            SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_RES_IN, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_RES_IN, CB_ADDSTRING, 0, (LPARAM)"1024 * 768   (changeable at the console)");
            SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_RES_IN, CB_SETCURSEL, /*Options_ClientDisplayRes*/ 0, 0);

            SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_BPP_IN, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_BPP_IN, CB_ADDSTRING, 0, (LPARAM)"16 bpp, HiColor   (faster, but fewer colors)");
            SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_BPP_IN, CB_ADDSTRING, 0, (LPARAM)"32 bpp, TrueColor");
            SendDlgItemMessage(hDialog, DLG1_CL_DISPLAY_BPP_IN, CB_SETCURSEL, Options_ClientDisplayBPP.GetValueInt(), 0);

            SendDlgItemMessage(hDialog, DLG1_CL_TEXTURE_DETAIL_IN, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDialog, DLG1_CL_TEXTURE_DETAIL_IN, CB_ADDSTRING, 0, (LPARAM)"high   (256 MB video RAM required!)");
            SendDlgItemMessage(hDialog, DLG1_CL_TEXTURE_DETAIL_IN, CB_ADDSTRING, 0, (LPARAM)"medium   (faster)");
            SendDlgItemMessage(hDialog, DLG1_CL_TEXTURE_DETAIL_IN, CB_ADDSTRING, 0, (LPARAM)"low   (fastest)");
            SendDlgItemMessage(hDialog, DLG1_CL_TEXTURE_DETAIL_IN, CB_SETCURSEL, Options_ClientTextureDetail.GetValueInt(), 0);


            // 3.1. Server: Setze die Namen der Games in die ComboBox ein (d.h. die Namen der Verzeichnisse im
            //      Verzeichnis 'Games', falls dort auch im 'Code' Unterverzeichnis eine DLL mit gleichem Namen existiert)
            SearchHandle=FindFirstFile("Games/*", &FindFileData);
            if (SearchHandle==INVALID_HANDLE_VALUE) EndDialog(hDialog, -3);

            SendDlgItemMessage(hDialog, DLG1_SV_GAME_IN, CB_RESETCONTENT, 0, 0);
            if (IsGameDirectory(FindFileData)) SendDlgItemMessage(hDialog, DLG1_SV_GAME_IN, CB_ADDSTRING, 0, (LPARAM)FindFileData.cFileName);

            while (FindNextFile(SearchHandle, &FindFileData))
                if (IsGameDirectory(FindFileData)) SendDlgItemMessage(hDialog, DLG1_SV_GAME_IN, CB_ADDSTRING, 0, (LPARAM)FindFileData.cFileName);

            FindClose(SearchHandle);
            // Versuche, den Eintrag in 'Options_ServerGameName' zu selektieren. Falls es schiefgeht, selektiere den Eintrag mit Index 0.
            if (SendDlgItemMessage(hDialog, DLG1_SV_GAME_IN, CB_SELECTSTRING, -1, (LPARAM)Options_ServerGameName.GetValueString().c_str())==CB_ERR)
                SendDlgItemMessage(hDialog, DLG1_SV_GAME_IN, CB_SETCURSEL, 0, 0);

            // 3.2. Server: Die ComboBox der World-Names wird in Abhängigkeit vom gewählten Game gefüllt. Dazu rufen wir die
            //      CafuMainDialogProcedure() rekursiv auf, um den gleichen Effekt zu erzielen, als ob vom User schon eine echte
            //      neue Auswahl getroffen worden wäre!
            //      Vorher aber noch den Options_ServerGameName löschen, damit diese 'Selektion' tatsächlich als *neu* erkannt wird!
            Options_ServerGameName=std::string("");
            CafuMainDialogProcedure(hDialog, WM_COMMAND, MAKELONG(DLG1_SV_GAME_IN, CBN_SELENDOK), 0);

            // 3.3. Server: Rest (Server Port)
            SetDlgItemInt (hDialog, DLG1_SV_PORT_IN, Options_ServerPortNr.GetValueInt(), FALSE);


            // 4. Status Bar
            SetDlgItemText(hDialog, DLG1_STATUSBAR, "Status - OK");


            SetFocus(GetDlgItem(hDialog, IDOK));
            return FALSE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case DLG1_RM_CLIENTONLY:
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PLAYERNAME_IN),     TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_CONFIGURE),         TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_MODELNAME_IN),      TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PORT_IN),           TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SV_NAME_IN),        TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SV_PORT_IN),        TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_GETSVINFO),         TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PINGSV),            TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SENDNOP),           TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_DISPLAY_RES_IN),    FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_DISPLAY_BPP_IN),    TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_TEXTURE_DETAIL_IN), TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_GAME_IN),           FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_WORLDNAME_IN),      FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_PORT_IN),           FALSE);
                    return TRUE;

                case DLG1_RM_SERVERONLY:
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PLAYERNAME_IN),     FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_CONFIGURE),         FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_MODELNAME_IN),      FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PORT_IN),           FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SV_NAME_IN),        FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SV_PORT_IN),        FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_GETSVINFO),         FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PINGSV),            FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SENDNOP),           FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_DISPLAY_RES_IN),    FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_DISPLAY_BPP_IN),    FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_TEXTURE_DETAIL_IN), FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_GAME_IN),           TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_WORLDNAME_IN),      TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_PORT_IN),           TRUE );
                    return TRUE;

                case DLG1_RM_CLIENTANDSERVER:
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PLAYERNAME_IN),     TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_CONFIGURE),         TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_MODELNAME_IN),      TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PORT_IN),           TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SV_NAME_IN),        FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SV_PORT_IN),        FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_GETSVINFO),         FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_PINGSV),            FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_SENDNOP),           FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_DISPLAY_RES_IN),    FALSE);
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_DISPLAY_BPP_IN),    TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_CL_TEXTURE_DETAIL_IN), TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_GAME_IN),           TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_WORLDNAME_IN),      TRUE );
                    EnableWindow(GetDlgItem(hDialog, DLG1_SV_PORT_IN),           TRUE );
                    return TRUE;

                case DLG1_SV_GAME_IN:
                {
                    if (HIWORD(wParam)!=CBN_SELENDOK) return FALSE;

                    // Lies den ausgewählten String nach 'NewGameName'
                    char NewGameName[1024];

                    GetDlgItemText(hDialog, DLG1_SV_GAME_IN, NewGameName, sizeof(NewGameName)-1);
                    NewGameName[sizeof(NewGameName)-1]=0;

                    // Falls die neue Auswahl sich nicht von der alten unterscheidet, brauchen wir nichts weiter zu unternehmen.
                    if (!strcmp(NewGameName, Options_ServerGameName.GetValueString().c_str())) return TRUE;

                    // Es gab eine neue Auswahl: Setze die Namen der CW (Cafu World) Files im 'Games/NewGameName/Worlds' Verzeichnis in die 'World name'-ComboBox ein.
                    char TempWorldPath[160];

                    sprintf(TempWorldPath, "Games/%s/Worlds/*.cw", NewGameName);
                    SearchHandle=FindFirstFile(TempWorldPath, &FindFileData);

                    if (SearchHandle==INVALID_HANDLE_VALUE)
                    {
                        // Updaten der CW (Cafu World) Files ('World name'-ComboBox) scheint nicht zu gehen.
                        // Falls es vorher schon mal eine gültige Auswahl gab, selektiere wieder das alte Game. Andernfalls brechen wir ab.
                        // Wir erkennen die Gültigkeit der vorherigen Auswahl am ServerGameName, da wir diesen String beim Init selbst gelöscht haben!
                        if (Options_ServerGameName.GetValueString().length()>0)
                        {
                            MessageBox(NULL, "Sorry, I cannot find worlds for the game you selected!", "Cafu Engine", MB_OK | MB_ICONINFORMATION);
                            SendDlgItemMessage(hDialog, DLG1_SV_GAME_IN, CB_SELECTSTRING, -1, (LPARAM)Options_ServerGameName.GetValueString().c_str());
                        }
                        else EndDialog(hDialog, -4);

                        return TRUE;
                    }

                    SendDlgItemMessage(hDialog, DLG1_SV_WORLDNAME_IN, CB_RESETCONTENT, 0, 0);
                    SendDlgItemMessage(hDialog, DLG1_SV_WORLDNAME_IN, CB_ADDSTRING, 0, (LPARAM)StripExtension(FindFileData.cFileName));

                    while (FindNextFile(SearchHandle, &FindFileData))
                        SendDlgItemMessage(hDialog, DLG1_SV_WORLDNAME_IN, CB_ADDSTRING, 0, (LPARAM)StripExtension(FindFileData.cFileName));

                    FindClose(SearchHandle);
                    if (SendDlgItemMessage(hDialog, DLG1_SV_WORLDNAME_IN, CB_SELECTSTRING, -1, (LPARAM)Options_ServerWorldName.GetValueString().c_str())==CB_ERR)
                        SendDlgItemMessage(hDialog, DLG1_SV_WORLDNAME_IN, CB_SETCURSEL, 0, 0);

                    // Hat alles geklappt, übernimm den NewGameName
                    Options_ServerGameName=std::string(NewGameName);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hDialog, 0);
                    return TRUE;

                case IDOK:
                    if (!ReadDialogSettings(hDialog)) return TRUE;
                    EndDialog(hDialog, 1);
                    return TRUE;

                case DLG1_CL_CONFIGURE:
                    if (!ReadDialogSettings(hDialog)) return TRUE;
                    EndDialog(hDialog, 2);
                    return TRUE;

                case DLG1_CL_GETSVINFO:
                    if (!ReadDialogSettings(hDialog)) return TRUE;
                    EndDialog(hDialog, 3);
                    return TRUE;

                case DLG1_CL_PINGSV:
                    if (!ReadDialogSettings(hDialog)) return TRUE;
                    EndDialog(hDialog, 4);
                    return TRUE;

                case DLG1_CL_SENDNOP:
                    if (!ReadDialogSettings(hDialog)) return TRUE;
                    EndDialog(hDialog, 5);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
#endif
