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

/// @file GuiSysEditor.cpp
/// Contains implementation of the editor related methods of each GuiSys class.
#include "EditorData/Window.hpp"

#include "GuiDocument.hpp"
#include "ChildFrame.hpp"

#include "Commands/ModifyWindow.hpp"

#include "../MaterialBrowser/MaterialBrowserDialog.hpp"
#include "../EditorMaterial.hpp"
#include "../EditorMaterialManager.hpp"
#include "../GameConfig.hpp"

#include "GuiSys/GuiMan.hpp"
#include "GuiSys/Window.hpp"
#include "GuiSys/WindowChoice.hpp"
#include "GuiSys/WindowEdit.hpp"
#include "GuiSys/WindowListBox.hpp"
#include "GuiSys/WindowModel.hpp"
#include "GuiSys/WindowCreateParams.hpp"

#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/Mesh.hpp"

#include "Fonts/FontTT.hpp"

#include "wx/wx.h"
#include "wx/regex.h"
#include "wx/propgrid/propgrid.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"

#include <fstream>


using namespace cf::GuiSys;
using namespace GuiEditor;


static const float SelectionColor[]={ 1.0, 0.0, 0.0, 1.0 };
static const float SelectionBorder =2;


static wxColour FromPGColorString(wxString ColorString)
{
    return wxColour("RGB"+ColorString);
}


/// Custom property to select materials for window backgrounds.
class MaterialPropertyT : public wxLongStringProperty
{
    public:

    MaterialPropertyT(const wxString& name,
                      const wxString& label,
                      const wxString& value,
                      GuiDocumentT* GuiDoc)
        : wxLongStringProperty(name, label, value),
          m_GuiDocument(GuiDoc)
    {
    }

    // Shows the file selection dialog and makes the choosen file path relative.
    virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
    {
        MaterialBrowserDialogT MatBrowser(GetGrid(), m_GuiDocument, m_GuiDocument->GetGameConfig()->GetMatMan().FindMaterial(GetValueAsString(), false), "", false);
        if (MatBrowser.ShowModal()!=wxID_OK) return false;

        EditorMaterialI* Mat=MatBrowser.GetCurrentMaterial();
        if (Mat==NULL) return false;

        value=Mat->GetName();
        return true;
    }


    private:

    GuiDocumentT* m_GuiDocument;
};


void WindowT::EditorFillInPG(wxPropertyGridManager* PropMan)
{
    PropMan->Append(new wxStringProperty("Name", wxPG_LABEL, Name));

    wxPGProperty* Item=PropMan->Append(new wxBoolProperty("Visible", wxPG_LABEL, ShowWindow));

    PropMan->SetPropertyAttribute(Item, wxPG_BOOL_USE_CHECKBOX, (long)1, wxPG_RECURSE); // Use checkboxes instead of choice.

    wxPGProperty* Position=PropMan->Append(new wxStringProperty("Position", wxPG_LABEL, "<composed>"));

    PropMan->AppendIn(Position, new wxFloatProperty("X", wxPG_LABEL, Rect[0]));
    PropMan->AppendIn(Position, new wxFloatProperty("Y", wxPG_LABEL, Rect[1]));

    wxPGProperty* Size=PropMan->Append(new wxStringProperty("Size", wxPG_LABEL, "<composed>"));

    PropMan->AppendIn(Size, new wxFloatProperty("Width", wxPG_LABEL, Rect[2]));
    PropMan->AppendIn(Size, new wxFloatProperty("Height", wxPG_LABEL, Rect[3]));

    PropMan->Append(new wxFloatProperty("Rotation", wxPG_LABEL, RotAngle));

    PropMan->Append(new MaterialPropertyT("BackMatName", wxPG_LABEL, BackRenderMatName, ((EditorDataWindowT*)EditorData)->GetGuiDoc()));

    PropMan->Append(new wxColourProperty("BackgroundColor", wxPG_LABEL, wxColour(BackColor[0]*255, BackColor[1]*255, BackColor[2]*255)));

    PropMan->Append(new wxIntProperty("BackgroundAlpha", wxPG_LABEL, BackColor[3]*255));

    PropMan->Append(new wxFloatProperty("BorderWidth", wxPG_LABEL, BorderWidth));

    PropMan->Append(new wxColourProperty("BorderColor", wxPG_LABEL, wxColour(BorderColor[0]*255, BorderColor[1]*255, BorderColor[2]*255)));

    PropMan->Append(new wxIntProperty("BorderColorAlpha", wxPG_LABEL, BorderColor[3]*255));

    PropMan->Append(new wxStringProperty("FontName", wxPG_LABEL, Font->GetName()));

    PropMan->Append(new wxLongStringProperty("Text", wxPG_LABEL, Text));

    PropMan->Append(new wxFloatProperty("TextScale", wxPG_LABEL, TextScale));

    PropMan->Append(new wxColourProperty("TextColor", wxPG_LABEL, wxColour(TextColor[0]*255, TextColor[1]*255, TextColor[2]*255)));

    PropMan->Append(new wxIntProperty("TextColorAlpha", wxPG_LABEL, TextColor[3]*255));

    wxPGChoices AlignHorChoices;
    wxPGChoices AlignVerChoices;

    AlignHorChoices.Add("Left");
    AlignHorChoices.Add("Right");
    AlignHorChoices.Add("Center");

    AlignVerChoices.Add("Top");
    AlignVerChoices.Add("Bottom");
    AlignVerChoices.Add("Middle");

    PropMan->Append(new wxEnumProperty("HorizontalAlign", wxPG_LABEL, AlignHorChoices, (int)TextAlignHor));
    PropMan->Append(new wxEnumProperty("VerticalAlign",   wxPG_LABEL, AlignVerChoices, (int)TextAlignVer));
}


