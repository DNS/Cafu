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

#include "PlantDescrMan.hpp"

#include "PlantDescription.hpp"
#include "TextParser/TextParser.hpp"


PlantDescrManT::PlantDescrManT(const std::string& BaseDir)
    : m_BaseDir(BaseDir)
{
}


PlantDescrManT::~PlantDescrManT()
{
    for (std::map<std::string, PlantDescriptionT*>::const_iterator PlantDescrIt=m_PlantDescriptions.begin(); PlantDescrIt!=m_PlantDescriptions.end(); PlantDescrIt++)
        delete PlantDescrIt->second;

    m_PlantDescriptions.clear();
}


PlantDescriptionT* PlantDescrManT::GetPlantDescription(const std::string& DescrFileName)
{
    std::map<std::string, PlantDescriptionT*>::const_iterator It=m_PlantDescriptions.find(DescrFileName);

    if (It!=m_PlantDescriptions.end()) return It->second;

    // Load plant description.
    TextParserT        TP((m_BaseDir+"/"+DescrFileName).c_str());
    PlantDescriptionT* NewPlantDescr=new PlantDescriptionT(TP, DescrFileName);

    m_PlantDescriptions[DescrFileName]=NewPlantDescr;

    return NewPlantDescr;
}
