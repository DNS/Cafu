/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