void ChoiceT::EditorFillInPG(wxPropertyGridManager* PropMan)
{
    WindowT::EditorFillInPG(PropMan);

    wxString ChoicesString;

    for (unsigned long ChoiceNr=0; ChoiceNr<Choices.Size(); ChoiceNr++)
        ChoicesString+=Choices[ChoiceNr]+";\n";

    PropMan->Append(new wxLongStringProperty("Choices", wxPG_LABEL, ChoicesString));

    PropMan->Append(new wxUIntProperty("DefaultChoice", wxPG_LABEL, SelectedChoice));
}


void EditWindowT::EditorFillInPG(wxPropertyGridManager* PropMan)
{
    WindowT::EditorFillInPG(PropMan);

    wxPGChoices CursorTypes;
    CursorTypes.Add("|");
    CursorTypes.Add("_");

    PropMan->Append(new wxEnumProperty("TextCursorType", wxPG_LABEL, CursorTypes, (int)TextCursorType));

    PropMan->Append(new wxFloatProperty("TextCursorRate", wxPG_LABEL, TextCursorRate));

    PropMan->Append(new wxColourProperty("TextCursorColor", wxPG_LABEL, wxColour(TextCursorColor[0]*255, TextCursorColor[1]*255, TextCursorColor[2]*255)));

    PropMan->Append(new wxIntProperty("TextCursorColorAlpha", wxPG_LABEL, TextCursorColor[3]*255));
}


void ListBoxT::EditorFillInPG(wxPropertyGridManager* PropMan)
{
    WindowT::EditorFillInPG(PropMan);

    PropMan->Append(new wxFloatProperty("RowHeight", wxPG_LABEL, RowHeight));

    PropMan->Append(new wxColourProperty("OddRowBgColor", wxPG_LABEL, wxColour(OddRowBgColor[0]*255, OddRowBgColor[1]*255, OddRowBgColor[2]*255)));

    PropMan->Append(new wxIntProperty("OddRowBgColorAlpha", wxPG_LABEL, OddRowBgColor[3]*255));

    PropMan->Append(new wxColourProperty("EvenRowBgColor", wxPG_LABEL, wxColour(EvenRowBgColor[0]*255, EvenRowBgColor[1]*255, EvenRowBgColor[2]*255)));

    PropMan->Append(new wxIntProperty("EvenRowBgColorAlpha", wxPG_LABEL, EvenRowBgColor[3]*255));

    PropMan->Append(new wxColourProperty("RowTextColor", wxPG_LABEL, wxColour(RowTextColor[0]*255, RowTextColor[1]*255, RowTextColor[2]*255)));

    PropMan->Append(new wxIntProperty("RowTextColorAlpha", wxPG_LABEL, RowTextColor[3]*255));

    PropMan->Append(new wxColourProperty("SelectedRowBgColor", wxPG_LABEL, wxColour(SelectedRowBgColor[0]*255, SelectedRowBgColor[1]*255, SelectedRowBgColor[2]*255)));

    PropMan->Append(new wxIntProperty("SelectedRowBgColorAlpha", wxPG_LABEL, SelectedRowBgColor[3]*255));

    PropMan->Append(new wxColourProperty("SelectedRowTextColor", wxPG_LABEL, wxColour(SelectedRowTextColor[0]*255, SelectedRowTextColor[1]*255, SelectedRowTextColor[2]*255)));

    PropMan->Append(new wxIntProperty("SelectedRowTextColorAlpha", wxPG_LABEL, SelectedRowTextColor[3]*255));
}


void ModelWindowT::EditorFillInPG(wxPropertyGridManager* PropMan)
{
    WindowT::EditorFillInPG(PropMan);

    PropMan->Append(new wxStringProperty("Model", wxPG_LABEL, Model.GetFileName()));

    PropMan->Append(new wxIntProperty("ModelSequNr", wxPG_LABEL, ModelSequNr));

    wxPGProperty* Vector=PropMan->Append(new wxStringProperty("ModelPos", wxPG_LABEL, "<composed>"));
    PropMan->AppendIn(Vector, new wxFloatProperty("x", wxPG_LABEL, ModelPos.x));
    PropMan->AppendIn(Vector, new wxFloatProperty("y", wxPG_LABEL, ModelPos.y));
    PropMan->AppendIn(Vector, new wxFloatProperty("z", wxPG_LABEL, ModelPos.z));

    PropMan->Append(new wxFloatProperty("ModelScale", wxPG_LABEL, ModelScale));

    Vector=PropMan->Append(new wxStringProperty("ModelAngles", wxPG_LABEL, "<composed>"));
    PropMan->AppendIn(Vector, new wxFloatProperty("x", wxPG_LABEL, ModelAngles.x));
    PropMan->AppendIn(Vector, new wxFloatProperty("y", wxPG_LABEL, ModelAngles.y));
    PropMan->AppendIn(Vector, new wxFloatProperty("z", wxPG_LABEL, ModelAngles.z));

    Vector=PropMan->Append(new wxStringProperty("CameraPos", wxPG_LABEL, "<composed>"));
    PropMan->AppendIn(Vector, new wxFloatProperty("x", wxPG_LABEL, CameraPos.x));
    PropMan->AppendIn(Vector, new wxFloatProperty("y", wxPG_LABEL, CameraPos.y));
    PropMan->AppendIn(Vector, new wxFloatProperty("z", wxPG_LABEL, CameraPos.z));
}


