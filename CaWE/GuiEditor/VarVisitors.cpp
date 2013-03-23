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

#include "VarVisitors.hpp"
#include "ChildFrame.hpp"
#include "GuiDocument.hpp"
#include "Commands/SetCompVar.hpp"

#include "../EditorMaterial.hpp"
#include "../MaterialBrowser/DocAccess.hpp"
#include "../MaterialBrowser/MaterialBrowserDialog.hpp"

#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"


using namespace GuiEditor;


namespace
{
    wxColour cast(const Vector3fT& v)
    {
        return wxColour(
            std::min(std::max(0.0f, v.x), 1.0f) * 255.0f + 0.5f,
            std::min(std::max(0.0f, v.y), 1.0f) * 255.0f + 0.5f,
            std::min(std::max(0.0f, v.z), 1.0f) * 255.0f + 0.5f);
    }


    /// I was not able to make wx(Edit)EnumProperty work with `string` values (as opposed to `int` values),
    /// so this is a simple replacement for wxEditEnumProperty that works with strings.
    class EditEnumPropertyT : public wxLongStringProperty
    {
        public:

        EditEnumPropertyT(const wxString& label,
                          const wxString& name,
                          const wxArrayString& Choices,
                          const wxString& value)
            : wxLongStringProperty(label, name, value),
              m_Choices(Choices)
        {
        }

        // Shows the string selection dialog.
        virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
        {
            const int Index = wxGetSingleChoiceIndex("Available choices:", "Select a string", m_Choices, propGrid->GetPanel());

            if (Index < 0) return false;

            value = m_Choices[Index];
            return true;
        }


        private:

        wxArrayString m_Choices;
    };


    /// Custom property to select materials from the Material Browser.
    class MaterialPropertyT : public wxLongStringProperty
    {
        public:

        MaterialPropertyT(const wxString& label,
                          const wxString& name,
                          const wxString& value,
                          GuiDocumentT* GuiDoc)
            : wxLongStringProperty(label, name, value),
              m_GuiDocument(GuiDoc)
        {
        }

        // Shows the Material Browser and returns the name of the selected material.
        virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
        {
            EditorMaterialI*                InitMat = NULL;
            const ArrayT<EditorMaterialI*>& EditorMaterials = m_GuiDocument->GetEditorMaterials();

            for (unsigned long EMNr = 0; EMNr < EditorMaterials.Size(); EMNr++)
                if (EditorMaterials[EMNr]->GetName() == value)
                {
                    InitMat=EditorMaterials[EMNr];
                    break;
                }

            MaterialBrowser::DialogT MatBrowser(GetGrid(), MaterialBrowser::GuiDocAccessT(*m_GuiDocument), MaterialBrowser::ConfigT()
                .InitialMaterial(InitMat)
                .NoFilterEditorMatsOnly()
                .NoButtonMark()
                .NoButtonReplace());

            if (MatBrowser.ShowModal() != wxID_OK) return false;

            EditorMaterialI* Mat = MatBrowser.GetCurrentMaterial();
            if (Mat == NULL) return false;

            value = Mat->GetName();
            return true;
        }


        private:

        GuiDocumentT* m_GuiDocument;
    };


    /// This class represents a custom PropertyGrid property for Cafu game filenames.
    /// A "game filename" is a filename that is relative to the Cafu main (working) directory.
    ///
    /// In contrast to using a wxFileProperty directly, deriving from wxLongStringProperty is much easier to
    /// understand and to customize (e.g. for making the path relative to the Cafu main (working) directory
    /// and for flipping the \ to / slashes).
    /// Deriving from wxFileProperty and overloading OnButtonClick() didn't work.
    class GameFileNamePropertyT : public wxLongStringProperty
    {
        public:

        GameFileNamePropertyT(const wxString& label,
                              const wxString& name,
                              const wxString& value,
                              const wxString& Filter="All Files (*.*)|*.*",
                              const wxString& SubDir="")
            : wxLongStringProperty(label, name, value),
              m_Filter(Filter),
              m_SubDir(SubDir)
        {
        }

        // Shows the file selection dialog and makes the choosen file path relative.
        virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
        {
            wxString InitialDir  = "";  // m_MapDoc->GetGameConfig()->ModDir + m_SubDir;
            wxString FileNameStr = wxFileSelector("Select a file", InitialDir, "", "", m_Filter, wxFD_OPEN, propGrid->GetPanel());

            if (FileNameStr == "") return false;

            wxFileName FileName(FileNameStr);
            FileName.MakeRelativeTo(".");   // Make it relative to the current working directory.

            value = FileName.GetFullPath(wxPATH_UNIX);
            return true;
        }


