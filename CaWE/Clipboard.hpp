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

#ifndef CAFU_MAPEDITOR_CLIPBOARD_HPP_INCLUDED
#define CAFU_MAPEDITOR_CLIPBOARD_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GameSys { class EntityT; } }
class MapElementT;
class MapPrimitiveT;


namespace MapEditor
{
    /// This class represents the Map Editors clipboard.
    class ClipboardT
    {
        public:

        ClipboardT();
        ~ClipboardT();

        void CopyFrom(const ArrayT<MapElementT*>& Elems);
        void Clear();
        void SetOriginalCenter(const Vector3fT& Center) { m_OriginalCenter = Center; }

        const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& GetEntities() const { return m_Entities; }
        const ArrayT<MapPrimitiveT*>& GetPrimitives() const { return m_Primitives; }
        const Vector3fT& GetOriginalCenter() const { return m_OriginalCenter; }


        private:

        ClipboardT(const ClipboardT&);          ///< Use of the Copy    Constructor is not allowed.
        void operator = (const ClipboardT&);    ///< Use of the Assignment Operator is not allowed.

        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > m_Entities;
        ArrayT<MapPrimitiveT*>                        m_Primitives;
        Vector3fT                                     m_OriginalCenter;
    };
}

#endif