bool WindowT::UpdateProperty(wxPGProperty* Property)
{
    wxString PropName=Property->GetName();

         if (PropName=="Name")             Property->SetValueFromString(Name);
    else if (PropName=="Visible")          Property->SetValueFromString(ShowWindow ? "true" : "false");
    else if (PropName=="Position.X")       Property->SetValue(Rect[0]);
    else if (PropName=="Position.Y")       Property->SetValue(Rect[1]);
    else if (PropName=="Size.Width")       Property->SetValue(Rect[2]);
    else if (PropName=="Size.Height")      Property->SetValue(Rect[3]);
    else if (PropName=="Rotation")         Property->SetValue(RotAngle);
    else if (PropName=="BackMatName")      Property->SetValueFromString(BackRenderMatName);
    else if (PropName=="BackgroundColor")  Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(BackColor[0]*255.0f), int(BackColor[1]*255.0f), int(BackColor[2]*255.0f)));
    else if (PropName=="BackgroundAlpha")  Property->SetValue(int(BackColor[3]*255.0f));
    else if (PropName=="BorderWidth")      Property->SetValue(BorderWidth);
    else if (PropName=="BorderColor")      Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(BorderColor[0]*255.0f), int(BorderColor[1]*255.0f), int(BorderColor[2]*255.0f)));
    else if (PropName=="BorderColorAlpha") Property->SetValue(int(BorderColor[3]*255.0f));
    else if (PropName=="FontName")         Property->SetValueFromString(Font->GetName());
    else if (PropName=="Text")             Property->SetValueFromString(Text);
    else if (PropName=="TextScale")        Property->SetValue(TextScale);
    else if (PropName=="TextColor")        Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(TextColor[0]*255.0f), int(TextColor[1]*255.0f), int(TextColor[2]*255.0f)));
    else if (PropName=="TextColorAlpha")   Property->SetValue(int(TextColor[3]*255.0f));
    else if (PropName=="HorizontalAlign")  Property->SetValue((int)TextAlignHor);
    else if (PropName=="VerticalAlign")    Property->SetValue((int)TextAlignVer);
    else                                   return false;

    return true;
}


bool ChoiceT::UpdateProperty(wxPGProperty* Property)
{
    if (WindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

    if (PropName=="Choices")
    {
        wxString ChoicesString;

        for (unsigned long ChoiceNr=0; ChoiceNr<Choices.Size(); ChoiceNr++)
            ChoicesString+=Choices[ChoiceNr]+";\n";

        Property->SetValueFromString(ChoicesString);
    }
    else if (PropName=="DefaultChoice") Property->SetValue((int)SelectedChoice);
    else                                return false;

    return true;
}


bool EditWindowT::UpdateProperty(wxPGProperty* Property)
{
    if (WindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

    if (PropName=="TextCursorType")
    {
        int Selection=0;
        if (TextCursorType=='_') Selection=1;

        Property->SetValue(Selection);
    }
    else if (PropName=="TextCursorRate")       Property->SetValue(TextCursorRate);
    else if (PropName=="TextCursorColor")      Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(TextCursorColor[0]*255.0f), int(TextCursorColor[1]*255.0f), int(TextCursorColor[2]*255.0f)));
    else if (PropName=="TextCursorColorAlhpa") Property->SetValue(int(TextCursorColor[3]*255.0f));
    else                                       return false;

    return true;
}


