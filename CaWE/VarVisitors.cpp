/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "VarVisitors.hpp"
#include "DocumentAdapter.hpp"
#include "EditorMaterial.hpp"
#include "SetCompVar.hpp"
#include "MaterialBrowser/MaterialBrowserDialog.hpp"

#include "TypeSys.hpp"

#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"

#include <algorithm>


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
                          DocAdapterI&    DocAdapter)
            : wxLongStringProperty(label, name, value),
              m_DocAdapter(DocAdapter)
        {
        }

        // Shows the Material Browser and returns the name of the selected material.
        virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
        {
            EditorMaterialI*                InitMat = NULL;
            const ArrayT<EditorMaterialI*>& EditorMaterials = m_DocAdapter.GetMaterials();

            for (unsigned long EMNr = 0; EMNr < EditorMaterials.Size(); EMNr++)
                if (EditorMaterials[EMNr]->GetName() == value)
                {
                    InitMat=EditorMaterials[EMNr];
                    break;
                }

            MaterialBrowser::DialogT MatBrowser(GetGrid(), m_DocAdapter, MaterialBrowser::ConfigT()
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

        DocAdapterI& m_DocAdapter;
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

VarVisitorAddPropT::VarVisitorAddPropT(wxPropertyGridManager& PropMan, DocAdapterI& DocAdapter, const cf::TypeSys::TypeInfoT* TI)
    : m_PropMan(PropMan),
      m_DocAdapter(DocAdapter),
      m_TI(TI)
{
}


void VarVisitorAddPropT::SetHelpString(wxPGProperty* Prop, wxPGProperty* SourceProp) const
{
    if (!m_TI->DocVars) return;

    if (!SourceProp)
    {
        // Prop is a normal property.
        for (unsigned int DocNr = 0; m_TI->DocVars[DocNr].Name; DocNr++)
            if (Prop->GetLabel() == m_TI->DocVars[DocNr].Name)
            {
                Prop->SetHelpString(m_TI->DocVars[DocNr].Doc);
                return;
            }
    }
    else
    {
        // Prop is a sub-property of composed property SourceProp.
        for (unsigned int DocNr = 0; m_TI->DocVars[DocNr].Name; DocNr++)
            if (SourceProp->GetLabel() == m_TI->DocVars[DocNr].Name)
            {
                Prop->SetHelpString("An element of " + SourceProp->GetLabel() + ": " + m_TI->DocVars[DocNr].Doc);
                return;
            }
    }
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<float>& Var)
{
    wxPGProperty* Prop = new wxFloatProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<double>& Var)
{
    wxPGProperty* Prop = new wxFloatProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    SetHelpString(Prop);
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

    SetHelpString(Prop);
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
            Choices.Add(wxString::Format("%s (%u)", Strings[i], Values[i]), Values[i]);

        Prop = new wxEnumProperty(Var.GetName(), wxString::Format("%p", &Var), Choices, Var.Get());
    }
    else
    {
        Prop = new wxUIntProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
    }

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<uint16_t>& Var)
{
    wxPGProperty*       Prop = NULL;
    ArrayT<std::string> Strings;
    ArrayT<uint16_t>    Values;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    if (Strings.Size() > 0)
    {
        wxPGChoices Choices;

        for (unsigned int i = 0; i < Strings.Size(); i++)
            Choices.Add(wxString::Format("%s (%u)", Strings[i], Values[i]), Values[i]);

        Prop = new wxEnumProperty(Var.GetName(), wxString::Format("%p", &Var), Choices, Var.Get());
    }
    else
    {
        Prop = new wxUIntProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
    }

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<uint8_t>& Var)
{
    wxPGProperty*       Prop = NULL;
    ArrayT<std::string> Strings;
    ArrayT<uint8_t>     Values;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    if (Strings.Size() > 0)
    {
        wxPGChoices Choices;

        for (unsigned int i = 0; i < Strings.Size(); i++)
            Choices.Add(wxString::Format("%s (%u)", Strings[i], Values[i]), Values[i]);

        Prop = new wxEnumProperty(Var.GetName(), wxString::Format("%p", &Var), Choices, Var.Get());
    }
    else
    {
        Prop = new wxUIntProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
    }

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<bool>& Var)
{
    wxPGProperty* Prop = new wxBoolProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    SetHelpString(Prop);
    m_PropMan.SetPropertyAttribute(Prop, wxPG_BOOL_USE_CHECKBOX, true);   // Use a checkbox, not a choice.
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    wxPGProperty* Prop = NULL;

    if (Var.HasFlag("IsMaterial"))
    {
        Prop = new MaterialPropertyT(Var.GetName(), wxString::Format("%p", &Var), Var.Get(), m_DocAdapter);
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
    else if (Var.HasFlag("IsGenericFileName"))
    {
        Prop = new GameFileNamePropertyT(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
    }
    else if (Var.HasFlag("IsGuiFileName"))
    {
        const wxString Wildcard = "Cafu GUI Files (*_main.cgui)|*.cgui|"
                                  "All Files (*.*)|*.*";

        Prop = new GameFileNamePropertyT(Var.GetName(), wxString::Format("%p", &Var), Var.Get(), Wildcard);
    }
    else if (Var.HasFlag("IsModelFileName"))
    {
        const wxString Wildcard = "Cafu Model Files (*.cmdl)|*.cmdl|"
                                  "All Files (*.*)|*.*";

        Prop = new GameFileNamePropertyT(Var.GetName(), wxString::Format("%p", &Var), Var.Get(), Wildcard);
    }
    else if (Var.HasFlag("IsLuaFileName"))
    {
        // Note that we also have Lua scripts with extensions other than ".lua",
        // but for those, the more generic flag "IsScriptFileName" should be used.
        const wxString Wildcard = "Cafu Script Files (*.lua)|*.lua|"
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

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), "<composed>");

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);

    wxPGProperty* px = new wxFloatProperty(Var.GetFlag("Labels", 1, "x"), wxPG_LABEL, Var.Get().x);
    wxPGProperty* py = new wxFloatProperty(Var.GetFlag("Labels", 2, "y"), wxPG_LABEL, Var.Get().y);

    SetHelpString(px, Prop);
    SetHelpString(py, Prop);

    m_PropMan.AppendIn(Prop, px);
    m_PropMan.AppendIn(Prop, py);

    // For unknown reasons, the text color can only be set *after* AppendIn() has been called.
    px->SetTextColour(wxColour(200, 0, 0));
    py->SetTextColour(wxColour(0, 200, 0));
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    if (Var.HasFlag("IsColor"))
    {
        wxColour      Col  = cast(Var.Get());
        wxPGProperty* Prop = new wxColourProperty(Var.GetName(), wxString::Format("%p", &Var), Col);

        SetHelpString(Prop);
        m_PropMan.Append(Prop)->SetClientData(&Var);
    }
    else
    {
        wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), "<composed>");

        SetHelpString(Prop);
        m_PropMan.Append(Prop)->SetClientData(&Var);

        wxPGProperty* px = new wxFloatProperty("x", wxPG_LABEL, Var.Get().x);
        wxPGProperty* py = new wxFloatProperty("y", wxPG_LABEL, Var.Get().y);
        wxPGProperty* pz = new wxFloatProperty("z", wxPG_LABEL, Var.Get().z);

        SetHelpString(px, Prop);
        SetHelpString(py, Prop);
        SetHelpString(pz, Prop);

        m_PropMan.AppendIn(Prop, px);
        m_PropMan.AppendIn(Prop, py);
        m_PropMan.AppendIn(Prop, pz);

        // For unknown reasons, the text color can only be set *after* AppendIn() has been called.
        px->SetTextColour(wxColour(200, 0, 0));
        py->SetTextColour(wxColour(0, 200, 0));
        pz->SetTextColour(wxColour(0, 0, 200));
    }
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<Vector3dT>& Var)
{
    wxASSERT(!Var.HasFlag("IsColor"));  // User code should really use a Vector3fT instead.

    wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), "<composed>");

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);

    wxPGProperty* px = new wxFloatProperty("x", wxPG_LABEL, Var.Get().x);
    wxPGProperty* py = new wxFloatProperty("y", wxPG_LABEL, Var.Get().y);
    wxPGProperty* pz = new wxFloatProperty("z", wxPG_LABEL, Var.Get().z);

    SetHelpString(px, Prop);
    SetHelpString(py, Prop);
    SetHelpString(pz, Prop);

    m_PropMan.AppendIn(Prop, px);
    m_PropMan.AppendIn(Prop, py);
    m_PropMan.AppendIn(Prop, pz);

    // For unknown reasons, the text color can only be set *after* AppendIn() has been called.
    px->SetTextColour(wxColour(200, 0, 0));
    py->SetTextColour(wxColour(0, 200, 0));
    pz->SetTextColour(wxColour(0, 0, 200));
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), "<composed>");

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);

    for (unsigned int i = 0; i < 2; i++)
    {
        const Vector3dT& MinMax = (i == 0) ? Var.Get().Min : Var.Get().Max;
        wxPGProperty*    MinMaxProp = new wxStringProperty(i == 0 ? "Min" : "Max", wxPG_LABEL, "<composed>");

        SetHelpString(MinMaxProp, Prop);
        m_PropMan.AppendIn(Prop, MinMaxProp);

        wxPGProperty* px = new wxFloatProperty("x", wxPG_LABEL, MinMax.x);
        wxPGProperty* py = new wxFloatProperty("y", wxPG_LABEL, MinMax.y);
        wxPGProperty* pz = new wxFloatProperty("z", wxPG_LABEL, MinMax.z);

        SetHelpString(px, Prop);
        SetHelpString(py, Prop);
        SetHelpString(pz, Prop);

        m_PropMan.AppendIn(MinMaxProp, px);
        m_PropMan.AppendIn(MinMaxProp, py);
        m_PropMan.AppendIn(MinMaxProp, pz);

        // For unknown reasons, the text color can only be set *after* AppendIn() has been called.
        px->SetTextColour(wxColour(200, 0, 0));
        py->SetTextColour(wxColour(0, 200, 0));
        pz->SetTextColour(wxColour(0, 0, 200));
    }
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += wxString::Format("%u\n", Var[i]);

    // We use a wxLongStringProperty instead of a wxArrayStringProperty, because the latter seems to be
    // very buggy in wx-2.9.2, and its edit dialog is cumbersome to use compared to a simple multi-line string.
    wxPGProperty* Prop = new wxLongStringProperty(Var.GetName(), wxString::Format("%p", &Var), Lines);

    // Have to disable the escaping of newlines, or otherwise our event handlers receive strings like "a\\nb" instead
    // of "a\nb", which is clearly not what we want. With the wxPG_PROP_NO_ESCAPE flag set, we receive the desired
    // "a\nb" form, but the value that is shown to the user in the property cell is "ab", so this clearly isn't ideal
    // either. TODO!
    Prop->ChangeFlag(wxPG_PROP_NO_ESCAPE, true);

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += wxString::Format("%u\n", Var[i]);

    // We use a wxLongStringProperty instead of a wxArrayStringProperty, because the latter seems to be
    // very buggy in wx-2.9.2, and its edit dialog is cumbersome to use compared to a simple multi-line string.
    wxPGProperty* Prop = new wxLongStringProperty(Var.GetName(), wxString::Format("%p", &Var), Lines);

    // Have to disable the escaping of newlines, or otherwise our event handlers receive strings like "a\\nb" instead
    // of "a\nb", which is clearly not what we want. With the wxPG_PROP_NO_ESCAPE flag set, we receive the desired
    // "a\nb" form, but the value that is shown to the user in the property cell is "ab", so this clearly isn't ideal
    // either. TODO!
    Prop->ChangeFlag(wxPG_PROP_NO_ESCAPE, true);

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += wxString::Format("%u\n", Var[i]);

    // We use a wxLongStringProperty instead of a wxArrayStringProperty, because the latter seems to be
    // very buggy in wx-2.9.2, and its edit dialog is cumbersome to use compared to a simple multi-line string.
    wxPGProperty* Prop = new wxLongStringProperty(Var.GetName(), wxString::Format("%p", &Var), Lines);

    // Have to disable the escaping of newlines, or otherwise our event handlers receive strings like "a\\nb" instead
    // of "a\nb", which is clearly not what we want. With the wxPG_PROP_NO_ESCAPE flag set, we receive the desired
    // "a\nb" form, but the value that is shown to the user in the property cell is "ab", so this clearly isn't ideal
    // either. TODO!
    Prop->ChangeFlag(wxPG_PROP_NO_ESCAPE, true);

    SetHelpString(Prop);
    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarArrayT<std::string>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += Var[i] + "\n";

    // We use a wxLongStringProperty instead of a wxArrayStringProperty, because the latter seems to be
    // very buggy in wx-2.9.2, and its edit dialog is cumbersome to use compared to a simple multi-line string.
    wxPGProperty* Prop = new wxLongStringProperty(Var.GetName(), wxString::Format("%p", &Var), Lines);

    // Have to disable the escaping of newlines, or otherwise our event handlers receive strings like "a\\nb" instead
    // of "a\nb", which is clearly not what we want. With the wxPG_PROP_NO_ESCAPE flag set, we receive the desired
    // "a\nb" form, but the value that is shown to the user in the property cell is "ab", so this clearly isn't ideal
    // either. TODO!
    Prop->ChangeFlag(wxPG_PROP_NO_ESCAPE, true);

    SetHelpString(Prop);
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
        Choices.Add(wxString::Format("%s (%u)", Strings[i], Values[i]), Values[i]);

    if (Choices.GetCount() > 0)
        m_Prop.SetChoices(Choices);

    m_Prop.SetValue(int(Var.Get()));    // Uh! Cannot convert to wxVariant from unsigned int.
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<uint16_t>& Var)
{
    ArrayT<std::string> Strings;
    ArrayT<uint16_t>    Values;
    wxPGChoices         Choices;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    for (unsigned int i = 0; i < Strings.Size(); i++)
        Choices.Add(wxString::Format("%s (%u)", Strings[i], Values[i]), Values[i]);

    if (Choices.GetCount() > 0)
        m_Prop.SetChoices(Choices);

    m_Prop.SetValue(int(Var.Get()));    // Uh! Cannot convert to wxVariant from unsigned int.
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<uint8_t>& Var)
{
    ArrayT<std::string> Strings;
    ArrayT<uint8_t>     Values;
    wxPGChoices         Choices;

    Var.GetChoices(Strings, Values);
    wxASSERT(Strings.Size() == Values.Size());

    for (unsigned int i = 0; i < Strings.Size(); i++)
        Choices.Add(wxString::Format("%s (%u)", Strings[i], Values[i]), Values[i]);

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


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<Vector3dT>& Var)
{
    wxASSERT(!Var.HasFlag("IsColor"));  // User code should really use a Vector3fT instead.

    const unsigned int Count = std::min(3u, m_Prop.GetChildCount());

    for (unsigned int i = 0; i < Count; i++)
        m_Prop.Item(i)->SetValue(Var.Get()[i]);
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    if (m_Prop.GetChildCount() != 2) return;

    const unsigned int CountMin = std::min(3u, m_Prop.Item(0)->GetChildCount());
    const unsigned int CountMax = std::min(3u, m_Prop.Item(1)->GetChildCount());

    for (unsigned int i = 0; i < CountMin; i++)
        m_Prop.Item(0)->Item(i)->SetValue(Var.Get().Min[i]);

    for (unsigned int i = 0; i < CountMax; i++)
        m_Prop.Item(1)->Item(i)->SetValue(Var.Get().Max[i]);
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += wxString::Format("%u\n", Var[i]);

    m_Prop.SetValue(Lines);
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += wxString::Format("%u\n", Var[i]);

    m_Prop.SetValue(Lines);
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += wxString::Format("%u\n", Var[i]);

    m_Prop.SetValue(Lines);
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarArrayT<std::string>& Var)
{
    wxString Lines;

    for (unsigned int i = 0; i < Var.Size(); i++)
        Lines += Var[i] + "\n";

    m_Prop.SetValue(Lines);
}


