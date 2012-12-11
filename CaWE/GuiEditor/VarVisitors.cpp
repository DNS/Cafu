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
#include "Commands/SetCompVar.hpp"

//#include "wx/propgrid/property.h"
#include "wx/propgrid/manager.h"
//#include "wx/propgrid/advprops.h"


using namespace GuiEditor;


/**************************/
/*** VarVisitorAddPropT ***/
/**************************/

VarVisitorAddPropT::VarVisitorAddPropT(wxPropertyGridManager& PropMan)
    : m_PropMan(PropMan)
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
    wxPGProperty* Prop = new wxIntProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());
 // wxPGProperty* Prop = new wxLongStringProperty(Var.GetName(), wxString::Format("%p", &Var), Var.Get());

    m_PropMan.Append(Prop)->SetClientData(&Var);
}


void VarVisitorAddPropT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    wxPGProperty* Prop = new wxStringProperty(Var.GetName(), wxString::Format("%p", &Var), "<composed>");

    m_PropMan.Append(Prop)->SetClientData(&Var);

    m_PropMan.AppendIn(Prop, new wxFloatProperty("x", wxPG_LABEL, Var.Get().x))->SetTextColour(wxColour(200, 0, 0));
    m_PropMan.AppendIn(Prop, new wxFloatProperty("y", wxPG_LABEL, Var.Get().y))->SetTextColour(wxColour(0, 200, 0));
    m_PropMan.AppendIn(Prop, new wxFloatProperty("z", wxPG_LABEL, Var.Get().z))->SetTextColour(wxColour(0, 0, 200));
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
    m_Prop.SetValue(Var.Get());
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<std::string>& Var)
{
    m_Prop.SetValue/*FromString*/(Var.Get());
}


void VarVisitorUpdatePropT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)
{
    const unsigned int Count = std::min(3u, m_Prop.GetChildCount());

    for (unsigned int i = 0; i < Count; i++)
        m_Prop.Item(i)->SetValue(Var.Get()[i]);
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


void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    const std::string s = std::string(m_Event.GetValue().GetString());

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<std::string>(m_GuiDoc, Var, s));
}


// This is a "<composed>" property, and its summary string is changing.
// For example, the value could be changing from "100.0; 0.0; 50.0" to "100.0; 150; 200.0".
void VarVisitorHandlePropChangingEventT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT         v;
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

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<Vector3fT>(m_GuiDoc, Var, v));
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
void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<std::string>& Var) { wxASSERT(false); }


void VarVisitorHandleSubChangingEventT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT          v = Var.Get();
    const unsigned int i = m_Event.GetProperty()->GetIndexInParent();

    // On error, return with m_Ok == false.
    if (i >= 3) return;

    v[i] = m_Event.GetValue().GetDouble();
    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<Vector3fT>(m_GuiDoc, Var, v));
}