bool ListBoxT::UpdateProperty(wxPGProperty* Property)
{
    if (WindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

         if (PropName=="RowHeight")                 Property->SetValue(RowHeight);
    else if (PropName=="OddRowBgColor")             Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(OddRowBgColor[0]*255.0f), int(OddRowBgColor[1]*255.0f), int(OddRowBgColor[2]*255.0f)));
    else if (PropName=="OddRowBgColorAlpha")        Property->SetValue(int(OddRowBgColor[3]*255.0f));
    else if (PropName=="EvenRowBgColor")            Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(EvenRowBgColor[0]*255.0f), int(EvenRowBgColor[1]*255.0f), int(EvenRowBgColor[2]*255.0f)));
    else if (PropName=="EvenRowBgColorAlpha")       Property->SetValue(int(EvenRowBgColor[3]*255.0f));
    else if (PropName=="RowTextColor")              Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(RowTextColor[0]*255.0f), int(RowTextColor[1]*255.0f), int(RowTextColor[2]*255.0f)));
    else if (PropName=="RowTextColorAlpha")         Property->SetValue(int(RowTextColor[3]*255.0f));
    else if (PropName=="SelectedRowBgColor")        Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(SelectedRowBgColor[0]*255.0f), int(SelectedRowBgColor[1]*255.0f), int(SelectedRowBgColor[2]*255.0f)));
    else if (PropName=="SelectedRowBgColorAlpha")   Property->SetValue(int(SelectedRowBgColor[3]*255.0f));
    else if (PropName=="SelectedRowTextColor")      Property->SetValueFromString(wxString::Format("(%i,%i,%i)", int(SelectedRowTextColor[0]*255.0f), int(SelectedRowTextColor[1]*255.0f), int(SelectedRowTextColor[2]*255.0f)));
    else if (PropName=="SelectedRowTextColorAlpha") Property->SetValue(int(SelectedRowTextColor[3]*255.0f));
    else                                            return false;

    return true;
}


bool ModelWindowT::UpdateProperty(wxPGProperty* Property)
{
    if (WindowT::UpdateProperty(Property)) return true;

    wxString PropName=Property->GetName();

         if (PropName=="Model")         Property->SetValueFromString(Model.GetFileName());
    else if (PropName=="ModelSequNr")   Property->SetValue(ModelSequNr);
    else if (PropName=="ModelPos.x")    Property->SetValue(ModelPos.x);
    else if (PropName=="ModelPos.y")    Property->SetValue(ModelPos.y);
    else if (PropName=="ModelPos.z")    Property->SetValue(ModelPos.z);
    else if (PropName=="ModelScale")    Property->SetValue(ModelScale);
    else if (PropName=="ModelAngles.x") Property->SetValue(ModelAngles.x);
    else if (PropName=="ModelAngles.y") Property->SetValue(ModelAngles.y);
    else if (PropName=="ModelAngles.z") Property->SetValue(ModelAngles.z);
    else if (PropName=="CameraPos.x")   Property->SetValue(CameraPos.x);
    else if (PropName=="CameraPos.y")   Property->SetValue(CameraPos.y);
    else if (PropName=="CameraPos.z")   Property->SetValue(CameraPos.z);
    else                                return false;

    return true;
}


static cf::GuiSys::WindowT::MemberVarT DummyVar;


bool WindowT::EditorHandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    wxASSERT(EditorData!=NULL);

    const wxPGProperty*      Prop       =Event.GetProperty();
    const wxString           PropName   =Prop->GetName();
    double                   PropValueD =0.0;
    const float              PropValueF =Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;
    GuiEditor::GuiDocumentT* GuiDocument=((EditorDataWindowT*)EditorData)->GetGuiDoc();

    if (PropName=="Name")
    {
        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, DummyVar, Prop->GetValueAsString()));
    }
    else if (PropName=="Visible")
    {
        wxASSERT(MemberVars.find("show")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["show"], Prop->GetValue().GetBool()));
    }
    else if (PropName=="Position.X")
    {
        wxASSERT(MemberVars.find("pos.x")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["pos.x"], &PropValueF));
    }
    else if (PropName=="Position.Y")
    {
        wxASSERT(MemberVars.find("pos.y")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["pos.y"], &PropValueF));
    }
    else if (PropName=="Size.Width")
    {
        wxASSERT(MemberVars.find("size.x")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["size.x"], &PropValueF));
    }
    else if (PropName=="Size.Height")
    {
        wxASSERT(MemberVars.find("size.y")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["size.y"], &PropValueF));
    }
    else if (PropName=="Rotation")
    {
        wxASSERT(MemberVars.find("rotAngle")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["rotAngle"], &PropValueF));
    }
    else if (PropName=="BackMatName")
    {
        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, DummyVar, Prop->GetValueAsString()));
    }
    else if (PropName=="BackgroundColor")
    {
        wxASSERT(MemberVars.find("backColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, BackColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["backColor"], NewValue));
    }
    else if (PropName=="BackgroundAlpha")
    {
        wxASSERT(MemberVars.find("backColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["backColor.a"], &NewValue));
    }
    else if (PropName=="BorderWidth")
    {
        wxASSERT(MemberVars.find("borderWidth")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["borderWidth"], &PropValueF));
    }
    else if (PropName=="BorderColor")
    {
        wxASSERT(MemberVars.find("borderColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, BorderColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["borderColor"], NewValue));
    }
    else if (PropName=="BorderColorAlpha")
    {
        wxASSERT(MemberVars.find("borderColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["borderColor.a"], &NewValue));
    }
    else if (PropName=="FontName")
    {
        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, DummyVar, Prop->GetValueAsString()));
    }
    else if (PropName=="Text")
    {
        wxASSERT(MemberVars.find("text")!=MemberVars.end());

        wxString FormattedText=Prop->GetValueAsString();
        FormattedText.Replace("\\n", "\n");

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["text"], FormattedText));
    }
    else if (PropName=="TextScale")
    {
        wxASSERT(MemberVars.find("textScale")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textScale"], &PropValueF));
    }
    else if (PropName=="TextColor")
    {
        wxASSERT(MemberVars.find("textColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, TextColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textColor"], NewValue));
    }
    else if (PropName=="TextColorAlpha")
    {
        wxASSERT(MemberVars.find("textColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textColor.a"], &NewValue));
    }
    else if (PropName=="HorizontalAlign")
    {
        wxASSERT(MemberVars.find("textAlignHor")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textAlignHor"], Prop->GetValue().GetLong()));
    }
    else if (PropName=="VerticalAlign")
    {
        wxASSERT(MemberVars.find("textAlignVer")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textAlignVer"], Prop->GetValue().GetLong()));
    }
    else
    {
        return false;
    }

    return true;
}


bool ChoiceT::EditorHandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (WindowT::EditorHandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty*      Prop       =Event.GetProperty();
    const wxString           PropName   =Prop->GetName();
    GuiEditor::GuiDocumentT* GuiDocument=((EditorDataWindowT*)EditorData)->GetGuiDoc();

    if (PropName=="Choices")
    {
        // Remove all newline characters from the string since they break the choices display.
        wxString FormattedString=Prop->GetValueAsString();
        FormattedString.Replace("\\n", "");
        // Remove all trainling characters after the last delimiter (;).
        FormattedString=FormattedString.Mid(0, FormattedString.Find(';', true)+1);

        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, DummyVar, FormattedString));
    }
    else if (PropName=="DefaultChoice")
    {
        wxASSERT(MemberVars.find("defaultChoice")!=MemberVars.end());

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["defaultChoice"], Prop->GetValue().GetLong()));
    }
    else
    {
        return false;
    }

    return true;
}


