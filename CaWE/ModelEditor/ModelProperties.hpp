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

#ifndef _MODELEDITOR_MODEL_PROPERTIES_HPP_
#define _MODELEDITOR_MODEL_PROPERTIES_HPP_

#include "wx/wx.h"
#include "wx/propgrid/manager.h"


namespace ModelEditor
{
    class ChildFrameT;


    class ModelPropertiesT : public wxPropertyGridManager
    {
        public:

        ModelPropertiesT(ChildFrameT* Parent, const wxSize& Size);

        void RefreshPropGrid();


        private:

        ChildFrameT* m_Parent;

        void OnPropertyGridChanged(wxPropertyGridEvent& Event);

        DECLARE_EVENT_TABLE()
    };
}

#endif
