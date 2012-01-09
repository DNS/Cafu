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

#ifndef CAFU_TOOL_NEW_LIGHT_HPP_INCLUDED
#define CAFU_TOOL_NEW_LIGHT_HPP_INCLUDED

#include "ToolNewEntity.hpp"


class OptionsBar_NewLightToolT;


/// This class implements the "New Light" tool.
/// It is really just a "New Entity" tool that is specialized on only creating new *light* point entities.
class ToolNewLightT : public ToolNewEntityT
{
    public:

    ToolNewLightT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);

    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_NEWLIGHT; }
    wxWindow* GetOptionsBar();

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    // Implementations/overrides of ToolNewEntityT methods.
    const EntityClassT* GetNewEntClass() const;

    OptionsBar_NewLightToolT* m_NewLightOptionsBar;   ///< The options bar for this tool.
};

#endif
