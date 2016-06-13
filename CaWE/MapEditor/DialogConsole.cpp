/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "wx/wx.h"
#include "DialogConsole.hpp"


BEGIN_EVENT_TABLE(ConsoleDialogT, wxPanel)
    EVT_TEXT_ENTER(ID_TEXTCTRL_INPUT, ConsoleDialogT::OnCommandInput)
    EVT_BUTTON(ID_BUTTON_CLEAR, ConsoleDialogT::OnClear)
END_EVENT_TABLE()


ConsoleDialogT::ConsoleDialogT(wxWindow* Parent)
    : wxPanel(Parent, -1),
      ConsoleOutput(NULL),
      CommandInput(NULL),
      SavedOutputForNextPrint("")
{
    // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    ConsoleOutput=new wxTextCtrl(this, -1, wxT("Console initialized.\n"), wxDefaultPosition, wxSize(400, 300), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    ConsoleOutput->SetMinSize(wxSize(40, 30));
    ConsoleOutput->SetForegroundColour( *wxGREEN );
    ConsoleOutput->SetBackgroundColour( *wxBLACK );
    item0->Add(ConsoleOutput, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item2 = new wxBoxSizer( wxHORIZONTAL );

    CommandInput=new wxTextCtrl(this, ID_TEXTCTRL_INPUT, wxT(""), wxDefaultPosition, wxSize(80,-1), wxTE_PROCESS_ENTER);
    item2->Add(CommandInput, 1, wxALIGN_CENTER|wxRIGHT, 5 );

    wxButton *item4 = new wxButton(this, ID_BUTTON_CLEAR, wxT("Clear"), wxDefaultPosition, wxSize(48,18), 0 );
    item2->Add( item4, 0, wxGROW|wxALIGN_CENTER_HORIZONTAL, 5 );

    item0->Add( item2, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    this->SetSizer( item0 );
    item0->SetSizeHints(this);
}


/************************/
/*** Public Functions ***/
/************************/

void ConsoleDialogT::Print(const wxString& s, const wxColour* Colour)
{
    wxString text=SavedOutputForNextPrint+s;

    SavedOutputForNextPrint="";
    ConsoleOutput->SetDefaultStyle(wxTextAttr(*Colour));

    // Deal with \r chars (used for progress indications by the compile tools).
    while (true)
    {
        size_t tLen=text.length();
        size_t PosR=text.find('\r');

        if (PosR==wxString::npos) break;    // No more \r chars left in this text.

        // Add everything left of PosR to the ConsoleOutput.
        if (PosR>0) ConsoleOutput->AppendText(text.Left(PosR));

        if (PosR+1<tLen && text[PosR+1]=='\n')
        {
            // It's a regular Windows line break (\r\n).
            ConsoleOutput->AppendText("\n");

            // The new text is everything that is right of PosR+1.
            text=text.Right(tLen-PosR-2);
        }
        else if (PosR+1<tLen && text[PosR+1]!='\n')
        {
            // Okay, this *is* a "true" \r character for progress indication, followed by more regular text.

            // Delete the last line in ConsoleOutput.
            const int NrOfLines =ConsoleOutput->GetNumberOfLines();
            const int LineLength=ConsoleOutput->GetLineLength(NrOfLines-1);
            wxTextPos LastPos   =ConsoleOutput->GetLastPosition();

            if (LineLength>0) ConsoleOutput->Remove(LastPos-LineLength, LastPos);

            // The new text is everything that is right of PosR.
            text=text.Right(tLen-PosR-1);
            wxASSERT(text!="");
        }
        else
        {
            // A true \r character at the end of line.
            // If this case was treated by the more general-case code above, we would get a blank
            // last line until this method is called again, which results in a horrible flicker!
            wxASSERT(PosR+1==tLen);

            SavedOutputForNextPrint="\r";
            // text="";
            return;
        }
    }

    if (text!="") ConsoleOutput->AppendText(text);
}


/**********************/
/*** Event Handlers ***/
/**********************/

void ConsoleDialogT::OnCommandInput(wxCommandEvent& Event)
{
    if (CommandInput->GetValue()=="") return;

    ConsoleOutput->AppendText("\n> "+CommandInput->GetValue()+"\n");
    ConsoleOutput->AppendText("Sorry, command \""+CommandInput->GetValue()+"\" is not yet implemented (no command currently is).\n");

    CommandInput->Clear();
}


void ConsoleDialogT::OnClear(wxCommandEvent& Event)
{
    ConsoleOutput->Clear();
}
