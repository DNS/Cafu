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

#include "ToolNewLight.hpp"
#include "ChildFrame.hpp"
#include "GameConfig.hpp"
#include "MapDocument.hpp"
#include "ToolOptionsBars.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolNewLightT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolNewLightT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolNewLightT::TypeInfo(GetToolTIM(), "ToolNewLightT", "ToolNewEntityT", ToolNewLightT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolNewLightT::ToolNewLightT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolNewEntityT(MapDoc, ToolMan, ParentOptionsBar, false /* Don't create the "New Entity" options bar. */),
      m_NewLightOptionsBar(new OptionsBar_NewLightToolT(ParentOptionsBar, MapDoc))
{
}


wxWindow* ToolNewLightT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_NewLightOptionsBar;
}


const EntityClassT* ToolNewLightT::GetNewEntClass() const
{
    return m_MapDoc.GetGameConfig()->FindClass(m_NewLightOptionsBar->m_LightChoice->GetStringSelection());
}
