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

#ifndef CAFU_MAPEDITOR_COMPONENT_MAP_ENTITY_HPP_INCLUDED
#define CAFU_MAPEDITOR_COMPONENT_MAP_ENTITY_HPP_INCLUDED

#include "GameSys/CompBase.hpp"


class MapEntityBaseT;


namespace MapEditor
{
    /******************************************************************************/
    /*** TODO: Implement a strict non-NULL policy for the m_MapEntity member??? ***/
    /******************************************************************************/

    /// This component houses a MapEntityBaseT instance for its entity.
    /// It is intended for use by the Map Editor application only, that is, as the "App" component of `cf::GameSys::EntityT`s.
    /// As such, it doesn't integrate with the TypeSys, and thus isn't available for scripting and whereever else we need
    /// the related meta-data.
    class CompMapEntityT : public cf::GameSys::ComponentBaseT
    {
        public:

        /// The constructor.
        CompMapEntityT(MapEntityBaseT* MapEnt);

        /// The copy constructor.
        /// @param Comp   The component to create a copy of.
        CompMapEntityT(const CompMapEntityT& Comp);

        /// The destructor.
        ~CompMapEntityT();

        MapEntityBaseT* GetMapEntity() { return m_MapEntity; }
        void SetMapEntity(MapEntityBaseT* MapEnt) { m_MapEntity = MapEnt; }

        // Base class overrides.
        CompMapEntityT* Clone() const;
        const char* GetName() const { return "MapEntity"; }
        void Render() const;


        private:

        MapEntityBaseT* m_MapEntity;
    };
}

#endif
