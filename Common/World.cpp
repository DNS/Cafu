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

/********************/
/*** World (Code) ***/
/********************/

#include "World.hpp"
#include "CompGameEntity.hpp"
#include "Bitmap/Bitmap.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "SceneGraph/_aux.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "String.hpp"

#include <cassert>
#include <cstring>
#include <fstream>


/************/
/*** MapT ***/
/************/

const double MapT::RoundEpsilon = 2.0;
const double MapT::MinVertexDist=10.0;  // 1 cm


/**************/
/*** WorldT ***/
/**************/

WorldT::WorldT()
    : m_ScriptWorld(NULL)
{
}


WorldT::~WorldT()
{
    delete m_ScriptWorld;
}


static std::string GetModDir(const char* FileName)
{
    std::string ModDir=FileName;

    // Determine the game directory, cleverly assuming that the destination file is in "Worlds".
    // Strip the file name, extention and the "Worlds" directory off.
    size_t i=ModDir.find_last_of("/\\");

    ModDir=ModDir.substr(0, i==std::string::npos ? 0 : i);

    i=ModDir.find_last_of("/\\");

    ModDir=ModDir.substr(0, i==std::string::npos ? 0 : i);

    return ModDir;
}


WorldT::WorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, ProgressFunctionT ProgressFunction) /*throw (LoadErrorT)*/
    : m_ScriptWorld(NULL)
{
    // Set the plant descriptions manager mod directory to the one from the world to load.
    PlantDescrMan.SetModDir(GetModDir(FileName));

    try
    {
        std::string ScriptName = cf::String::StripExt(FileName) + ".cent";
        ScriptName = cf::String::Replace(ScriptName, "/Worlds/", "/Maps/");
        ScriptName = cf::String::Replace(ScriptName, "\\Worlds\\", "\\Maps\\");

        m_ScriptWorld = new cf::GameSys::WorldT(
            ScriptName,
            ModelMan,
            GuiRes,
            0 /*cf::GameSys::WorldT::InitFlag_InMapEditor*/);
    }
    catch (const cf::GameSys::WorldT::InitErrorT& IE)
    {
        throw LoadErrorT(IE.what());
    }

    std::ifstream InFile(FileName, std::ios::in | std::ios::binary);
    if (InFile.bad()) throw LoadErrorT("Unable to open Cafu world file.");

    // Determine the size of the input file.
    InFile.seekg(0, std::ios::end);
    const std::streampos InFileSize=InFile.tellg();
    InFile.seekg(0, std::ios::beg);
    if (ProgressFunction) ProgressFunction(float(InFile.tellg())/float(InFileSize), "Opening file.");

    char FileHeader[32];
    InFile.read(FileHeader, 32);
    if (strcmp(FileHeader, "CARSTEN WORLD BSP FILE.")) throw LoadErrorT("Invalid file header. Not a Cafu world file.");

    unsigned short FileVersion;
    InFile.read((char*)&FileVersion, sizeof(FileVersion));
    if (FileVersion != 29) throw LoadErrorT("Invalid file version. Current version is 29.");

    cf::SceneGraph::aux::PoolT Pool;


    // 1.3. Read Map Faces, Map LightMaps, Map SHLMaps, and Map FacesDrawIndices.
    InFile.read((char*)&cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize, sizeof(cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize));

    InFile.read((char*)&cf::SceneGraph::SHLMapManT::NrOfBands , sizeof(cf::SceneGraph::SHLMapManT::NrOfBands ));
    InFile.read((char*)&cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize , sizeof(cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize ));
    InFile.read((char*)&cf::SceneGraph::SHLMapManT::NrOfRepres, sizeof(cf::SceneGraph::SHLMapManT::NrOfRepres));

    // Read the lookup-table the indices are referring to (nothing is written when FaceT::SHLMapInfoT::NrOfRepres==0 (uncompressed data)).
    SHLMapMan.ReadSHLCoeffsTable(InFile);


    // 3. Read InfoPlayerStarts
    InfoPlayerStarts.Clear();
    if (ProgressFunction) ProgressFunction(float(InFile.tellg())/float(InFileSize), "Reading InfoPlayerStarts.");
    for (unsigned long Size=cf::SceneGraph::aux::ReadUInt32(InFile); Size>0; Size--)
    {
        InfoPlayerStartT IPS;

        InFile.read((char*)&IPS.Origin.x, sizeof(IPS.Origin.x));
        InFile.read((char*)&IPS.Origin.y, sizeof(IPS.Origin.y));
        InFile.read((char*)&IPS.Origin.z, sizeof(IPS.Origin.z));
        InFile.read((char*)&IPS.Heading , sizeof(IPS.Heading ));
        InFile.read((char*)&IPS.Pitch   , sizeof(IPS.Pitch   ));
        InFile.read((char*)&IPS.Bank    , sizeof(IPS.Bank    ));

        InfoPlayerStarts.PushBack(IPS);
    }

    // Read Map PointLights
    PointLights.Clear();
    if (ProgressFunction) ProgressFunction(float(InFile.tellg())/float(InFileSize), "Reading PointLights.");
    for (unsigned long Size=cf::SceneGraph::aux::ReadUInt32(InFile); Size>0; Size--)
    {
        PointLightT PL;

        InFile.read((char*)&PL.Origin.x   , sizeof(PL.Origin.x   ));
        InFile.read((char*)&PL.Origin.y   , sizeof(PL.Origin.y   ));
        InFile.read((char*)&PL.Origin.z   , sizeof(PL.Origin.z   ));
        InFile.read((char*)&PL.Dir.x      , sizeof(PL.Dir.x      ));
        InFile.read((char*)&PL.Dir.y      , sizeof(PL.Dir.y      ));
        InFile.read((char*)&PL.Dir.z      , sizeof(PL.Dir.z      ));
        InFile.read((char*)&PL.Angle      , sizeof(PL.Angle      ));
        InFile.read((char*)&PL.Intensity.x, sizeof(PL.Intensity.x));
        InFile.read((char*)&PL.Intensity.y, sizeof(PL.Intensity.y));
        InFile.read((char*)&PL.Intensity.z, sizeof(PL.Intensity.z));

        PointLights.PushBack(PL);
    }

    // 5. Read GameEntities
    if (ProgressFunction) ProgressFunction(float(InFile.tellg())/float(InFileSize), "Reading Game Entities.");

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    const unsigned int NumGameEnts = cf::SceneGraph::aux::ReadUInt32(InFile);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        IntrusivePtrT<CompGameEntityT> GameEnt = EntNr < NumGameEnts
            ? new CompGameEntityT(InFile, Pool, ModelMan, LightMapMan, SHLMapMan, PlantDescrMan)
            : new CompGameEntityT(EntNr);

        // We really should be able to remove the m_MFIndex property sooner rather than later...
        assert(GameEnt->m_MFIndex == EntNr);

        assert(AllEnts[EntNr]->GetApp().IsNull());
        AllEnts[EntNr]->SetApp(GameEnt);
    }

    if (ProgressFunction) ProgressFunction(1.0f, "World file loaded.");
}


