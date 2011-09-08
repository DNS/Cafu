/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "EditorListBoxWindow.hpp"
#include "../ChildFrame.hpp"
#include "../GuiDocument.hpp"
#include "../Commands/ModifyWindow.hpp"

#include "GuiSys/WindowListBox.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "wx/propgrid/property.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"


using namespace GuiEditor;


namespace
{
    unsigned char* cast(const float* f)
    {
        static unsigned char c[4];

        for (int i=0; i<4; i++)
        {
            float g=f[i];

            if (g<0.0f) g=0.0f;
            if (g>1.0f) g=1.0f;

            c[i]=(unsigned char)(g*255.0f + 0.5f);
        }

        return c;
    }
}


EditorListBoxWindowT::EditorListBoxWindowT(cf::GuiSys::ListBoxT* ListBox, GuiDocumentT* GuiDoc)
    : EditorWindowT(ListBox, GuiDoc),
      m_ListBox(ListBox)
{
}


void EditorListBoxWindowT::FillInPG(wxPropertyGridManager* PropMan)
{
    EditorWindowT::FillInPG(PropMan);

    PropMan->Append(new wxFloatProperty("RowHeight", wxPG_LABEL, m_ListBox->GetRowHeight()));

    unsigned char* Col=cast(m_ListBox->GetOddRowBgColor());
    PropMan->Append(new wxColourProperty("OddRowBgColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("OddRowBgColorAlpha", wxPG_LABEL, Col[3]));

    Col=cast(m_ListBox->GetEvenRowBgColor());
    PropMan->Append(new wxColourProperty("EvenRowBgColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("EvenRowBgColorAlpha", wxPG_LABEL, Col[3]));

    Col=cast(m_ListBox->GetRowTextColor());
    PropMan->Append(new wxColourProperty("RowTextColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("RowTextColorAlpha", wxPG_LABEL, Col[3]));

    Col=cast(m_ListBox->GetSelectedRowBgColor());
    PropMan->Append(new wxColourProperty("SelectedRowBgColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("SelectedRowBgColorAlpha", wxPG_LABEL, Col[3]));

    Col=cast(m_ListBox->GetSelectedRowTextColor());
    PropMan->Append(new wxColourProperty("SelectedRowTextColor", wxPG_LABEL, wxColour(Col[0], Col[1], Col[2])));
    PropMan->Append(new wxIntProperty("SelectedRowTextColorAlpha", wxPG_LABEL, Col[3]));
}


bool EditorListBoxWindowT::UpdateProperty(wxPGProperty* Property)
{
    if (EditorWindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

         if (PropName=="RowHeight")                 { Property->SetValue(m_ListBox->GetRowHeight()); }
    else if (PropName=="OddRowBgColor")             { unsigned char* Col=cast(m_ListBox->GetOddRowBgColor());        Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="OddRowBgColorAlpha")        { unsigned char* Col=cast(m_ListBox->GetOddRowBgColor());        Property->SetValue(Col[3]); }
    else if (PropName=="EvenRowBgColor")            { unsigned char* Col=cast(m_ListBox->GetEvenRowBgColor());       Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="EvenRowBgColorAlpha")       { unsigned char* Col=cast(m_ListBox->GetEvenRowBgColor());       Property->SetValue(Col[3]); }
    else if (PropName=="RowTextColor")              { unsigned char* Col=cast(m_ListBox->GetRowTextColor());         Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="RowTextColorAlpha")         { unsigned char* Col=cast(m_ListBox->GetRowTextColor());         Property->SetValue(Col[3]); }
    else if (PropName=="SelectedRowBgColor")        { unsigned char* Col=cast(m_ListBox->GetSelectedRowBgColor());   Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="SelectedRowBgColorAlpha")   { unsigned char* Col=cast(m_ListBox->GetSelectedRowBgColor());   Property->SetValue(Col[3]); }
    else if (PropName=="SelectedRowTextColor")      { unsigned char* Col=cast(m_ListBox->GetSelectedRowTextColor()); Property->SetValueFromString(wxString::Format("(%i,%i,%i)", Col[0], Col[1], Col[2])); }
    else if (PropName=="SelectedRowTextColorAlpha") { unsigned char* Col=cast(m_ListBox->GetSelectedRowTextColor()); Property->SetValue(Col[3]); }
    else                                            { return false; }

    return true;
}


bool EditorListBoxWindowT::HandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (EditorWindowT::HandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty* Prop    =Event.GetProperty();
    const wxString      PropName=Prop->GetName();

    if (PropName=="RowHeight")
    {
        wxASSERT(m_ListBox->GetMemberVar("rowHeight").Member!=NULL);
        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("rowHeight"), Prop->GetValue().GetLong()));
    }
    else if (PropName=="OddRowBgColor")
    {
        wxASSERT(m_ListBox->GetMemberVar("oddRowBgColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_ListBox->GetOddRowBgColor()[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("oddRowBgColor"), NewValue));
    }
    else if (PropName=="OddRowBgColorAlpha")
    {
        wxASSERT(m_ListBox->GetMemberVar("oddRowBgColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("oddRowBgColor.a"), &NewValue));
    }
    else if (PropName=="EvenRowBgColor")
    {
        wxASSERT(m_ListBox->GetMemberVar("evenRowBgColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_ListBox->GetEvenRowBgColor()[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("evenRowBgColor"), NewValue));
    }
    else if (PropName=="EvenRowBgColorAlpha")
    {
        wxASSERT(m_ListBox->GetMemberVar("evenRowBgColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("evenRowBgColor.a"), &NewValue));
    }
    else if (PropName=="RowTextColor")
    {
        wxASSERT(m_ListBox->GetMemberVar("rowTextColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_ListBox->GetRowTextColor()[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("rowTextColor"), NewValue));
    }
    else if (PropName=="RowTextColorAlpha")
    {
        wxASSERT(m_ListBox->GetMemberVar("rowTextColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("rowTextColor.a"), &NewValue));
    }
    else if (PropName=="SelectedRowBgColor")
    {
        wxASSERT(m_ListBox->GetMemberVar("selectedRowBgColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_ListBox->GetRowTextColor()[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("selectedRowBgColor"), NewValue));
    }
    else if (PropName=="SelectedRowBgColorAlpha")
    {
        wxASSERT(m_ListBox->GetMemberVar("selectedRowBgColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("selectedRowBgColor.a"), &NewValue));
    }
    else if (PropName=="SelectedRowTextColor")
    {
        wxASSERT(m_ListBox->GetMemberVar("selectedRowTextColor").Member!=NULL);

        wxColour NewColor; NewColor << Prop->GetValue();
        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, m_ListBox->GetRowTextColor()[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("selectedRowTextColor"), NewValue));
    }
    else if (PropName=="SelectedRowTextColorAlpha")
    {
        wxASSERT(m_ListBox->GetMemberVar("selectedRowTextColor.a").Member!=NULL);

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(m_GuiDoc, m_ListBox, PropName, m_ListBox->GetMemberVar("selectedRowTextColor.a"), &NewValue));
    }
    else
    {
        return false;
    }

    return true;
}


namespace
{
    void WriteColors(std::ostream& OutFile, const char* func, const float* Col, const float* DefCol)
    {
        if (Col[0]!=DefCol[0] || Col[1]!=DefCol[1] || Col[2]!=DefCol[2] || Col[3]!=DefCol[3])
            OutFile << "    " << func << "(" << Col[0] << ", " << Col[1] << ", " << Col[2] << ", " << Col[3] << ");\n";
    }
}


bool EditorListBoxWindowT::WriteInitMethod(std::ostream& OutFile)
{
    EditorWindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    cf::GuiSys::WindowCreateParamsT Params(*m_GuiDoc->GetGui());
    cf::GuiSys::ListBoxT            Default(Params);

    if (m_ListBox->GetRowHeight()!=Default.GetRowHeight())
        OutFile << "    self:SetRowHeight(" << m_ListBox->GetRowHeight() << ");\n";

    WriteColors(OutFile, "self:SetOddRowBgColor",   m_ListBox->GetOddRowBgColor(),        Default.GetOddRowBgColor());
    WriteColors(OutFile, "self:SetEvenRowBgColor",  m_ListBox->GetEvenRowBgColor(),       Default.GetEvenRowBgColor());
    WriteColors(OutFile, "self:SetRowTextColor",    m_ListBox->GetRowTextColor(),         Default.GetRowTextColor());
    WriteColors(OutFile, "self:SetSelRowBgColor",   m_ListBox->GetSelectedRowBgColor(),   Default.GetSelectedRowBgColor());
    WriteColors(OutFile, "self:SetSelRowTextColor", m_ListBox->GetSelectedRowTextColor(), Default.GetSelectedRowTextColor());

    return true;
}
