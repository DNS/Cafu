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

#ifndef _PLANT_DESCR_MAN_HPP_
#define _PLANT_DESCR_MAN_HPP_

#include <string>
#include <map>


struct PlantDescriptionT;


/// The plant description manager holds and manages all plant descriptions so they can be shared
/// with multiple different plants.
class PlantDescrManT
{
    public:

    /// Constructor.
    /// @param BaseDir The base directory in which all plant descriptions are contained.
    PlantDescrManT(const std::string& BaseDir="");

    /// Destrcutor.
    ~PlantDescrManT();

    /// Gets a plant description identified by its filename.
    /// @param DescrFileName The file name of the plant description (relative to the plant description base dir).
    /// @return Pointer to the plant description or a dummy plant description on error.
    PlantDescriptionT* GetPlantDescription(const std::string& DescrFileName);

    /// Changes the base directory for the plant descriptions.
    /// @param BaseDir The new base directory.
    void SetModDir(const std::string& BaseDir) { m_BaseDir=BaseDir; }


    private:

    PlantDescrManT(const PlantDescrManT&);      // Use of the Copy Constructor    is not allowed.
    void operator = (const PlantDescrManT&);    // Use of the Assignment Operator is not allowed.

    std::map<std::string, PlantDescriptionT*> m_PlantDescriptions;

    std::string m_BaseDir;
};


#endif