bool EditWindowT::EditorHandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (WindowT::EditorHandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty*      Prop       =Event.GetProperty();
    const wxString           PropName   =Prop->GetName();
    double                   PropValueD =0.0;
    const float              PropValueF =Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;
    GuiEditor::GuiDocumentT* GuiDocument=((EditorDataWindowT*)EditorData)->GetGuiDoc();

    if (PropName=="TextCursorType")
    {
        wxASSERT(MemberVars.find("textCursorType")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textCursorType"], Prop->GetValue().GetLong()));
    }
    else if (PropName=="TextCursorRate")
    {
        wxASSERT(MemberVars.find("textCursorRate")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textCursorRate"], &PropValueF));
    }
    else if (PropName=="TextCursorColor")
    {
        wxASSERT(MemberVars.find("textCursorColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, TextCursorColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textCursorColor"], NewValue));
    }
    else if (PropName=="TextCursorColorAlpha")
    {
        wxASSERT(MemberVars.find("textCursorColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["textCursorColor.a"], &NewValue));
    }
    else
    {
        return false;
    }

    return true;
}


bool ListBoxT::EditorHandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (WindowT::EditorHandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty*      Prop       =Event.GetProperty();
    const wxString           PropName   =Prop->GetName();
    GuiEditor::GuiDocumentT* GuiDocument=((EditorDataWindowT*)EditorData)->GetGuiDoc();

    if (PropName=="RowHeight")
    {
        wxASSERT(MemberVars.find("rowHeight")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["rowHeight"], Prop->GetValue().GetLong()));
    }
    else if (PropName=="OddRowBgColor")
    {
        wxASSERT(MemberVars.find("oddRowBgColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, OddRowBgColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["oddRowBgColor"], NewValue));
    }
    else if (PropName=="OddRowBgColorAlpha")
    {
        wxASSERT(MemberVars.find("oddRowBgColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["oddRowBgColor.a"], &NewValue));
    }
    else if (PropName=="EvenRowBgColor")
    {
        wxASSERT(MemberVars.find("evenRowBgColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, EvenRowBgColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["evenRowBgColor"], NewValue));
    }
    else if (PropName=="EvenRowBgColorAlpha")
    {
        wxASSERT(MemberVars.find("evenRowBgColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["evenRowBgColor.a"], &NewValue));
    }
    else if (PropName=="RowTextColor")
    {
        wxASSERT(MemberVars.find("rowTextColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, RowTextColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["rowTextColor"], NewValue));
    }
    else if (PropName=="RowTextColorAlpha")
    {
        wxASSERT(MemberVars.find("rowTextColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["rowTextColor.a"], &NewValue));
    }
    else if (PropName=="SelectedRowBgColor")
    {
        wxASSERT(MemberVars.find("selectedRowBgColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, RowTextColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["selectedRowBgColor"], NewValue));
    }
    else if (PropName=="SelectedRowBgColorAlpha")
    {
        wxASSERT(MemberVars.find("selectedRowBgColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["selectedRowBgColor.a"], &NewValue));
    }
    else if (PropName=="SelectedRowTextColor")
    {
        wxASSERT(MemberVars.find("selectedRowTextColor")!=MemberVars.end());

        wxColour NewColor(FromPGColorString(Prop->GetValueAsString()));

        float NewValue[]={ NewColor.Red()/255.0f, NewColor.Green()/255.0f, NewColor.Blue()/255.0f, RowTextColor[3] };

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["selectedRowTextColor"], NewValue));
    }
    else if (PropName=="SelectedRowTextColorAlpha")
    {
        wxASSERT(MemberVars.find("selectedRowTextColor.a")!=MemberVars.end());

        float NewValue=float(Prop->GetValue().GetLong())/255.0f;

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["selectedRowTextColor.a"], &NewValue));
    }
    else
    {
        return false;
    }

    return true;
}


bool ModelWindowT::EditorHandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame)
{
    if (WindowT::EditorHandlePGChange(Event, ChildFrame)) return true;

    const wxPGProperty*      Prop       =Event.GetProperty();
    const wxString           PropName   =Prop->GetName();
    double                   PropValueD =0.0;
    const float              PropValueF =Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;
    GuiEditor::GuiDocumentT* GuiDocument=((EditorDataWindowT*)EditorData)->GetGuiDoc();

    if (PropName=="Model")
    {
        // Specially treated by command.
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, DummyVar, Prop->GetValueAsString()));
    }
    else if (PropName=="ModelSequNr")
    {
        wxASSERT(MemberVars.find("modelSequNr")!=MemberVars.end());

        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelSequNr"], Prop->GetValue().GetLong()));
    }
    else if (PropName==("ModelPos.x"))
    {
        wxASSERT(MemberVars.find("modelPos.x")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelPos.x"], &PropValueF));
    }
    else if (PropName==("ModelPos.y"))
    {
        wxASSERT(MemberVars.find("modelPos.y")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelPos.y"], &PropValueF));
    }
    else if (PropName==("ModelPos.z"))
    {
        wxASSERT(MemberVars.find("modelPos.z")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelPos.z"], &PropValueF));
    }
    else if (PropName=="ModelScale")
    {
        wxASSERT(MemberVars.find("modelScale")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelScale"], &PropValueF));
    }
    else if (PropName==("ModelAngles.x"))
    {
        wxASSERT(MemberVars.find("modelAngles.x")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelAngles.x"], &PropValueF));
    }
    else if (PropName==("ModelAngles.y"))
    {
        wxASSERT(MemberVars.find("modelAngles.y")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelAngles.y"], &PropValueF));
    }
    else if (PropName==("ModelAngles.z"))
    {
        wxASSERT(MemberVars.find("modelAngles.z")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["modelAngles.z"], &PropValueF));
    }
    else if (PropName==("CameraPos.x"))
    {
        wxASSERT(MemberVars.find("cameraPos.x")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["cameraPos.x"], &PropValueF));
    }
    else if (PropName==("CameraPos.y"))
    {
        wxASSERT(MemberVars.find("cameraPos.y")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["cameraPos.y"], &PropValueF));
    }
    else if (PropName==("CameraPos.z"))
    {
        wxASSERT(MemberVars.find("cameraPos.z")!=MemberVars.end());
        ChildFrame->SubmitCommand(new CommandModifyWindowT(GuiDocument, this, PropName, MemberVars["cameraPos.z"], &PropValueF));
    }
    else
    {
        return false;
    }

    return true;
}


bool WindowT::WriteInitMethod(std::ostream& OutFile)
{
    // Create empty window to get default values.
    WindowCreateParamsT Params(Gui);
    WindowT             Default(Params);

    if (ShowWindow!=Default.ShowWindow)
        OutFile << "    self:set(\"show\", " << (ShowWindow ? "true" : "false") << ");\n";

    if (Rect[0]!=Default.Rect[0] || Rect[1]!=Default.Rect[1] || Rect[2]!=Default.Rect[2] || Rect[3]!=Default.Rect[3])
        OutFile << "    self:set(\"rect\", " << Rect[0] << ", " << Rect[1] << ", " << Rect[2] << ", " << Rect[3] << ");\n";

    if (RotAngle!=Default.RotAngle)
        OutFile << "    self:set(\"rotAngle\", " << RotAngle << ");\n";

    if (BackRenderMatName!=Default.BackRenderMatName)
        OutFile << "    self:set(\"backMaterial\", \"" << BackRenderMatName << "\");\n";

    if (BackColor[0]!=Default.BackColor[0] || BackColor[1]!=Default.BackColor[1] || BackColor[2]!=Default.BackColor[2] || BackColor[3]!=Default.BackColor[3])
        OutFile << "    self:set(\"backColor\", " << BackColor[0] << ", " << BackColor[1] << ", " << BackColor[2] << ", " << BackColor[3] << ");\n";

    if (BorderWidth!=Default.BorderWidth)
        OutFile << "    self:set(\"borderWidth\", " << BorderWidth << ");\n";

    if (BorderColor[0]!=Default.BorderColor[0] || BorderColor[1]!=Default.BorderColor[1] || BorderColor[2]!=Default.BorderColor[2] || BorderColor[3]!=Default.BorderColor[3])
        OutFile << "    self:set(\"borderColor\", " << BorderColor[0] << ", " << BorderColor[1] << ", " << BorderColor[2] << ", " << BorderColor[3] << ");\n";

    if (Font->GetName()!=Default.Font->GetName())
        OutFile << "    self:set(\"font\", \"" << Font->GetName() << "\");\n";

    if (Text!=Default.Text)
    {
        // Replace line breaks with \n and " with \".
        wxString FormattedText=Text;
        FormattedText.Replace("\n", "\\n");
        FormattedText.Replace("\"", "\\\"");

        OutFile << "    self:set(\"text\", \"" << FormattedText << "\");\n";
    }

    if (TextScale!=Default.TextScale)
        OutFile << "    self:set(\"textScale\", " << TextScale << ");\n";

    if (TextColor[0]!=Default.TextColor[0] || TextColor[1]!=Default.TextColor[1] || TextColor[2]!=Default.TextColor[2] || TextColor[3]!=Default.TextColor[3])
        OutFile << "    self:set(\"textColor\", " << TextColor[0] << ", " << TextColor[1] << ", " << TextColor[2] << ", " << TextColor[3] << ");\n";

    if (TextAlignHor!=Default.TextAlignHor)
        OutFile << "    self:set(\"textAlignHor\", " << TextAlignHor << ");\n";

    if (TextAlignVer!=Default.TextAlignVer)
        OutFile << "    self:set(\"textAlignVer\", " << TextAlignVer << ");\n";

    return true;
}


bool ChoiceT::WriteInitMethod(std::ostream& OutFile)
{
    WindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    WindowCreateParamsT Params(Gui);
    ChoiceT             Default(Params);

    for (unsigned long ChoiceNr=0; ChoiceNr<Choices.Size(); ChoiceNr++)
        OutFile << "    self:Append(\"" << Choices[ChoiceNr] << "\");\n";

    if (SelectedChoice!=Default.SelectedChoice)
        OutFile << "    self:SetSelection(" << SelectedChoice << ");\n";

    return true;
}


bool EditWindowT::WriteInitMethod(std::ostream& OutFile)
{
    WindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    WindowCreateParamsT Params(Gui);
    EditWindowT         Default(Params);

    if (TextCursorType!=Default.TextCursorType)
        OutFile << "    self:SetTextCursorType(\"" << TextCursorType << "\");\n";

    if (TextCursorRate!=Default.TextCursorRate)
        OutFile << "    self:SetTextCursorRate(" << TextCursorRate << ");\n";

    if (TextCursorColor[0]!=Default.TextCursorColor[0] || TextCursorColor[1]!=Default.TextCursorColor[1] || TextCursorColor[2]!=Default.TextCursorColor[2] || TextCursorColor[3]!=Default.TextCursorColor[3])
        OutFile << "    self:SetTextCursorColor(" << TextCursorColor[0] << ", " << TextCursorColor[1] << ", " << TextCursorColor[2] << ", " << TextCursorColor[3] << ");\n";

    return true;
}


bool ListBoxT::WriteInitMethod(std::ostream& OutFile)
{
    WindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    WindowCreateParamsT Params(Gui);
    ListBoxT            Default(Params);

    if (RowHeight!=Default.RowHeight)
        OutFile << "    self:SetRowHeight(" << RowHeight << ");\n";

    if (OddRowBgColor[0]!=Default.OddRowBgColor[0] || OddRowBgColor[1]!=Default.OddRowBgColor[1] || OddRowBgColor[2]!=Default.OddRowBgColor[2] || OddRowBgColor[3]!=Default.OddRowBgColor[3])
        OutFile << "    self:SetOddRowBgColor(" << OddRowBgColor[0] << ", " << OddRowBgColor[1] << ", " << OddRowBgColor[2] << ", " << OddRowBgColor[3] << ");\n";

    if (EvenRowBgColor[0]!=Default.EvenRowBgColor[0] || EvenRowBgColor[1]!=Default.EvenRowBgColor[1] || EvenRowBgColor[2]!=Default.EvenRowBgColor[2] || EvenRowBgColor[3]!=Default.EvenRowBgColor[3])
        OutFile << "    self:SetEvenRowBgColor(" << EvenRowBgColor[0] << ", " << EvenRowBgColor[1] << ", " << EvenRowBgColor[2] << ", " << EvenRowBgColor[3] << ");\n";

    if (RowTextColor[0]!=Default.RowTextColor[0] || RowTextColor[1]!=Default.RowTextColor[1] || RowTextColor[2]!=Default.RowTextColor[2] || RowTextColor[3]!=Default.RowTextColor[3])
        OutFile << "    self:SetRowTextColor(" << RowTextColor[0] << ", " << RowTextColor[1] << ", " << RowTextColor[2] << ", " << RowTextColor[3] << ");\n";

    if (SelectedRowBgColor[0]!=Default.SelectedRowBgColor[0] || SelectedRowBgColor[1]!=Default.SelectedRowBgColor[1] || SelectedRowBgColor[2]!=Default.SelectedRowBgColor[2] || SelectedRowBgColor[3]!=Default.SelectedRowBgColor[3])
        OutFile << "    self:SetSelRowBgColor(" << SelectedRowBgColor[0] << ", " << SelectedRowBgColor[1] << ", " << SelectedRowBgColor[2] << ", " << SelectedRowBgColor[3] << ");\n";

    if (SelectedRowTextColor[0]!=Default.SelectedRowTextColor[0] || SelectedRowTextColor[1]!=Default.SelectedRowTextColor[1] || SelectedRowTextColor[2]!=Default.SelectedRowTextColor[2] || SelectedRowTextColor[3]!=Default.SelectedRowTextColor[3])
        OutFile << "    self:SetSelRowTextColor(" << SelectedRowTextColor[0] << ", " << SelectedRowTextColor[1] << ", " << SelectedRowTextColor[2] << ", " << SelectedRowTextColor[3] << ");\n";

    return true;
}


bool ModelWindowT::WriteInitMethod(std::ostream& OutFile)
{
    WindowT::WriteInitMethod(OutFile);

    // Create empty window to get default values.
    WindowCreateParamsT Params(Gui);
    ModelWindowT        Default(Params);

    if (Model.GetFileName()!=Default.Model.GetFileName())
        OutFile << "    self:SetModel(\"" << Model.GetFileName() << "\");\n";

    if (ModelSequNr!=Default.ModelSequNr)
        OutFile << "    self:SetModelSequNr(" << ModelSequNr << ");\n";

    if (ModelPos!=Default.ModelPos)
        OutFile << "    self:SetModelPos(" << ModelPos.x << ", " << ModelPos.y << ", " << ModelPos.z << ");\n";

    if (ModelScale!=Default.ModelScale)
        OutFile << "    self:SetModelScale(" << ModelScale << ");\n";

    if (ModelAngles!=Default.ModelAngles)
        OutFile << "    self:SetModelAngles(" << ModelAngles.x << ", " << ModelAngles.y << ", " << ModelAngles.z << ");\n";

    if (CameraPos!=Default.CameraPos)
        OutFile << "    self:SetCameraPos(" << CameraPos.x << ", " << CameraPos.y << ", " << CameraPos.z << ");\n";

    return true;
}


void WindowT::EditorRender() const
{
    if (!EditorData) return;

    // Render selection state of this window.
    if (((EditorDataWindowT*)EditorData)->Selected)
    {
        // Render selection border.

        float x1;
        float y1;

        GetAbsolutePos(x1, y1);

        const float x2=x1+Rect[2];
        const float y2=y1+Rect[3];

        MatSys::Renderer->SetCurrentMaterial(GuiMan->GetDefaultRM());

        static MatSys::MeshT BorderMesh(MatSys::MeshT::Quads);
        BorderMesh.Vertices.Overwrite();
        BorderMesh.Vertices.PushBackEmpty(4*4);     // One rectangle for each side of the background.

        for (unsigned long VertexNr=0; VertexNr<BorderMesh.Vertices.Size(); VertexNr++)
        {
            for (unsigned long i=0; i<4; i++)
                BorderMesh.Vertices[VertexNr].Color[i]=SelectionColor[i];
        }

        // Left border rectangle.
        BorderMesh.Vertices[ 0].SetOrigin(x1,                 y1); BorderMesh.Vertices[ 0].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 1].SetOrigin(x1+SelectionBorder, y1); BorderMesh.Vertices[ 1].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 2].SetOrigin(x1+SelectionBorder, y2); BorderMesh.Vertices[ 2].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 3].SetOrigin(x1,                 y2); BorderMesh.Vertices[ 3].SetTextureCoord(0.0f, 1.0f);

        // Top border rectangle.
        BorderMesh.Vertices[ 4].SetOrigin(x1+SelectionBorder, y1                ); BorderMesh.Vertices[ 4].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 5].SetOrigin(x2-SelectionBorder, y1                ); BorderMesh.Vertices[ 5].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 6].SetOrigin(x2-SelectionBorder, y1+SelectionBorder); BorderMesh.Vertices[ 6].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 7].SetOrigin(x1+SelectionBorder, y1+SelectionBorder); BorderMesh.Vertices[ 7].SetTextureCoord(0.0f, 1.0f);

        // Right border rectangle.
        BorderMesh.Vertices[ 8].SetOrigin(x2-SelectionBorder, y1); BorderMesh.Vertices[ 8].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 9].SetOrigin(x2,                 y1); BorderMesh.Vertices[ 9].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[10].SetOrigin(x2,                 y2); BorderMesh.Vertices[10].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[11].SetOrigin(x2-SelectionBorder, y2); BorderMesh.Vertices[11].SetTextureCoord(0.0f, 1.0f);

        // Bottom border rectangle.
        BorderMesh.Vertices[12].SetOrigin(x1+SelectionBorder, y2-SelectionBorder); BorderMesh.Vertices[12].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[13].SetOrigin(x2-SelectionBorder, y2-SelectionBorder); BorderMesh.Vertices[13].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[14].SetOrigin(x2-SelectionBorder, y2                ); BorderMesh.Vertices[14].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[15].SetOrigin(x1+SelectionBorder, y2                ); BorderMesh.Vertices[15].SetTextureCoord(0.0f, 1.0f);

        MatSys::Renderer->RenderMesh(BorderMesh);
    }
}


void ChoiceT::EditorRender() const
{
    WindowT::EditorRender();
}


void EditWindowT::EditorRender() const
{
    WindowT::EditorRender();
}


void ListBoxT::EditorRender() const
{
    WindowT::EditorRender();
}


void ModelWindowT::EditorRender() const
{
    WindowT::EditorRender();
}
