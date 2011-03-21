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

#ifndef _DIALOG_CREATE_ARCH_HPP_
#define _DIALOG_CREATE_ARCH_HPP_

#include "Math3D/Vector3.hpp"
#include "Math3D/BoundingBox.hpp"
#include "wx/wx.h"


class AxesInfoT;
class EditorMaterialI;
class MapPrimitiveT;
class wxSpinCtrl;
class wxSpinEvent;


class ArchDialogT : public wxDialog
{
    public:

    ArchDialogT(const BoundingBox3fT& BB, const AxesInfoT& AxesInfo, wxWindow* Parent=NULL);

    ArrayT<MapPrimitiveT*> GetArch(EditorMaterialI* Material) const;


    private:

    class PreviewWinT;

    void OnSpinEvent(wxSpinEvent& event);

    const BoundingBox3fT m_BB;
    int                  m_WallWidth;
    int                  m_NrOfSegments;
    int                  m_Arc;
    int                  m_StartAngle;
    int                  m_AddHeight;
    PreviewWinT*         m_PreviewWin;

    DECLARE_EVENT_TABLE()
};

#endif