/******************************************/
/*** VarVisitorHandlePropChangingEventT ***/
/******************************************/

VarVisitorHandlePropChangingEventT::VarVisitorHandlePropChangingEventT(wxPropertyGridEvent& Event, unsigned int Depth, DocAdapterI& DocAdapter)
    : m_Event(Event),
      m_Depth(Depth),
      m_DocAdapter(DocAdapter),
      m_Command(NULL)
{
}


VarVisitorHandlePropChangingEventT::~VarVisitorHandlePropChangingEventT()
{
    // If m_Command != NULL, then TransferCommand() was not called and we're leaking memory.
    wxASSERT(m_Command == NULL);
}


CommandT* VarVisitorHandlePropChangingEventT::TransferCommand()
{
    wxASSERT(m_Command);

    CommandT* Cmd = m_Command;
    m_Command = NULL;

    return Cmd;
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<float>& Var)
{
    wxASSERT(m_Depth == 0);
    const float f = m_Event.GetValue().GetDouble();

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<float>(m_DocAdapter, Var, f);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<double>& Var)
{
    wxASSERT(m_Depth == 0);
    const double d = m_Event.GetValue().GetDouble();

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<double>(m_DocAdapter, Var, d);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<int>& Var)
{
    wxASSERT(m_Depth == 0);
    const int i = m_Event.GetValue().GetLong();

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<int>(m_DocAdapter, Var, i);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<unsigned int>& Var)
{
    wxASSERT(m_Depth == 0);
    const unsigned int ui = m_Event.GetValue().GetLong();   // Uh! There is no GetULong() method.

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<unsigned int>(m_DocAdapter, Var, ui);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<uint16_t>& Var)
{
    wxASSERT(m_Depth == 0);
    const uint16_t ui = m_Event.GetValue().GetLong();   // Uh! There is no GetULong() method.

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<uint16_t>(m_DocAdapter, Var, ui);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<uint8_t>& Var)
{
    wxASSERT(m_Depth == 0);
    const uint8_t ui = m_Event.GetValue().GetLong();    // Uh! There is no GetULong() method.

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<uint8_t>(m_DocAdapter, Var, ui);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<bool>& Var)
{
    wxASSERT(m_Depth == 0);
    const bool b = m_Event.GetValue().GetBool();

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<bool>(m_DocAdapter, Var, b);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    wxASSERT(m_Depth == 0);
    const std::string s = std::string(m_Event.GetValue().GetString());

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<std::string>(m_DocAdapter, Var, s);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT v = Var.Get();

    if (m_Depth == 0)
    {
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
    }
    else if (m_Depth == 1)
    {
        // A single value, v.x or v.y, has changed.
        const unsigned int i = m_Event.GetProperty()->GetIndexInParent();

        if (i >= 2) return;
        v[i] = m_Event.GetValue().GetDouble();
    }
    else
    {
        wxASSERT(false);
    }

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<Vector2fT>(m_DocAdapter, Var, v);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v = Var.Get();

    if (m_Depth == 0)
    {
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
    }
    else if (m_Depth == 1)
    {
        wxASSERT(!Var.HasFlag("IsColor"));

        // A single value, v.x, v.y or v.z, has changed.
        const unsigned int i = m_Event.GetProperty()->GetIndexInParent();

        if (i >= 3) return;
        v[i] = m_Event.GetValue().GetDouble();
    }
    else
    {
        wxASSERT(false);
    }

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<Vector3fT>(m_DocAdapter, Var, v);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<Vector3dT>& Var)
{
    Vector3dT v = Var.Get();

    if (m_Depth == 0)
    {
        wxASSERT(!Var.HasFlag("IsColor"));  // User code should really use a Vector3fT instead.

        // This is a "<composed>" property, and its summary string is changing.
        // For example, the value could be changing from "100.0; 0.0; 50.0" to "100.0; 150; 200.0".
        wxStringTokenizer Tokenizer(m_Event.GetValue().GetString(), "; \t\r\n", wxTOKEN_STRTOK);

        for (unsigned int i = 0; i < 3; i++)
        {
            // On error, return with m_Ok == false.
            if (!Tokenizer.HasMoreTokens()) return;
            if (!Tokenizer.GetNextToken().ToCDouble(&v[i])) return;
        }

        if (Tokenizer.HasMoreTokens()) return;
    }
    else if (m_Depth == 1)
    {
        wxASSERT(!Var.HasFlag("IsColor"));

        // A single value, v.x, v.y or v.z, has changed.
        const unsigned int i = m_Event.GetProperty()->GetIndexInParent();

        if (i >= 3) return;
        v[i] = m_Event.GetValue().GetDouble();
    }
    else
    {
        wxASSERT(false);
    }

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<Vector3dT>(m_DocAdapter, Var, v);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    BoundingBox3dT BB = Var.Get();

    if (m_Depth == 0)
    {
        // This is a "<composed>" property, and its summary string is changing.
        // For example, the value could be changing from "[-12; -12; -36] [12; 12; 36]" to "[-12; -4.25; -36] [12; 12; 36]".
        wxStringTokenizer Tokenizer(m_Event.GetValue().GetString(), "[]; \t\r\n", wxTOKEN_STRTOK);

        for (unsigned int i = 0; i < 3; i++)
        {
            // On error, return with m_Ok == false.
            if (!Tokenizer.HasMoreTokens()) return;
            if (!Tokenizer.GetNextToken().ToCDouble(&BB.Min[i])) return;
        }

        for (unsigned int i = 0; i < 3; i++)
        {
            // On error, return with m_Ok == false.
            if (!Tokenizer.HasMoreTokens()) return;
            if (!Tokenizer.GetNextToken().ToCDouble(&BB.Max[i])) return;
        }

        if (Tokenizer.HasMoreTokens()) return;
    }
    else if (m_Depth == 1)
    {
#if 1
        // BUG IN wxWidgets:
        // If e.g. "y" was changed, m_Event.GetValue().GetString() yields "y" rather than the expected and required "x; y; z"!
        // Our code below can only fail, returning with m_Command == NULL.
        wxLogDebug("        in visit(): Var == \"%s\", Depth == %u, Prop \"%s\"", Var.GetName(), m_Depth, m_Event.GetProperty()->GetLabel());
        wxLogDebug("        Event value: '%s'", m_Event.GetValue().GetString());                // the new value in wrong format
        wxLogDebug("        Prop value: '%s'", m_Event.GetProperty()->GetValue().GetString());  // the old value before the change
#else
        // This is a "<composed>" property (for BB.Min or BB.Max), and its summary string is changing.
        // For example, the value could be changing from "100.0; 0.0; 50.0" to "100.0; 150; 200.0".
        wxStringTokenizer Tokenizer(m_Event.GetValue().GetString(), "; \t\r\n", wxTOKEN_STRTOK);
        Vector3dT& v = (m_Event.GetProperty()->GetIndexInParent() == 0) ? BB.Min : BB.Max;

        for (unsigned int i = 0; i < 3; i++)
        {
            if (!Tokenizer.HasMoreTokens()) return;
            if (!Tokenizer.GetNextToken().ToCDouble(&v[i])) return;
        }

        if (Tokenizer.HasMoreTokens()) return;
#endif
    }
    else if (m_Depth == 2)
    {
        // A single value, e.g. BB.Min.x, or BB.Max.z, has changed.
        const unsigned int i = m_Event.GetProperty()->GetIndexInParent();

        if (i >= 3) return;
        if (!m_Event.GetProperty()->GetParent()) return;

        if (m_Event.GetProperty()->GetParent()->GetIndexInParent() == 0)
            BB.Min[i] = m_Event.GetValue().GetDouble();
        else
            BB.Max[i] = m_Event.GetValue().GetDouble();
    }
    else
    {
        wxASSERT(false);
    }

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarT<BoundingBox3dT>(m_DocAdapter, Var, BB);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    wxASSERT(m_Depth == 0);

    ArrayT<uint32_t>  A;
    unsigned long     ul = 0;
    wxStringTokenizer Tokenizer(m_Event.GetValue().GetString());

    while (Tokenizer.HasMoreTokens())
        A.PushBack(Tokenizer.GetNextToken().ToCULong(&ul) ? ul : 0);

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarArrayT<uint32_t>(m_DocAdapter, Var, A);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    wxASSERT(m_Depth == 0);

    ArrayT<uint16_t>  A;
    unsigned long     ul = 0;
    wxStringTokenizer Tokenizer(m_Event.GetValue().GetString());

    while (Tokenizer.HasMoreTokens())
        A.PushBack(Tokenizer.GetNextToken().ToCULong(&ul) ? ul : 0);

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarArrayT<uint16_t>(m_DocAdapter, Var, A);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    wxASSERT(m_Depth == 0);

    ArrayT<uint8_t>   A;
    unsigned long     ul = 0;
    wxStringTokenizer Tokenizer(m_Event.GetValue().GetString());

    while (Tokenizer.HasMoreTokens())
        A.PushBack(Tokenizer.GetNextToken().ToCULong(&ul) ? ul : 0);

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarArrayT<uint8_t>(m_DocAdapter, Var, A);
}


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarArrayT<std::string>& Var)
{
    wxASSERT(m_Depth == 0);

    ArrayT<std::string> A;
    wxStringTokenizer   Tokenizer(m_Event.GetValue().GetString(), "\n", wxTOKEN_RET_EMPTY);

    while (Tokenizer.HasMoreTokens())
        A.PushBack(std::string(Tokenizer.GetNextToken()));

    wxASSERT(m_Command == NULL);
    m_Command = new CommandSetCompVarArrayT<std::string>(m_DocAdapter, Var, A);
}
