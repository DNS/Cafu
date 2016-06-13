/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
