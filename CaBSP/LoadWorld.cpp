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

/******************/
/*** Load World ***/
/******************/

#include <iostream>
#include "ClipSys/CollisionModel_static.hpp"
#include "ConsoleCommands/ConsoleWarningsOnly.hpp"
#include "TextParser/TextParser.hpp"
#include "SceneGraph/BezierPatchNode.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "SceneGraph/TerrainNode.hpp"
#include "SceneGraph/PlantNode.hpp"
#include "SceneGraph/ModelNode.hpp"
#include "MapFile.hpp"
#include "Plants/PlantDescrMan.hpp"


using namespace cf;


void MapFileSanityCheck(const ArrayT<MapFileEntityT>& MFEntityList)
{
    unsigned long NrOfWorldSpawnEntities     =0;
    unsigned long NrOfInfoPlayerStartEntities=0;

    for (unsigned long EntityNr=0; EntityNr<MFEntityList.Size(); EntityNr++)
    {
        const MapFileEntityT& E=MFEntityList[EntityNr];
        const std::map<std::string, std::string>::const_iterator It=E.MFProperties.find("classname");

        if (It==E.MFProperties.end()) Error("\"classname\" property not found in entity %lu.", EntityNr);

        // If we found the 'classname worldspawn' property pair, we count the occurence, because there must only be exactly one such entity.
        // Additionally, our map file spec. implies that there MUST be a 'mapfile_version XX' pair following the 'classname worldspawn' pair.
        if (It->second=="worldspawn")
        {
            if (EntityNr!=0) Error("Entity %u is the 'worldspawn' entity, but entity 0 is expected to be the only 'worldspawn' entity.", EntityNr);
            if (E.MFProperties.size()==1) Error("Entity %u is the worldspawn entity, but has only one PPair (missing mapfile_version).", EntityNr);

            // These two issues are directly checked while loading.
            // The pair also must not be precisely at second position (index 1) anymore.
            // if (E.MFPropPairs[1].Key!="mapfile_version") Error("Entity %u: Second PPair key is not 'mapfile_version'.", EntityNr);
            // if (atoi(E.MFPropPairs[1].Value.c_str())!=9)) Error("Bad map file version: Expected %s, got %s.", "9", E.MFPropPairs[1].Value.c_str());

            if (E.MFBrushes.Size()<4)
            {
                Console->Print(cf::va("This map only has %lu brush%s, but CSG requires at least 4 (better 6).\n", E.MFBrushes.Size(), E.MFBrushes.Size()==1 ? "" : "es"));
                Console->Print("That means that the minimum of geometry you have to provide is a small room,\n");
                Console->Print("consisting of 4 walls, the floor, and the ceiling.\n");
                Console->Print("That in turn means that you need at least 6 brushes to create a \"closed room\".\n");
                Console->Print("(The minimum is 4 brushes for a \"closed pyramid\".  :-)\n");
                Error("Too few brushes.");
            }

            NrOfWorldSpawnEntities++;
        }

        // If this is a 'classname info_player_start' entity, we count the occurence, because we insist that there is at least one such entity.
        if (It->second=="info_player_start")
            NrOfInfoPlayerStartEntities++;

        // No further checks should be necessary for now. We can always ignore invalid property pairs and/or fill in default values.
    }

    if (NrOfWorldSpawnEntities     !=1) Error("Found %u worldspawn entities, expected exactly 1.", NrOfWorldSpawnEntities);
    if (NrOfInfoPlayerStartEntities==0) Error("Found no info_player_start entities at all, expected at least 1.", NrOfInfoPlayerStartEntities);
}


// Nimmt ein Token, das aus drei durch Leerzeichen voneinander getrennten Zahlen besteht und gibt sie als VectorT zurück.
VectorT GetVectorFromTripleToken(const std::string& TripleToken)
{
    VectorT V;
    std::istringstream iss(TripleToken);

    iss >> V.x >> V.y >> V.z;   // Bank, Heading and Pitch.

    return V;
}