        private:

        wxString m_Filter;
        wxString m_SubDir;
    };
}


/**************************/
/*** VarVisitorAddPropT ***/
/**************************/

VarVisitorAddPropT::VarVisitorAddPropT(wxPropertyGridManager& PropMan, GuiDocumentT* GuiDoc)
    : m_PropMan(PropMan),
      m_GuiDoc(GuiDoc)
{
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<float>& Var)
{
    wxPGProperty* Prop = new wxFloatProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<double>& Var)
{
    wxPGProperty* Prop = new wxFloatProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<int>& Var)
{
    wxPGProperty*       Prop = NULL;
    ArrayT<std::string> Strings;
    ArrayT<int>         Values;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    if (Strings.Size() > 0)
    {
        wxPGChoices Choices;

        for (unsigned int i = 0; i < Strings.Size(); i++)
            Choices.Add(wxString::Format("%s (%i)", Strings[i], Values[i]), Values[i]);

        Prop = new wxEnumProperty(Var.GetName(), wxString::Format("%p", &Var), Choices, Var.Get());
    }
    else
    {
        Prop = new wxIntProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
    }

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<unsigned int>& Var)
{
    wxPGProperty*        Prop = NULL;
    ArrayT<std::string>  Strings;
    ArrayT<unsigned int> Values;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    if (Strings.Size() > 0)
    {
        wxPGChoices Choices;

        for (unsigned int i = 0; i < Strings.Size(); i++)
            Choices.Add(wxString::Format("%s (%i)", Strings[i], Values[i]), Values[i]);

        Prop = new wxEnumProperty(Var.GetName(), wxString::Format("%p", &Var), Choices, Var.Get());
    }
    else
    {
        Prop = new wxUIntProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
    }

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<bool>& Var)
{
    wxPGProperty* Prop = new wxBoolProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    m_PropMan.SetPropertyAttribute(Prop, wxPG_BOOL_USE_CHECKBOX, true);   // Use a checkbox, not a choice.
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    wxPGProperty* Prop = NULL;

    if (Var.HasFlag("IsMaterial"))
    {
        Prop = new MaterialPropertyT(Var.GetName(), wxString::Format("%p", &Var), Var.Get(), m_GuiDoc);
    }
    else if (Var.HasFlag("IsLongString"))
    {
        Prop = new wxLongStringProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

        // Have to disable the escaping of newlines, or otherwise our event handlers receive strings like "a\\nb" instead
        // of "a\nb", which is clearly not what we want. With the wxPG_PROP_NO_ESCAPE flag set, we receive the desired
        // "a\nb" form, but the value that is shown to the user in the property cell is "ab", so this clearly isn't ideal
        // either. TODO!
        Prop->ChangeFlag(wxPG_PROP_NO_ESCAPE, true);
    }
    else if (Var.HasFlag("IsModelFileName"))
    {
        const wxString Wildcard = "Cafu Model Files (*.cmdl)|*.cmdl|"
                                  "All Files (*.*)|*.*";

        Prop = new GameFileNamePropertyT(Var.GetName(), wxString::Format("%p", &Var), Var.Get(), Wildcard);
    }
    else
    {
        ArrayT<std::string> Strings;
        ArrayT<std::string> Values;

        Var.GetChoices(Strings, Values);
        wxASSERT(Strings.Size() == Values.Size());

        if (Strings.Size() > 0)
        {
            wxArrayString Choices;

            for (unsigned int i = 0; i < Strings.Size(); i++)
                Choices.Add(Strings[i]);

            Prop = new EditEnumPropertyT(Var.GetName(), wxString::Format("%p", &Var), Choices, Var.Get());
        }
        else
        {
            Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
        }
    }

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), "<composed>");

    m_PropMan.Append(Prop)->SetClientData(&Var);

    m_PropMan.AppendIn(Prop, new wxFloatProperty(Var.GetFlag("Labels", 1, "x"), wxPG_LABEL, Var.Get().x))->SetTextColour(wxColour(200, 0, 0));
    m_PropMan.AppendIn(Prop, new wxFloatProperty(Var.GetFlag("Labels", 2, "y"), wxPG_LABEL, Var.Get().y))->SetTextColour(wxColour(0, 200, 0));
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    if (Var.HasFlag("IsColor"))
    {
        wxColour      Col  = cast(Var.Get());
        wxPGProperty* Prop = new wxColourProperty(Var.GetName(), wxString::Format("%p", &Var), Col);

        m_PropMan.Append(Prop)->SetClientData(&Var);
    }
    else
    {
        wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), "<composed>");

        m_PropMan.Append(Prop)->SetClientData(&Var);

        m_PropMan.AppendIn(Prop, new wxFloatProperty("x", wxPG_LABEL, Var.Get().x))->SetTextColour(wxColour(200, 0, 0));
        m_PropMan.AppendIn(Prop, new wxFloatProperty("y", wxPG_LABEL, Var.Get().y))->SetTextColour(wxColour(0, 200, 0));
        m_PropMan.AppendIn(Prop, new wxFloatProperty("z", wxPG_LABEL, Var.Get().z))->SetTextColour(wxColour(0, 0, 200));
    }
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Get().Size(); i++)
        Lines += Var.Get()[i] + "\n";

    // We use a wxLongStringProperty instead of a wxArrayStringProperty, because the latter seems to be
    // very buggy in wx-2.9.2, and its edit dialog is cumbersome to use compared to a simple multi-line string.
    wxPGProperty* Prop = new wxLongStringProperty(Var.GetName(), wxString::Format("%p", &Var), Lines);

    // Have to disable the escaping of newlines, or otherwise our event handlers receive strings like "a\\nb" instead
    // of "a\nb", which is clearly not what we want. With the wxPG_PROP_NO_ESCAPE flag set, we receive the desired
    // "a\nb" form, but the value that is shown to the user in the property cell is "ab", so this clearly isn't ideal
    // either. TODO!
    Prop->ChangeFlag(wxPG_PROP_NO_ESCAPE, true);

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


