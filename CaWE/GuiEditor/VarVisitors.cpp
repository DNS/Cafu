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
    ;
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
}


/*****************************************/
/*** VarVisitorHandlePropChangedEventT ***/
/*****************************************/

VarVisitorHandlePropChangedEventT::VarVisitorHandlePropChangedEventT(wxPropertyGridEvent& Event, ChildFrameT* ChildFrame)
    : m_Event(Event),
      m_ChildFrame(ChildFrame),
      m_GuiDoc(ChildFrame->GetGuiDoc()),
      m_Ok(true)
{
}


void VarVisitorHandlePropChangedEventT::visit(cf::TypeSys::VarT<float>& Var)
{
    const float f = m_Event.GetProperty()->GetValue().GetDouble();

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<float>(m_GuiDoc, Var, f));
}


void VarVisitorHandlePropChangedEventT::visit(cf::TypeSys::VarT<double>& Var)
{
    const double d = m_Event.GetProperty()->GetValue().GetDouble();

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<double>(m_GuiDoc, Var, d));
}


void VarVisitorHandlePropChangedEventT::visit(cf::TypeSys::VarT<int>& Var)
{
    const int i = m_Event.GetProperty()->GetValue().GetLong();

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<int>(m_GuiDoc, Var, i));
}


void VarVisitorHandlePropChangedEventT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    const std::string s = std::string(m_Event.GetProperty()->GetValueAsString());

    m_Ok = m_ChildFrame->SubmitCommand(new CommandSetCompVarT<std::string>(m_GuiDoc, Var, s));
}


void VarVisitorHandlePropChangedEventT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
}