/* OBSOLETE - CaWE saves the cmap files now immediately right.
// Nimmt ein Token, das aus drei durch Leerzeichen voneinander getrennten Zahlen besteht, die als Winkel interpretiert werden,
// und gibt die dazugehörige Richtung als VectorT zurück.
VectorT GetDirFromTripleAngleToken(const char* TripleToken)
{
    VectorT V;
    std::istringstream iss(TripleToken);

    iss >> V.x >> V.y >> V.z;

    VectorT D(0.0, 1.0, 0.0);

    // Sigh. All this angle related stuff *REALLY* must be checked:
    // Which angles rotates about which axis, and in what order?
    // Also wrt. the BBs of static detail models!
    D=D.GetRotX(V.x);
    D=D.GetRotZ(V.y);

    return normalize(D, 0.0);
} */


// Computes the brush polygons from a MapFileBrushT.
void ComputeBrushPolys(const MapFileBrushT& MFBrush, ArrayT< Polygon3T<double> >& BrushPolys, const unsigned long EntityNr, const unsigned long BrushNr)
{
    // Konstruiere die Polygone des Brushes.
    for (unsigned long MFPlaneNr=0; MFPlaneNr<MFBrush.MFPlanes.Size(); MFPlaneNr++)
    {
        BrushPolys.PushBackEmpty();
        BrushPolys[BrushPolys.Size()-1].Plane=MFBrush.MFPlanes[MFPlaneNr].Plane;
    }


    Polygon3T<double>::Complete(BrushPolys, MapT::RoundEpsilon);


    // Prüfe die Gültigkeit der konstruierten Polygone.
    // Eine explizite Gültigkeitsprüfung ist sinnvoll und notwendig um sicherzustellen, daß wir mit "sauberen" Eingabedaten anfangen!
    for (unsigned long MFPlaneNr=0; MFPlaneNr<BrushPolys.Size(); MFPlaneNr++)
        if (!BrushPolys[MFPlaneNr].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist))
        {
            Console->Print("\n");
            for (unsigned long PlaneNr=0; PlaneNr<MFBrush.MFPlanes.Size(); PlaneNr++)
                std::cout << convertToString(MFBrush.MFPlanes[PlaneNr].Plane) << " "
                          << MFBrush.MFPlanes[PlaneNr].Material->Name << "\n";

            Console->Print("\n");
            for (unsigned long PlaneNr=0; PlaneNr<BrushPolys.Size(); PlaneNr++)
            {
                std::cout << "Plane " << convertToString(BrushPolys[PlaneNr].Plane) << ", Vertices ";

                for (unsigned long VertexNr=0; VertexNr<BrushPolys[PlaneNr].Vertices.Size(); VertexNr++)
                    std::cout << convertToString(BrushPolys[PlaneNr].Vertices[VertexNr]) << " ";

                std::cout << "\n";
            }

            Error("Entity #%u, brush #%u: polygon #%u is invalid.", EntityNr, BrushNr, MFPlaneNr);
        }
}