/*****************************/
/*** VarVisitorUpdatePropT ***/
/*****************************/

VarVisitorUpdatePropT::VarVisitorUpdatePropT(wxPGProperty& Prop)
    : m_Prop(Prop)
{
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<float>& Var)
{
    m_Prop.SetValue(Var.Get());
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<double>& Var)
{
    m_Prop.SetValue(Var.Get());
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<int>& Var)
{
    ArrayT<std::string> Strings;
    ArrayT<int>         Values;
    wxPGChoices         Choices;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    for (unsigned int i = 0; i < Strings.Size(); i++)
        Choices.Add(wxString::Format("%s (%i)", Strings[i], Values[i]), Values[i]);

    if (Choices.GetCount() > 0)
        m_Prop.SetChoices(Choices);

    m_Prop.SetValue(Var.Get());
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<unsigned int>& Var)
{
    ArrayT<std::string>  Strings;
    ArrayT<unsigned int> Values;
    wxPGChoices          Choices;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    for (unsigned int i = 0; i < Strings.Size(); i++)
        Choices.Add(wxString::Format("%s (%i)", Strings[i], Values[i]), Values[i]);

    if (Choices.GetCount() > 0)
        m_Prop.SetChoices(Choices);

    m_Prop.SetValue(int(Var.Get()));    // Uh! Cannot convert to wxVariant from unsigned int.
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<bool>& Var)
{
    m_Prop.SetValue(Var.Get());
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<std::string>& Var)
{
    m_Prop.SetValue(Var.Get());
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<Vector2fT>& Var)
{
    const unsigned int Count = std::min(2u, m_Prop.GetChildCount());

    for (unsigned int i = 0; i < Count; i++)
        m_Prop.Item(i)->SetValue(Var.Get()[i]);
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)
{
    if (Var.HasFlag("IsColor"))
    {
        const wxColour Col = cast(Var.Get());

        m_Prop.SetValueFromString(wxString::Format("(%u,%u,%u)", Col.Red(), Col.Green(), Col.Blue()));
    }
    else
    {
        const unsigned int Count = std::min(3u, m_Prop.GetChildCount());

        for (unsigned int i = 0; i < Count; i++)
            m_Prop.Item(i)->SetValue(Var.Get()[i]);
    }
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Get().Size(); i++)
        Lines += Var.Get()[i] + "\n";

    m_Prop.SetValue(Lines);
}


/******************************************/
/*** VarVisitorHandlePropChangingEventT ***/
/******************************************/

