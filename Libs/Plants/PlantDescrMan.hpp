/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_PLANT_DESCR_MAN_HPP_INCLUDED
#define CAFU_PLANT_DESCR_MAN_HPP_INCLUDED

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
