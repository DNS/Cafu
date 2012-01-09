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

/********************/
/*** World (Code) ***/
/********************/

#include "World.hpp"
#include "Bitmap/Bitmap.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "SceneGraph/_aux.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"

#include <cassert>
#include <cstring>
#include <fstream>


/************/
/*** MapT ***/
/************/

const double MapT::RoundEpsilon = 2.0;
const double MapT::MinVertexDist=10.0;  // 1 cm


/**********************/
/*** SharedTerrainT ***/
/**********************/

SharedTerrainT::SharedTerrainT(const BoundingBox3dT& BB_, unsigned long SideLength_, const ArrayT<unsigned short>& HeightData_, MaterialT* Material_)
    : BB(BB_),
      SideLength(SideLength_),
      HeightData(HeightData_),
      Material(Material_),
      Terrain(&HeightData[0], SideLength, BB.AsBoxOfFloat())
{
}


SharedTerrainT::SharedTerrainT(std::istream& InFile)
{
    using namespace cf::SceneGraph;

    BB.Min    =aux::ReadVector3d(InFile);
    BB.Max    =aux::ReadVector3d(InFile);
    SideLength=aux::ReadUInt32(InFile);
    Material  =MaterialManager->GetMaterial(aux::ReadString(InFile));

    HeightData.PushBackEmptyExact(SideLength*SideLength);

    for (unsigned long i=0; i<HeightData.Size(); i++)
        HeightData[i]=aux::ReadUInt16(InFile);

    Terrain=TerrainT(&HeightData[0], SideLength, BB.AsBoxOfFloat());
}


void SharedTerrainT::WriteTo(std::ostream& OutFile) const
{
    using namespace cf::SceneGraph;

    aux::Write(OutFile, BB.Min);
    aux::Write(OutFile, BB.Max);
    aux::Write(OutFile, aux::cnc_ui32(SideLength));
    aux::Write(OutFile, Material->Name);    // There are only few terrains, no need for the Pool here.

    for (unsigned long i=0; i<HeightData.Size(); i++)
        aux::Write(OutFile, HeightData[i]);
}


/*******************/
/*** GameEntityT ***/
/*******************/

GameEntityT::GameEntityT(unsigned long MFIndex_)
    : MFIndex(MFIndex_),
      BspTree(NULL),
      CollModel(NULL)
{
}


GameEntityT::~GameEntityT()
{
    delete BspTree;
    delete CollModel;

    // Delete the terrains that were shared by the BspTree and the CollModel.
    for (unsigned long TerrainNr=0; TerrainNr<Terrains.Size(); TerrainNr++)
        delete Terrains[TerrainNr];
}


/**************/
/*** WorldT ***/
/**************/

WorldT::WorldT()
    : BspTree(new cf::SceneGraph::BspTreeNodeT),
      CollModel(NULL)
{
}