void ComputeBrushFaces(const MapFileBrushT& MFBrush, WorldT& World, cf::SceneGraph::BspTreeNodeT* BspTree,
                       ArrayT<VectorT>& DrawWorldOutsidePointSamples, const unsigned long EntityNr, const unsigned long BrushNr)
{
    // Compute the overall clip flags (combined from the faces) of the brush.
    unsigned long CombinedClipFlags_Ored =0;
    unsigned long CombinedClipFlags_Anded=0xFFFFFFFF;

    for (unsigned long MFPlaneNr=0; MFPlaneNr<MFBrush.MFPlanes.Size(); MFPlaneNr++)
    {
        const MapFilePlaneT& MFPlane=MFBrush.MFPlanes[MFPlaneNr];

        CombinedClipFlags_Ored |=MFPlane.Material->ClipFlags;
        CombinedClipFlags_Anded&=MFPlane.Material->ClipFlags;

        // Print a warning if a materials stops portals, but not players and monsters.
        if ((MFPlane.Material->ClipFlags & MaterialT::Clip_BspPortals)!=0 &&
            !((MFPlane.Material->ClipFlags & MaterialT::Clip_Players)!=0 && (MFPlane.Material->ClipFlags & MaterialT::Clip_Monsters)!=0))
            Console->Warning("Material "+MFPlane.Material->Name+" stops portals, but not players and monsters!\n");
    }

    // The materials of all faces should have the same value for the MaterialT::Clip_BspPortals flag - all on or all off.
    // Having mixed flags is not really a problem, but can lead to some "unusual" results later during Portalization,
    // e.g. when a cube that is floating in mid-air has 5 sides with and one side without the bspPortals flag set.
    if ((CombinedClipFlags_Ored & MaterialT::Clip_BspPortals)!=(CombinedClipFlags_Anded & MaterialT::Clip_BspPortals))
        Console->Warning(cf::va("Entity %lu, brush %lu: Faces have materials with mismatching \"bspPortal\" clip flags.\n", EntityNr, BrushNr));


    // Compute the brush polygons.
    ArrayT< Polygon3T<double> > BrushPolys;
    ComputeBrushPolys(MFBrush, BrushPolys, EntityNr, BrushNr);


    // Compute the center of the brush, to be used for leak detection later.
    // The algorithm is probably not 100% correct, in the sense that the computed center
    // might be different from mathematically correct center, but that's negligible.
    VectorT       BrushCenter;
    unsigned long AverageCount=0;

    for (unsigned long MFPlaneNr=0; MFPlaneNr<BrushPolys.Size(); MFPlaneNr++)
        for (unsigned long VertexNr=0; VertexNr<BrushPolys[MFPlaneNr].Vertices.Size(); VertexNr++)
        {
            BrushCenter+=BrushPolys[MFPlaneNr].Vertices[VertexNr];
            AverageCount++;
        }

    BrushCenter=scale(BrushCenter, 1.0/double(AverageCount));

    // Include a small sanity check to make sure that the 'BrushCenter' is really inside the brush (think of rounding errors...).
    for (unsigned long MFPlaneNr=0; MFPlaneNr<BrushPolys.Size(); MFPlaneNr++)
        if (BrushPolys[MFPlaneNr].Plane.GetDistance(BrushCenter) > -MapT::RoundEpsilon)
            Error("Entity #%u, brush #%u: BrushCenter is outside brush.", EntityNr, BrushNr);

    // If all faces of the brush are solid for BSP portals (unlike e.g. glass, there cannot be a portal that sees into the brush),
    // consider the inside of the brush as "solid" and "outside of the visible world", and thus collect an outside point sample.
    if (CombinedClipFlags_Anded & MaterialT::Clip_BspPortals) DrawWorldOutsidePointSamples.PushBack(BrushCenter);


    for (unsigned long MFPlaneNr=0; MFPlaneNr<MFBrush.MFPlanes.Size(); MFPlaneNr++)
    {
        const MapFilePlaneT& MFPlane=MFBrush.MFPlanes[MFPlaneNr];

        if (((MFPlane.Material->AmbientShaderName=="none" && MFPlane.Material->LightShaderName=="none") || MFPlane.Material->NoDraw) &&
            (MFPlane.Material->ClipFlags & MaterialT::Clip_BspPortals)==0)
        {
            // A face enters the draw BSP tree only if it is visible (like walls, glass, water) or stops the BSP flood-fill
            // (like invisible "caulk" materials), or both (like most of the normal walls).
            // Faces with materials that don't draw at all and are not "caulk" (invisible materials that nontheless stop portals)
            // can be omitted from the BSP tree. As the BSP tree is only used for drawing, this prevents unnecessary leaves and thus complexity.
            continue;
        }

        cf::SceneGraph::FaceNodeT::TexInfoT TI;

        TI.U      =MFPlane.U.AsVectorOfFloat();
        TI.V      =MFPlane.V.AsVectorOfFloat();
        TI.OffsetU=float(MFPlane.ShiftU);
        TI.OffsetV=float(MFPlane.ShiftV);

        BspTree->FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(World.LightMapMan, World.SHLMapMan, BrushPolys[MFPlaneNr], MFPlane.Material, TI));
    }
}


