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

#ifndef _MAP_HELPER_HPP_
#define _MAP_HELPER_HPP_

#include "MapElement.hpp"


class MapEntityT;
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }


class MapHelperT : public MapElementT
{
    public:

    /// The default constructor.
    MapHelperT(const MapEntityT* ParentEntity);

    /// The copy constructor for copying a helper.
    /// @param Helper   The helper to copy-construct this helper from.
    MapHelperT(const MapHelperT& Helper);

    /// This method (re-)sets the parent entity that we are a helper for,
    /// for use after an entity (with all its helpers) has been cloned.
    void SetParentEntity(const MapEntityT* ParentEntity);


    // Implementations and overrides for base class methods.
    MapHelperT* Clone() const=0;
    void        Assign(const MapElementT* Elem);


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    const MapEntityT* m_ParentEntity;   ///< Our parent entity that we are a helper for.
};

#endif