WorldT::~WorldT()
{
    delete BspTree;
    delete CollModel;

    // Delete the terrains that were shared by the BspTree and the CollModel.
    for (unsigned long TerrainNr=0; TerrainNr<Terrains.Size(); TerrainNr++)
        delete Terrains[TerrainNr];

    for (unsigned long EntityNr=0; EntityNr<GameEntities.Size(); EntityNr++)
        delete GameEntities[EntityNr];
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


WorldT::WorldT(const char* FileName, ModelManagerT& ModelMan, ProgressFunctionT ProgressFunction) /*throw (LoadErrorT)*/
    : BspTree(NULL),
      CollModel(NULL)
{
    // Set the plant descriptions manager mod directory to the one from the world to load.
    PlantDescrMan.SetModDir(GetModDir(FileName));

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
    if (FileVersion!=28) throw LoadErrorT("Invalid file version. Current version is 28.");

    cf::SceneGraph::aux::PoolT Pool;


    // 1.3. Read Map Faces, Map LightMaps, Map SHLMaps, and Map FacesDrawIndices.
    InFile.read((char*)&cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize, sizeof(cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize));

    InFile.read((char*)&cf::SceneGraph::SHLMapManT::NrOfBands , sizeof(cf::SceneGraph::SHLMapManT::NrOfBands ));
    InFile.read((char*)&cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize , sizeof(cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize ));
    InFile.read((char*)&cf::SceneGraph::SHLMapManT::NrOfRepres, sizeof(cf::SceneGraph::SHLMapManT::NrOfRepres));


    // Read the shared terrain data.
    ArrayT<const TerrainT*> ShTe_SceneGr;
    ArrayT<cf::ClipSys::CollisionModelStaticT::TerrainRefT> ShTe_CollDet;

    for (unsigned long TerrainNr=cf::SceneGraph::aux::ReadUInt32(InFile); TerrainNr>0; TerrainNr--)
    {
        SharedTerrainT* ShTe=new SharedTerrainT(InFile);

        Terrains.PushBack(ShTe);
        ShTe_SceneGr.PushBack(&ShTe->Terrain);
        ShTe_CollDet.PushBack(cf::ClipSys::CollisionModelStaticT::TerrainRefT(&ShTe->Terrain, ShTe->Material, ShTe->BB));
    }

    // Read the SceneGraph BSP tree.
    // Don't call cf::SceneGraph::BspTreeNodeT::CreateFromFile_cw() directly, because it would expect
    // that the "BspTree" string that states the identity of this NodeT has already been read!
    BspTree=dynamic_cast<cf::SceneGraph::BspTreeNodeT*>(cf::SceneGraph::GenericNodeT::CreateFromFile_cw(InFile, Pool,
        LightMapMan, SHLMapMan, PlantDescrMan, ShTe_SceneGr, ModelMan));

    if (BspTree==NULL) throw LoadErrorT("Could not read scene graph bsp tree node!");

    // Read the ClipSys collision model.
    CollModel=new cf::ClipSys::CollisionModelStaticT(InFile, Pool, ShTe_CollDet);


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
    assert(GameEntities.Size()==0);
    GameEntities.Clear();
    if (ProgressFunction) ProgressFunction(float(InFile.tellg())/float(InFileSize), "Reading Game Entities.");
    for (unsigned long Size=cf::SceneGraph::aux::ReadUInt32(InFile); Size>0; Size--)
    {
        GameEntityT* GE=new GameEntityT(cf::SceneGraph::aux::ReadUInt32(InFile) /*MFIndex*/);

        // Read the shared terrain data.
        ArrayT<const TerrainT*> ShTe_SceneGr;
        ArrayT<cf::ClipSys::CollisionModelStaticT::TerrainRefT> ShTe_CollDet;

        for (unsigned long TerrainNr=cf::SceneGraph::aux::ReadUInt32(InFile); TerrainNr>0; TerrainNr--)
        {
            SharedTerrainT* ShTe=new SharedTerrainT(InFile);

            GE->Terrains.PushBack(ShTe);
            ShTe_SceneGr.PushBack(&ShTe->Terrain);
            ShTe_CollDet.PushBack(cf::ClipSys::CollisionModelStaticT::TerrainRefT(&ShTe->Terrain, ShTe->Material, ShTe->BB));
        }

        // Read the SceneGraph BSP tree.
        // Don't call cf::SceneGraph::BspTreeNodeT::CreateFromFile_cw() directly, because it would expect
        // that the "BspTree" string that states the identity of this NodeT has already been read!
        GE->BspTree=dynamic_cast<cf::SceneGraph::BspTreeNodeT*>(cf::SceneGraph::GenericNodeT::CreateFromFile_cw(InFile, Pool,
            LightMapMan, SHLMapMan, PlantDescrMan, ShTe_SceneGr, ModelMan));

        if (GE->BspTree==NULL)
        {
            delete GE;
            throw LoadErrorT("Could not read scene graph bsp tree node for game entity!");
        }

        // Read the ClipSys collision model.
        bool HasCollisionModel=false;
        assert(sizeof(HasCollisionModel)==1);
        InFile.read((char*)&HasCollisionModel, sizeof(HasCollisionModel));

        GE->CollModel=HasCollisionModel ? new cf::ClipSys::CollisionModelStaticT(InFile, Pool, ShTe_CollDet) : NULL;

        // Read the origin.
        InFile.read((char*)&GE->Origin.x, sizeof(GE->Origin.x));
        InFile.read((char*)&GE->Origin.y, sizeof(GE->Origin.y));
        InFile.read((char*)&GE->Origin.z, sizeof(GE->Origin.z));

        // Read the property pairs.
        for (unsigned long NrOfPropertyPairs=cf::SceneGraph::aux::ReadUInt32(InFile); NrOfPropertyPairs>0; NrOfPropertyPairs--)
        {
            const std::string Key  =cf::SceneGraph::aux::ReadString(InFile);
            const std::string Value=cf::SceneGraph::aux::ReadString(InFile);

            GE->Properties[Key]=Value;
        }

        // Store it in the list.
        GameEntities.PushBack(GE);
    }

    if (ProgressFunction) ProgressFunction(1.0f, "World file loaded.");
}


void WorldT::SaveToDisk(const char* FileName) const /*throw (SaveErrorT)*/
{
    std::ofstream OutFile(FileName, std::ios::out | std::ios::binary);
    if (OutFile.bad()) throw SaveErrorT("Unable to open world BSP file.");

    cf::SceneGraph::aux::PoolT Pool;

    char           FileHeader[32]="CARSTEN WORLD BSP FILE."; OutFile.write(FileHeader, 32);
    unsigned short FileVersion   =28;                        OutFile.write((char*)&FileVersion, sizeof(FileVersion));


    // 1.3. Write Map Faces, Map LightMaps, Map SHLMaps, and Map FacesDrawIndices.
    OutFile.write((char*)&cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize, sizeof(cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize));

    OutFile.write((char*)&cf::SceneGraph::SHLMapManT::NrOfBands , sizeof(cf::SceneGraph::SHLMapManT::NrOfBands ));
    OutFile.write((char*)&cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize , sizeof(cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize ));
    OutFile.write((char*)&cf::SceneGraph::SHLMapManT::NrOfRepres, sizeof(cf::SceneGraph::SHLMapManT::NrOfRepres));


    // Write the shared terrain data.
    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(Terrains.Size()));
    for (unsigned long TerrainNr=0; TerrainNr<Terrains.Size(); TerrainNr++)
        Terrains[TerrainNr]->WriteTo(OutFile);

    // Write the SceneGraph BSP tree.
    BspTree->WriteTo(OutFile, Pool);

    // Write the ClipSys collision model.
    CollModel->SaveToFile(OutFile, Pool);


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
    cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(GameEntities.Size()));
    for (unsigned long GENr=0; GENr<GameEntities.Size(); GENr++)
    {
        const GameEntityT* GE=GameEntities[GENr];

        // Write the how many-th entity in the map file this game entity corresponds to.
        cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(GE->MFIndex));

        // Write the shared terrain data.
        cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(GE->Terrains.Size()));
        for (unsigned long TerrainNr=0; TerrainNr<GE->Terrains.Size(); TerrainNr++)
            GE->Terrains[TerrainNr]->WriteTo(OutFile);

        // Write the SceneGraph BSP tree.
        GE->BspTree->WriteTo(OutFile, Pool);

        // Write the ClipSys collision model.
        const bool HasCollisionModel=(GE->CollModel!=NULL);
        assert(sizeof(HasCollisionModel)==1);
        OutFile.write((char*)&HasCollisionModel, sizeof(HasCollisionModel));
        if (GE->CollModel) GE->CollModel->SaveToFile(OutFile, Pool);

        // Write the origin.
        OutFile.write((char*)&GE->Origin.x, sizeof(GE->Origin.x));
        OutFile.write((char*)&GE->Origin.y, sizeof(GE->Origin.y));
        OutFile.write((char*)&GE->Origin.z, sizeof(GE->Origin.z));

        // Write the property pairs.
        cf::SceneGraph::aux::Write(OutFile, cf::SceneGraph::aux::cnc_ui32(GE->Properties.size()));
        for (std::map<std::string, std::string>::const_iterator It=GE->Properties.begin(); It!=GE->Properties.end(); ++It)
        {
            cf::SceneGraph::aux::Write(OutFile, It->first );
            cf::SceneGraph::aux::Write(OutFile, It->second);
        }
    }
}