VarVisitorHandlePropChangingEventT::VarVisitorHandlePropChangingEventT(wxPropertyGridEvent& Event, ChildFrameT* ChildFrame)
    : m_Event(Event),
      m_ChildFrame(ChildFrame),
      m_GuiDoc(ChildFrame->GetGuiDoc()),
      m_Ok(false)
{
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<float>& Var)
{
    const float f = m_Event.GetValue().GetDouble();

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<float>(m_GuiDoc, Var, f));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<double>& Var)
{
    const double d = m_Event.GetValue().GetDouble();

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<double>(m_GuiDoc, Var, d));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<int>& Var)
{
    const int i = m_Event.GetValue().GetLong();

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<int>(m_GuiDoc, Var, i));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<unsigned int>& Var)
{
    const unsigned int ui = m_Event.GetValue().GetLong();   // Uh! There is no GetULong() method.

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<unsigned int>(m_GuiDoc, Var, ui));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<bool>& Var)
{
    const bool b = m_Event.GetValue().GetBool();

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<bool>(m_GuiDoc, Var, b));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    const std::string s = std::string(m_Event.GetValue().GetString());

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<std::string>(m_GuiDoc, Var, s));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT v;

    // This is a "<composed>" property, and its summary string is changing.
    // For example, the value could be changing from "100.0; 0.0" to "100.0; 150".
    wxStringTokenizer Tokenizer(m_Event.GetValue().GetString(), "; \t\r\n", wxTOKEN_STRTOK);

    for (unsigned int i = 0; i < 2; i++)
    {
        double d = 0.0;

        // On error, return with m_Ok == false.
        if (!Tokenizer.HasMoreTokens()) return;
        if (!Tokenizer.GetNextToken().ToCDouble(&d)) return;

        v[i] = float(d);
    }

    if (Tokenizer.HasMoreTokens()) return;

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<Vector2fT>(m_GuiDoc, Var, v));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v;

    if (Var.HasFlag("IsColor"))
    {
        wxColour NewColor;
        NewColor << m_Event.GetValue();

        v.x = NewColor.Red()   / 255.0f;
        v.y = NewColor.Green() / 255.0f;
        v.z = NewColor.Blue()  / 255.0f;
    }
    else
    {
        // This is a "<composed>" property, and its summary string is changing.
        // For example, the value could be changing from "100.0; 0.0; 50.0" to "100.0; 150; 200.0".
        wxStringTokenizer Tokenizer(m_Event.GetValue().GetString(), "; \t\r\n", wxTOKEN_STRTOK);

        for (unsigned int i = 0; i < 3; i++)
        {
            double d = 0.0;

            // On error, return with m_Ok == false.
            if (!Tokenizer.HasMoreTokens()) return;
            if (!Tokenizer.GetNextToken().ToCDouble(&d)) return;

            v[i] = float(d);
        }

        if (Tokenizer.HasMoreTokens()) return;
    }

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<Vector3fT>(m_GuiDoc, Var, v));
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var)
{
    ArrayT<std::string> A;
    wxStringTokenizer   Tokenizer(m_Event.GetValue().GetString(), "\n", wxTOKEN_RET_EMPTY);

    while (Tokenizer.HasMoreTokens())
        A.PushBack(std::string(Tokenizer.GetNextToken()));

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT< ArrayT<std::string> >(m_GuiDoc, Var, A));
}


/*****************************************/
/*** VarVisitorHandleSubChangingEventT ***/
/*****************************************/

VarVisitorHandleSubChangingEventT::VarVisitorHandleSubChangingEventT(wxPropertyGridEvent& Event, ChildFrameT* ChildFrame)
    : m_Event(Event),
      m_ChildFrame(ChildFrame),
      m_GuiDoc(ChildFrame->GetGuiDoc()),
      m_Ok(false)
{
}


// Plain variables have no sub-properties.
void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<float>& Var) { wxASSERT(false); }
void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<double>& Var) { wxASSERT(false); }
void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<int>& Var) { wxASSERT(false); }
void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<unsigned int>& Var) { wxASSERT(false); }
void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<bool>& Var) { wxASSERT(false); }
void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<std::string>& Var) { wxASSERT(false); }


void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT          v = Var.Get();
    const unsigned int i = m_Event.GetProperty()->GetIndexInParent();

    // On error, return with m_Ok == false.
    if (i >= 2) return;

    v[i] = m_Event.GetValue().GetDouble();
    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<Vector2fT>(m_GuiDoc, Var, v));
}


void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    wxASSERT(!Var.HasFlag("IsColor"));

    Vector3fT          v = Var.Get();
    const unsigned int i = m_Event.GetProperty()->GetIndexInParent();

    // On error, return with m_Ok == false.
    if (i >= 3) return;

    v[i] = m_Event.GetValue().GetDouble();
    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<Vector3fT>(m_GuiDoc, Var, v));
}


void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var) { wxASSERT(false); }
