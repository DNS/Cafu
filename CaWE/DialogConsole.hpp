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

#ifndef CAFU_DIALOG_CONSOLE_HPP_INCLUDED
#define CAFU_DIALOG_CONSOLE_HPP_INCLUDED


class ConsoleDialogT : public wxPanel
{
    public:

    /// Constructor.
    ConsoleDialogT(wxWindow* Parent);

    /// Adds the text s to the console output.
    void Print(const wxString& s, const wxColour* Colour=wxGREEN);


    private:

    wxTextCtrl* ConsoleOutput;
    wxTextCtrl* CommandInput;
    wxString    SavedOutputForNextPrint;

    // Event handlers.
    void OnCommandInput(wxCommandEvent& Event);
    void OnClear       (wxCommandEvent& Event);

    // IDs for the controls whose events we are interested in.
    enum
    {
        ID_TEXTCTRL_INPUT=wxID_HIGHEST+1,
        ID_BUTTON_CLEAR
    };

    DECLARE_EVENT_TABLE()
};

#endif