void WorldT::SaveToDisk(const char* FileName) const /*throw (SaveErrorT)*/
{
    std::ofstream OutFile(FileName, std::ios::out | std::ios::binary);
    if (OutFile.bad()) throw SaveErrorT("Unable to open world BSP file.");

    cf::SceneGraph::aux::PoolT Pool;

    char           FileHeader[32]="CARSTEN WORLD BSP FILE."; OutFile.write(FileHeader, 32);
    unsigned short FileVersion   =29;                        OutFile.write((char*)&FileVersion, sizeof(FileVersion));


    // 1.3. Write Map Faces, Map LightMaps, Map SHLMaps, and Map FacesDrawIndices.
    OutFile.write((char*)&cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize, sizeof(cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize));

    OutFile.write((char*)&cf::SceneGraph::SHLMapManT::NrOfBands , sizeof(cf::SceneGraph::SHLMapManT::NrOfBands ));
    OutFile.write((char*)&cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize , sizeof(cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize ));
    OutFile.write((char*)&cf::SceneGraph::SHLMapManT::NrOfRepres, sizeof(cf::SceneGraph::SHLMapManT::NrOfRepres));

    // Write the lookup-table the indices are referring to (nothing is written when cf::SceneGraph::SHLMapManT::NrOfRepres==0 (uncompressed data)).
    // This writes a total of cf::SceneGraph::SHLMapManT::NrOfRepres*NR_OF_SH_COEFFS coefficients.
    SHLMapMan.WriteSHLCoeffsTable(OutFile);


    // 3. Write InfoPlayerStarts
    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(InfoPlayerStarts.Size()));
    for (unsigned long IPSNr=0; IPSNr<InfoPlayerStarts.Size(); IPSNr++)
    {
        const InfoPlayerStartT& IPS=InfoPlayerStarts[IPSNr];

        OutFile.write((char*)&IPS.Origin.x, sizeof(IPS.Origin.x));
        OutFile.write((char*)&IPS.Origin.y, sizeof(IPS.Origin.y));
        OutFile.write((char*)&IPS.Origin.z, sizeof(IPS.Origin.z));
        OutFile.write((char*)&IPS.Heading , sizeof(IPS.Heading ));
        OutFile.write((char*)&IPS.Pitch   , sizeof(IPS.Pitch   ));
        OutFile.write((char*)&IPS.Bank    , sizeof(IPS.Bank    ));
    }

    // Write Map PointLights
    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(PointLights.Size()));
    for (unsigned long PLNr=0; PLNr<PointLights.Size(); PLNr++)
    {
        const PointLightT& PL=PointLights[PLNr];

        OutFile.write((char*)&PL.Origin.x   , sizeof(PL.Origin.x   ));
        OutFile.write((char*)&PL.Origin.y   , sizeof(PL.Origin.y   ));
        OutFile.write((char*)&PL.Origin.z   , sizeof(PL.Origin.z   ));
        OutFile.write((char*)&PL.Dir.x      , sizeof(PL.Dir.x      ));
        OutFile.write((char*)&PL.Dir.y      , sizeof(PL.Dir.y      ));
        OutFile.write((char*)&PL.Dir.z      , sizeof(PL.Dir.z      ));
        OutFile.write((char*)&PL.Angle      , sizeof(PL.Angle      ));
        OutFile.write((char*)&PL.Intensity.x, sizeof(PL.Intensity.x));
        OutFile.write((char*)&PL.Intensity.y, sizeof(PL.Intensity.y));
        OutFile.write((char*)&PL.Intensity.z, sizeof(PL.Intensity.z));
    }

    // 5. Write GameEntities
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(AllEnts.Size()));

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        GetGameEnt(AllEnts[EntNr])->WriteTo(OutFile, Pool);
    }
}
