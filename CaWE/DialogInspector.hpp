/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _DIALOG_INSPECTOR_HPP_
#define _DIALOG_INSPECTOR_HPP_

#include "Templates/Array.hpp"
#include "wx/panel.h"


class wxNotebook;
class InspDlgEntityTreeT;
class InspDlgEntityPropsT;
class InspDlgPrimitivePropsT;
class InspDlgMapScriptT;
class MapDocumentT;
class MapElementT;


class InspectorDialogT : public wxPanel
{
    public:

    InspectorDialogT(wxWindow* Parent, MapDocumentT* MapDoc);

    void ChangePage(int Tab);

    /// Based on the current selection, (try to) make an intelligent guess on which page of the dialog
    /// (scene graph, entity properties or primitive properties) should be shown.
    int GetBestPage(const ArrayT<MapElementT*>& Selection) const;

    private:

    wxNotebook*             Notebook;
    InspDlgEntityTreeT*     EntityTree;
    InspDlgEntityPropsT*    EntityProps;
    InspDlgPrimitivePropsT* PrimitiveProps;
    InspDlgMapScriptT*      MapScript;
};

#endif
