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

#ifndef _DIALOG_INSP_MAP_SCRIPT_HPP_
#define _DIALOG_INSP_MAP_SCRIPT_HPP_

#include "ObserverPattern.hpp"
#include "wx/panel.h"


class wxNotebook;
class MapDocumentT;


class InspDlgMapScriptT : public wxPanel, public ObserverT
{
    public:

    InspDlgMapScriptT(wxNotebook* Parent_, MapDocumentT* MapDoc_);
    ~InspDlgMapScriptT();

    // Implementation of the ObserverT interface.
    void NotifySubjectDies(SubjectT* dyingSubject);


    private:

    MapDocumentT* MapDoc;
};

#endif
