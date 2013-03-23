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

#ifndef CAFU_GUIEDITOR_SET_COMPONENT_VARIABLE_HPP_INCLUDED
#define CAFU_GUIEDITOR_SET_COMPONENT_VARIABLE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Network/State.hpp"


namespace cf { namespace TypeSys { template<class T> class VarT; } }


namespace GuiEditor
{
    class GuiDocumentT;


    template<class T>
    class CommandSetCompVarT : public CommandT
    {
        public:

        /// The constructor for setting the given variable to a new value.
        CommandSetCompVarT(GuiDocumentT* GuiDoc, cf::TypeSys::VarT<T>& Var, const T& NewValue);

        /// The constructor to be used when the variable has already been set to the new value.
        /// With this constructor, the command is initialized in the "already done" state.
        CommandSetCompVarT(GuiDocumentT* GuiDoc, cf::TypeSys::VarT<T>& Var, const cf::Network::StateT& OldState);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT*             m_GuiDoc;
        cf::TypeSys::VarT<T>&     m_Var;
        const cf::Network::StateT m_OldState;
        const T                   m_NewValue;
    };
}

#endif
