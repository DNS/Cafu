/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_COMMAND_REPARENT_PRIMITIVE_HPP_INCLUDED
#define CAFU_COMMAND_REPARENT_PRIMITIVE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace MapEditor { class CompMapEntityT; }
class MapDocumentT;
class MapPrimitiveT;


namespace MapEditor
{
    /// A command to assign one or several map primitives to another entity.
    class CommandReparentPrimitiveT : public CommandT
    {
        public:

        /// The constructor for assigning a map primitive to another entity.
        CommandReparentPrimitiveT(MapDocumentT& MapDoc, MapPrimitiveT* Prim, IntrusivePtrT<CompMapEntityT> NewParent);

        /// The constructor for assigning several map primitives to another entity.
        CommandReparentPrimitiveT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& Prims, IntrusivePtrT<CompMapEntityT> NewParent);

        // Implementation of the CommandT interface.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        MapDocumentT&                           m_MapDoc;
        ArrayT<MapPrimitiveT*>                  m_Prims;
        ArrayT< IntrusivePtrT<CompMapEntityT> > m_OldParents;
        IntrusivePtrT<CompMapEntityT>           m_NewParent;
    };
}

#endif