// Ließt ein MapFile, das die der Version entsprechenden "MapFile Specifications" erfüllen muß, in die World ein.
// Dabei werden folgende Komponenten der World modifiziert (ausgefüllt, u.U. nur teilweise):
// Map.Faces, Map.TexInfos, Map.PointLights, InfoPlayerStarts und GameEntities.
void LoadWorld(const char* LoadName, const std::string& GameDirectory, WorldT& World, ArrayT<VectorT>& DrawWorldOutsidePointSamples)
{
    World.PlantDescrMan.SetModDir(GameDirectory);

    Console->Print(cf::va("\n*** Load World %s ***\n", LoadName));


    // Parse all map entities into the MFEntityList.
    ArrayT<MapFileEntityT> MFEntityList;
    TextParserT            TP(LoadName, "({})");

    try
    {
        MapFileReadHeader(TP);

        while (!TP.IsAtEOF())
        {
            MFEntityList.PushBack(MapFileEntityT(MFEntityList.Size(), TP));
        }
    }
    catch (const TextParserT::ParseError&)
    {
        Error("Problem with parsing the map near byte %lu (%.3f%%) of the file.", TP.GetReadPosByte(), TP.GetReadPosPercent()*100.0);
    }


    // Perform certain sanity checks on the map (MFEntityList), look into the function for details.
    MapFileSanityCheck(MFEntityList);


    // 'func_group' entities are just for editor convenience, thus toss all their brushes into the 'worldspawn' entity.
    // 'func_wall' and 'func_water' entities exist for historic reasons (render walls specially, treat water specially),
    //     but are largely obsolete now with the Material System and Clip System and should probably be removed from
    //     the maps and the EntityClassDefs.lua files.
    // Assumptions:
    // 1.) Entity 0 is the 'worldspawn' entity.
    // 2.) Each entity has the "classname" property.
    for (unsigned long EntityNr=1; EntityNr<MFEntityList.Size(); EntityNr++)
    {
        const std::string EntClassName=MFEntityList[EntityNr].MFProperties["classname"];

        if (EntClassName=="func_group" || EntClassName=="func_wall" || EntClassName=="func_water")
        {
            // Copy all brushes of this entity into the 'worldspawn' entity.
            for (unsigned long BrushNr=0; BrushNr<MFEntityList[EntityNr].MFBrushes.Size(); BrushNr++)
                MFEntityList[0].MFBrushes.PushBack(MFEntityList[EntityNr].MFBrushes[BrushNr]);

            // Copy all bezier patches of this entity into the 'worldspawn' entity.
            for (unsigned long BPNr=0; BPNr<MFEntityList[EntityNr].MFPatches.Size(); BPNr++)
                MFEntityList[0].MFPatches.PushBack(MFEntityList[EntityNr].MFPatches[BPNr]);

            // Delete this entity.
            MFEntityList[EntityNr]=MFEntityList[MFEntityList.Size()-1];
            MFEntityList.DeleteBack();
            EntityNr--;
        }
    }


    // *** HACK HACK HACK HACK ***   (In fact, two hacks total.)
    //
    // In TechDemo.cmap, "light" entities sometimes have Bezier Patches (and brushes?).
    // However, Cafu can currently not render them as such, thus let's move them into the "worldspawn" entity.
    for (unsigned long EntityNr=1; EntityNr<MFEntityList.Size(); EntityNr++)
    {
        MapFileEntityT& E=MFEntityList[EntityNr];

        if (E.MFProperties["classname"]=="light")
        {
            // Move all brushes of this entity into the 'worldspawn' entity.
            for (unsigned long BrushNr=0; BrushNr<E.MFBrushes.Size(); BrushNr++)
                MFEntityList[0].MFBrushes.PushBack(E.MFBrushes[BrushNr]);
            E.MFBrushes.Clear();

            // Move all bezier patches of this entity into the 'worldspawn' entity.
            for (unsigned long BPNr=0; BPNr<E.MFPatches.Size(); BPNr++)
                MFEntityList[0].MFPatches.PushBack(E.MFPatches[BPNr]);
            E.MFPatches.Clear();

#if 1
            // Delete this entity.
            MFEntityList[EntityNr]=MFEntityList[MFEntityList.Size()-1];
            MFEntityList.DeleteBack();
            EntityNr--;
#else
            // Now the 2nd part of the hack: Convert the "light" entity into a "PointLightSource" entity,
            // it will be included with the other regular game entities below.
            // NOTE: "PointLight" and "PointLightSource" are NOT THE SAME!
            bool HaveColor =false;
            bool HaveRadius=false;

            for (unsigned long PPairNr=1; PPairNr<E.MFPropPairs.Size(); PPairNr++)
                     if (E.MFPropPairs[PPairNr].Key=="_color") HaveColor=true;
                else if (E.MFPropPairs[PPairNr].Key=="light_radius") HaveRadius=true;

            if (!HaveColor || !HaveRadius) continue;

            // Okay, it's a point light source.
            E.MFPropPairs[0].Value="PointLightSource";

            for (unsigned long PPairNr=1; PPairNr<E.MFPropPairs.Size(); PPairNr++)
            {
                if (E.MFPropPairs[PPairNr].Key=="_color")
                {
                    const Vector3dT LightColor=GetVectorFromTripleToken(E.MFPropPairs[PPairNr].Value);
                    char            str[256];

                    sprintf(str, "%i %i %i", int(LightColor.x*255), int(LightColor.y*255), int(LightColor.z*255));

                    E.MFPropPairs[PPairNr].Key  ="light_color_diff";
                    E.MFPropPairs[PPairNr].Value=str;

                    MapFilePropPairT SpecCol;
                    SpecCol.Key  ="light_color_spec";
                    SpecCol.Value=str;
                    E.MFPropPairs.PushBack(SpecCol);
                }
                else if (E.MFPropPairs[PPairNr].Key=="light_radius")
                {
                    const Vector3dT LightRadius=GetVectorFromTripleToken(E.MFPropPairs[PPairNr].Value);
                    const double    LightRAvg  =(LightRadius.x+LightRadius.y+LightRadius.z)/3.0 * CA3DE_SCALE;
                    char            str[256];

                    sprintf(str, "%i", int(LightRAvg));

                    E.MFPropPairs[PPairNr].Value=str;
                }
                else if (E.MFPropPairs[PPairNr].Key=="light_origin")
                {
                    // Update our "origin" with this value??
                    // E.MFPropPairs[OriginPPairNr].Value=E.MFPropPairs[PPairNr].Value;
                }
                else if (E.MFPropPairs[PPairNr].Key=="light_target" || E.MFPropPairs[PPairNr].Key=="light_up" || E.MFPropPairs[PPairNr].Key=="light_right")
                {
                    Console->Warning("\"light\" entity %lu, mixed point and projected light keys?\n", EntityNr);
                }
            }
#endif
        }
    }


    // Finally, fill-in our World data structures!
    for (unsigned long EntityNr=0; EntityNr<MFEntityList.Size(); EntityNr++)
    {
        const MapFileEntityT& E=MFEntityList[EntityNr];
        const std::map<std::string, std::string>::const_iterator ClassNamePair=E.MFProperties.find("classname");

        if (ClassNamePair==E.MFProperties.end()) Error("\"classname\" property not found in entity %lu.", EntityNr);

        if (ClassNamePair->second=="worldspawn")
        {
            std::map<std::string, std::string>::const_iterator It;

            It=E.MFProperties.find("lightmap_patchsize");
            if (It!=E.MFProperties.end())
            {
                cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize=float(atof(It->second.c_str()));

                if (cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize<  50.0) { cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize=  50.0; Console->Print("NOTE: LightMap PatchSize clamped to 50.\n"  ); }
                if (cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize>2000.0) { cf::SceneGraph::FaceNodeT::LightMapInfoT::PatchSize=2000.0; Console->Print("NOTE: LightMap PatchSize clamped to 2000.\n"); }
            }

            It=E.MFProperties.find("shlmap_patchsize");
            if (It!=E.MFProperties.end())
            {
                cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize=float(atof(It->second.c_str()));

                if (cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize<  50.0) { cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize=  50.0; Console->Print("NOTE: SHLMap PatchSize clamped to 50.\n"  ); }
                if (cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize>2000.0) { cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize=2000.0; Console->Print("NOTE: SHLMap PatchSize clamped to 2000.\n"); }
            }


            for (unsigned long BrushNr=0; BrushNr<E.MFBrushes.Size(); BrushNr++)
                ComputeBrushFaces(E.MFBrushes[BrushNr], World, World.BspTree, DrawWorldOutsidePointSamples, EntityNr, BrushNr);

            for (unsigned long BPNr=0; BPNr<E.MFPatches.Size(); BPNr++)
            {
                const MapFileBezierPatchT& BP=E.MFPatches[BPNr];

                World.BspTree->OtherChildren.PushBack(new cf::SceneGraph::BezierPatchNodeT(World.LightMapMan, BP.SizeX, BP.SizeY, BP.ControlPoints, BP.SubdivsHorz, BP.SubdivsVert, BP.Material));
            }

            for (unsigned long TerrainNr=0; TerrainNr<E.MFTerrains.Size(); TerrainNr++)
            {
                const MapFileTerrainT& Terrain=E.MFTerrains[TerrainNr];

                World.Terrains.PushBack(new SharedTerrainT(Terrain.Bounds, Terrain.SideLength, Terrain.HeightData, Terrain.Material));
                World.BspTree->OtherChildren.PushBack(new cf::SceneGraph::TerrainNodeT(Terrain.Bounds, World.Terrains[TerrainNr]->Terrain, TerrainNr, Terrain.Material->Name));
            }

            for (unsigned long PlantNr=0; PlantNr<E.MFPlants.Size(); PlantNr++)
            {
                const MapFilePlantT& Plant=E.MFPlants[PlantNr];

                World.BspTree->OtherChildren.PushBack(new cf::SceneGraph::PlantNodeT(World.PlantDescrMan.GetPlantDescription(Plant.DescrFileName) , Plant.RandomSeed, Plant.Position, Plant.Angles));
            }

            for (unsigned long ModelNr=0; ModelNr<E.MFModels.Size(); ModelNr++)
            {
                const MapFileModelT& Model=E.MFModels[ModelNr];

                World.BspTree->OtherChildren.PushBack(new cf::SceneGraph::ModelNodeT(GameDirectory+"/"+Model.Model, Model.Label, Model.Origin, Model.Angles, Model.Scale, Model.SeqNumber, Model.FrameOffset, Model.FrameTimeScale, Model.Animate));
            }
        }
        else if (ClassNamePair->second=="PointLight")
        {
            const double Pi=3.14159265358979323846;

            PointLightT PL;
            std::map<std::string, std::string>::const_iterator It;

            PL.Dir  =VectorT(0.0, 1.0, 0.0);
            PL.Angle=float(Pi);

            It=E.MFProperties.find("origin"       ); if (It!=E.MFProperties.end()) PL.Origin     =GetVectorFromTripleToken(It->second)*CA3DE_SCALE;
            It=E.MFProperties.find("angles"       ); if (It!=E.MFProperties.end()) PL.Dir        =Vector3dT(0, 0, -1);   // Sigh... yet another hack! :-/   // GetVectorFromTripleToken(It->second);
            It=E.MFProperties.find("opening_angle"); if (It!=E.MFProperties.end()) PL.Angle      =float(atof(It->second.c_str())/180.0*Pi);
            It=E.MFProperties.find("intensity_r"  ); if (It!=E.MFProperties.end()) PL.Intensity.x=atof(It->second.c_str());
            It=E.MFProperties.find("intensity_g"  ); if (It!=E.MFProperties.end()) PL.Intensity.y=atof(It->second.c_str());
            It=E.MFProperties.find("intensity_b"  ); if (It!=E.MFProperties.end()) PL.Intensity.z=atof(It->second.c_str());

            if (PL.Angle<0.0) PL.Angle=0.0;
            if (PL.Angle> Pi) PL.Angle=float(Pi);

            World.PointLights.PushBack(PL);
        }
     /* else if (ClassNamePair->second=="func_wall")    // Special case, handled above.
        {
        }
        else if (ClassNamePair->second=="func_water")   // Special case, handled above.
        {
        } */
        else if (ClassNamePair->second=="info_player_start")
        {
            InfoPlayerStartT IPS;
            std::map<std::string, std::string>::const_iterator It;

            IPS.Heading=0;
            IPS.Pitch  =0;
            IPS.Bank   =0;

            It=E.MFProperties.find("origin"); if (It!=E.MFProperties.end()) IPS.Origin =GetVectorFromTripleToken(It->second.c_str())*CA3DE_SCALE;
            It=E.MFProperties.find("angles"); if (It!=E.MFProperties.end()) IPS.Heading=(unsigned short)(GetVectorFromTripleToken(It->second).y*8192.0/45.0);

            World.InfoPlayerStarts.PushBack(IPS);
        }
        else
        {
            // Console->Print("INFO: PROCESSING GAME ENTITY "+cf::va("%lu", World.GameEntities.Size())+" (class \""+ClassNamePair->second+"\"):\n");
            // Console->Print("***********************************************************************************\n\n");

            // Es ist ein Game/MOD/DLL-Entity, kein Engine-Entity!
            GameEntityT* GE=new GameEntityT(E.MFIndex);

            // 1. Copy the properties.
            GE->Properties=E.MFProperties;

            // 2. Copy the origin point.
            std::map<std::string, std::string>::const_iterator It=E.MFProperties.find("origin");
            if (It!=E.MFProperties.end())
            {
                // Der Origin ist etwas besonderes, denn er kommt bei allen "point entities" vor
                // (bei "solid entities" idR in Form eines Origin-Brushes),
                // und in der Engine (EntityStateT) sogar bei allen Entities.
                GE->Origin=GetVectorFromTripleToken(It->second)*CA3DE_SCALE;
            }

            // 3. Fill-in the Terrains array.
            for (unsigned long TerrainNr=0; TerrainNr<E.MFTerrains.Size(); TerrainNr++)
            {
                const MapFileTerrainT& Terrain=E.MFTerrains[TerrainNr];

                GE->Terrains.PushBack(new SharedTerrainT(Terrain.Bounds, Terrain.SideLength, Terrain.HeightData, Terrain.Material));
            }

            // 4. Create a new BSP tree.
            GE->BspTree=new cf::SceneGraph::BspTreeNodeT;

            // 5. Build the collision model (if this entity has one that is made of map primitives).
            if (E.MFBrushes.Size()>0 || E.MFPatches.Size()>0 || GE->Terrains.Size()>0)
            {
                ArrayT<cf::ClipSys::CollisionModelStaticT::TerrainRefT> ShTe;

                for (unsigned long TerrainNr=0; TerrainNr<GE->Terrains.Size(); TerrainNr++)
                {
                    const SharedTerrainT* ST=GE->Terrains[TerrainNr];

                    ShTe.PushBack(cf::ClipSys::CollisionModelStaticT::TerrainRefT(&ST->Terrain, ST->Material, ST->BB));
                }

                GE->CollModel=new cf::ClipSys::CollisionModelStaticT(E, ShTe, true /*Use generic brushes.*/);
            }

            // 6. Collect the geometry primitives for the BSP tree.
            ArrayT<VectorT> GameEntityOutsidePointSamples;

            for (unsigned long BrushNr=0; BrushNr<E.MFBrushes.Size(); BrushNr++)
            {
                ComputeBrushFaces(E.MFBrushes[BrushNr], World, GE->BspTree, GameEntityOutsidePointSamples, EntityNr, BrushNr);
            }

            for (unsigned long BPNr=0; BPNr<E.MFPatches.Size(); BPNr++)
            {
                const MapFileBezierPatchT& BP=E.MFPatches[BPNr];

                GE->BspTree->OtherChildren.PushBack(new cf::SceneGraph::BezierPatchNodeT(World.LightMapMan, BP.SizeX, BP.SizeY, BP.ControlPoints, BP.SubdivsHorz, BP.SubdivsVert, BP.Material));
            }

            for (unsigned long TerrainNr=0; TerrainNr<E.MFTerrains.Size(); TerrainNr++)
            {
                const MapFileTerrainT& Terrain=E.MFTerrains[TerrainNr];

                // This has already done been done above: GE->Terrains.PushBack(new SharedTerrainT(...));
                GE->BspTree->OtherChildren.PushBack(new cf::SceneGraph::TerrainNodeT(Terrain.Bounds, GE->Terrains[TerrainNr]->Terrain, TerrainNr, Terrain.Material->Name));
            }

            for (unsigned long PlantNr=0; PlantNr<E.MFPlants.Size(); PlantNr++)
            {
                const MapFilePlantT& Plant=E.MFPlants[PlantNr];

                GE->BspTree->OtherChildren.PushBack(new cf::SceneGraph::PlantNodeT(World.PlantDescrMan.GetPlantDescription(Plant.DescrFileName) , Plant.RandomSeed, Plant.Position, Plant.Angles));
            }

            for (unsigned long ModelNr=0; ModelNr<E.MFModels.Size(); ModelNr++)
            {
                const MapFileModelT& Model=E.MFModels[ModelNr];

                GE->BspTree->OtherChildren.PushBack(new cf::SceneGraph::ModelNodeT(Model.Model, Model.Label, Model.Origin, Model.Angles, Model.Scale, Model.SeqNumber, Model.FrameOffset, Model.FrameTimeScale, Model.Animate));
            }


            // 7. Compute the BSP tree.
            // It's done immediately here rather than after the map has been loaded completely
            // so that we don't have to keep the OutsidePointSamples array.
            BspTreeBuilderT BspTreeBuilder(GE->BspTree, false /*most simple tree*/, false /*min face splits*/);

            ArrayT<Vector3dT> EmptyFloodFillSources;
            std::string       EmptyMapFileName="";  // Entity BSP trees aren't flood-filled, so they cannot leak, so we never need to write a .pts point file for them.

            // Temporarily filter the console output by redirecting everything through the warnings-only console.
            cf::ConsoleWarningsOnlyT ConWarnOnly(Console);
            cf::ConsoleI* PrevConsole=Console;
            Console=&ConWarnOnly;

            BspTreeBuilder.Build(ClassNamePair->second=="worldspawn", EmptyFloodFillSources, GameEntityOutsidePointSamples, EmptyMapFileName);

            // Restore the previous console.
            Console=PrevConsole;

            // 8. Add the entity to the worlds list.
            World.GameEntities.PushBack(GE);
        }
    }


    Console->Print("All game entities done, processing the worldspawn entity now.\n");
    Console->Print(std::string("Preparing worldspawn clip data (")+GetTimeSinceProgramStart()+")...");

    ArrayT<cf::ClipSys::CollisionModelStaticT::TerrainRefT> ShTe;

    for (unsigned long TerrainNr=0; TerrainNr<World.Terrains.Size(); TerrainNr++)
    {
        const SharedTerrainT* ST=World.Terrains[TerrainNr];

        ShTe.PushBack(cf::ClipSys::CollisionModelStaticT::TerrainRefT(&ST->Terrain, ST->Material, ST->BB));
    }

    World.CollModel=new cf::ClipSys::CollisionModelStaticT(MFEntityList[0], ShTe, false /*Use brushes with precomputed bevel planes.*/);
    Console->Print(std::string(" done (")+GetTimeSinceProgramStart()+").\n");

    Console->Print(cf::va("Face Children    : %10lu    Draw World Outer Point Samples: %5lu\n", World.BspTree->FaceChildren.Size(), DrawWorldOutsidePointSamples.Size()));
    Console->Print(cf::va("PointLights      : %10lu\n", World.PointLights.Size()));
    Console->Print(cf::va("InfoPlayerStarts : %10lu\n", World.InfoPlayerStarts.Size()));
    Console->Print(cf::va("Other Children   : %10lu\n", World.BspTree->OtherChildren.Size()));
    Console->Print(cf::va("GameEntities     : %10lu\n", World.GameEntities.Size()));
}
