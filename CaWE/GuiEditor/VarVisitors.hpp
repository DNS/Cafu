/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_GUIEDITOR_VAR_VISITORS_PROPERTIES_HPP_INCLUDED
#define CAFU_GUIEDITOR_VAR_VISITORS_PROPERTIES_HPP_INCLUDED

#include "Variables.hpp"


namespace cf { namespace TypeSys { class TypeInfoT; } }
class wxPGProperty;
class wxPropertyGridEvent;
class wxPropertyGridManager;


namespace GuiEditor
{
    class ChildFrameT;
    class GuiDocumentT;


    /// This visitor creates a wxPGProperty for the visited variable
    /// and adds it to the given wxPropertyGridManager.
    class VarVisitorAddPropT : public cf::TypeSys::VisitorT
    {
        public:

        VarVisitorAddPropT(wxPropertyGridManager& PropMan, GuiDocumentT* GuiDoc, const cf::TypeSys::TypeInfoT* TI);

        void visit(cf::TypeSys::VarT<float>& Var);
        void visit(cf::TypeSys::VarT<double>& Var);
        void visit(cf::TypeSys::VarT<int>& Var);
        void visit(cf::TypeSys::VarT<unsigned int>& Var);
        void visit(cf::TypeSys::VarT<bool>& Var);
        void visit(cf::TypeSys::VarT<std::string>& Var);
        void visit(cf::TypeSys::VarT<Vector2fT>& Var);
        void visit(cf::TypeSys::VarT<Vector3fT>& Var);
        void visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var);


        private:

        void SetHelpString(wxPGProperty* Prop, wxPGProperty* SourceProp = NULL) const;

        wxPropertyGridManager&        m_PropMan;
        GuiDocumentT*                 m_GuiDoc;
        const cf::TypeSys::TypeInfoT* m_TI;
    };


    /// This visitor updates the value of the given property `Prop` to the value of the visited variable.
    class VarVisitorUpdatePropT : public cf::TypeSys::VisitorConstT
    {
        public:

        VarVisitorUpdatePropT(wxPGProperty& Prop);

        void visit(const cf::TypeSys::VarT<float>& Var);
        void visit(const cf::TypeSys::VarT<double>& Var);
        void visit(const cf::TypeSys::VarT<int>& Var);
        void visit(const cf::TypeSys::VarT<unsigned int>& Var);
        void visit(const cf::TypeSys::VarT<bool>& Var);
        void visit(const cf::TypeSys::VarT<std::string>& Var);
        void visit(const cf::TypeSys::VarT<Vector2fT>& Var);
        void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
        void visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var);


        private:

        wxPGProperty& m_Prop;
    };


    /// This visitor updates the value of the visited variable according to the given EVT_PG_CHANGING event.
    class VarVisitorHandlePropChangingEventT : public cf::TypeSys::VisitorT
    {
        public:

        VarVisitorHandlePropChangingEventT(wxPropertyGridEvent& Event, ChildFrameT* ChildFrame);

        const bool Ok() const { return m_Ok; }

        void visit(cf::TypeSys::VarT<float>& Var);
        void visit(cf::TypeSys::VarT<double>& Var);
        void visit(cf::TypeSys::VarT<int>& Var);
        void visit(cf::TypeSys::VarT<unsigned int>& Var);
        void visit(cf::TypeSys::VarT<bool>& Var);
        void visit(cf::TypeSys::VarT<std::string>& Var);
        void visit(cf::TypeSys::VarT<Vector2fT>& Var);
        void visit(cf::TypeSys::VarT<Vector3fT>& Var);
        void visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var);


        private:

        wxPropertyGridEvent& m_Event;
        ChildFrameT*         m_ChildFrame;
        GuiDocumentT*        m_GuiDoc;
        bool                 m_Ok;
    };


    /// This visitor is similar to VarVisitorHandlePropChangingEventT:
    /// It updates the value of the visited variable when a *sub-property* of the variable has changed.
    /// The given EVT_PG_CHANGING event refers to the change of the sub-property.
    class VarVisitorHandleSubChangingEventT : public cf::TypeSys::VisitorT
    {
        public:

        VarVisitorHandleSubChangingEventT(wxPropertyGridEvent& Event, ChildFrameT* ChildFrame);

        const bool Ok() const { return m_Ok; }

        void visit(cf::TypeSys::VarT<float>& Var);
        void visit(cf::TypeSys::VarT<double>& Var);
        void visit(cf::TypeSys::VarT<int>& Var);
        void visit(cf::TypeSys::VarT<unsigned int>& Var);
        void visit(cf::TypeSys::VarT<bool>& Var);
        void visit(cf::TypeSys::VarT<std::string>& Var);
        void visit(cf::TypeSys::VarT<Vector2fT>& Var);
        void visit(cf::TypeSys::VarT<Vector3fT>& Var);
        void visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var);


        private:

        wxPropertyGridEvent& m_Event;
        ChildFrameT*         m_ChildFrame;
        GuiDocumentT*        m_GuiDoc;
        bool                 m_Ok;
    };
}

#endif
