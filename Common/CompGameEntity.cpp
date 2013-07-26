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

#include "CompGameEntity.hpp"
#include "World.hpp"


/*
 * Note that as an alternative to using entity IDs as indices into WorldT::m_StaticEntityData[] as has been
 * introduced in the revisions of 2013-07-26, it might be possible to let this class have a reference to the
 * WorldT::m_StaticEntityData[] array, add an `unsigned int` member for the world file index, and to
 * serialize/deserialize this index as part of the component. (`ComponentBaseT::Serialize()` would have to
 * call a virtual `DoSerialize()` method that we had to override here.)
 * This would liberate the entity ID from the requirements of double-purpose use, and would allow us to
 * actually *copy* entities with associated world file information. But is it worth the effort?
 * See the revisions of 2013-07-26 for additional details.
 */

CompGameEntityT::CompGameEntityT(StaticEntityDataT* SED)
    : m_StaticEntityData(SED ? SED : new StaticEntityDataT()),
      m_DeleteSED(SED == NULL)
{
}


CompGameEntityT::CompGameEntityT(const CompGameEntityT& Comp)
    : m_StaticEntityData(new StaticEntityDataT()),
      m_DeleteSED(true)
{
    // A CompGameEntityT should actually never be copied...
    // (because the m_StaticEntityData cannot be copied -- but see the /*...*/ comment above).
    assert(false);
}


CompGameEntityT::~CompGameEntityT()
{
    if (m_DeleteSED)
        delete m_StaticEntityData;
}


CompGameEntityT* CompGameEntityT::Clone() const
{
    return new CompGameEntityT(*this);
}
