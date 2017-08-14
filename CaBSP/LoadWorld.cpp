/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include <iostream>
#include "ClipSys/CollisionModel_static.hpp"
#include "ConsoleCommands/ConsoleWarningsOnly.hpp"
#include "TextParser/TextParser.hpp"
#include "SceneGraph/BezierPatchNode.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/TerrainNode.hpp"
#include "SceneGraph/PlantNode.hpp"
#include "SceneGraph/ModelNode.hpp"
#include "MapFile.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"
#include "Models/ModelManager.hpp"
#include "Plants/PlantDescrMan.hpp"
#include "String.hpp"


using namespace cf;


void MapFileSanityCheck(const ArrayT<MapFileEntityT>& MFEntityList)
{
    if (MFEntityList.Size() == 0)
        Error("There are no entities in the map file.");

    const MapFileEntityT& RootEnt = MFEntityList[0];

    if (RootEnt.MFBrushes.Size() < 4)
    {
        Console->Print(cf::va("This map only has %lu brush%s, but CSG requires at least 4 (better 6).\n", RootEnt.MFBrushes.Size(), RootEnt.MFBrushes.Size()==1 ? "" : "es"));
        Console->Print("That means that the minimum of geometry you have to provide is a small room,\n");
        Console->Print("consisting of 4 walls, the floor, and the ceiling.\n");
        Console->Print("That in turn means that you need at least 6 brushes to create a \"closed room\".\n");
        Console->Print("(The minimum is 4 brushes for a \"closed pyramid\".  :-)\n");

        Error("Too few brushes.");
    }
}


// Nimmt ein Token, das aus drei durch Leerzeichen voneinander getrennten Zahlen besteht und gibt sie als VectorT zurück.
VectorT GetVectorFromTripleToken(const std::string& TripleToken)
{
    VectorT V;
    std::istringstream iss(TripleToken);

    iss >> V.x >> V.y >> V.z;   // Bank, Heading and Pitch.

    return V;
}


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


// Liest ein MapFile, das die der Version entsprechenden "MapFile Specifications" erfüllen muß, in die World ein.
void LoadWorld(const char* LoadName, const std::string& GameDirectory, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes,
               WorldT& World, ArrayT<Vector3dT>& FloodFillSources, ArrayT<Vector3dT>& DrawWorldOutsidePointSamples, unsigned int& NumPlayerPrototypes)
{
    World.PlantDescrMan.SetModDir(GameDirectory);

    Console->Print(cf::va("\n*** Load World %s ***\n", LoadName));

    cf::UniScriptStateT                ScriptState;
    IntrusivePtrT<cf::GameSys::WorldT> ScriptWorld;

    cf::GameSys::WorldT::InitScriptState(ScriptState);

    try
    {
        ScriptWorld = new cf::GameSys::WorldT(
            cf::GameSys::WorldT::RealmOther,
            ScriptState,
            ModelMan,
            GuiRes,
            *cf::ClipSys::CollModelMan,   // TODO: The CollModelMan should not be a global, but rather be instantiated along with the ModelMan and GuiRes.
            NULL,       // No clip world for this instance.
            NULL);      // No physics world for this instance.

        ScriptWorld->LoadScript(cf::String::StripExt(LoadName) + ".cent", cf::GameSys::WorldT::InitFlag_OnlyStatic);
    }
    catch (const cf::GameSys::WorldT::InitErrorT& IE)
    {
        Error(IE.what());
    }


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


    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllScriptEnts;
    ScriptWorld->GetRootEntity()->GetAll(AllScriptEnts);

    if (AllScriptEnts.Size() > MFEntityList.Size())
        Console->Print("Note: There are more entities in the .cent file than in the .cmap file.\n"
                       "This is a bit unusual, but normally not a problem.");

    if (AllScriptEnts.Size() < MFEntityList.Size())
        Console->Warning("There are fewer entities in the .cent file than in the .cmap file.\n"
                         "Let's try to proceed, but something may not be right that will cause a more serious problem later.");

    // Move map primitives of "static" entities into the "worldspawn" entity.
    for (unsigned long EntNr = 1; EntNr < AllScriptEnts.Size() && EntNr < MFEntityList.Size(); EntNr++)
    {
        if (AllScriptEnts[EntNr]->GetBasics()->IsStatic())
        {
            MapFileEntityT& E = MFEntityList[EntNr];

            // Move all brushes of this entity into the 'worldspawn' entity.
            MFEntityList[0].MFBrushes.PushBack(E.MFBrushes);
            E.MFBrushes.Clear();

            // Move all bezier patches of this entity into the 'worldspawn' entity.
            MFEntityList[0].MFPatches.PushBack(E.MFPatches);
            E.MFPatches.Clear();

            // TODO:
            // Should other types of primitives (terrains, plants, models) be moved as well??

            // Note that these days we no longer delete the now empty `E`.
            // Doing so would bring AllScriptEnts and MFEntityList out of sync,
            // and be unexpected for the user, who may still refer to `E` e.g. in scripts.
        }
    }

    // Perform certain sanity checks on the map (MFEntityList), look into the function for details.
    MapFileSanityCheck(MFEntityList);

    for (unsigned long EntNr = 0; EntNr < AllScriptEnts.Size() && EntNr < MFEntityList.Size(); EntNr++)
    {
        // Collect the FloodFillSources in advance to the loop below, because along with
        // everything else, these must be transformed into (the worldspawn's) entity space.
        if (AllScriptEnts[EntNr]->GetComponent("PlayerStart") != NULL)
            FloodFillSources.PushBack(AllScriptEnts[EntNr]->GetTransform()->GetOriginWS().AsVectorOfDouble());

        if (AllScriptEnts[EntNr]->GetComponent("HumanPlayer") != NULL)
            NumPlayerPrototypes++;
    }

    // Move/migrate/insert the map primitives of each entity into the entity's BSP tree.
    for (unsigned long EntNr = 0; EntNr < AllScriptEnts.Size() && EntNr < MFEntityList.Size(); EntNr++)
    {
        assert(World.m_StaticEntityData.Size() == EntNr);

        // This is also checked in the `cf::GameSys::WorldT` ctor, see there for details.
        // It is repeated here as a reminder: entity IDs are used as indices into World.m_StaticEntityData[].
        assert(AllScriptEnts[EntNr]->GetID() == EntNr);

        StaticEntityDataT* GameEnt = new StaticEntityDataT();
        World.m_StaticEntityData.PushBack(GameEnt);

        const MapFileEntityT& E = MFEntityList[EntNr];

        float LightMapPatchSize = 8.0f;
        float SHLMapPatchSize   = 8.0f;

        {
            std::map<std::string, std::string>::const_iterator It;

            It = E.MFProperties.find("lightmap_patchsize");
            if (It != E.MFProperties.end())
            {
                LightMapPatchSize = float(atof(It->second.c_str()));

                if (LightMapPatchSize <   2.0f) { LightMapPatchSize =   2.0f; Console->Print("NOTE: LightMap PatchSize clamped to 2.\n"  ); }
                if (LightMapPatchSize > 128.0f) { LightMapPatchSize = 128.0f; Console->Print("NOTE: LightMap PatchSize clamped to 128.\n"); }
            }

            It = E.MFProperties.find("shlmap_patchsize");
            if (It != E.MFProperties.end())
            {
                SHLMapPatchSize = float(atof(It->second.c_str()));

                if (SHLMapPatchSize <   2.0f) { SHLMapPatchSize =   2.0f; Console->Print("NOTE: SHLMap PatchSize clamped to 2.\n"  ); }
                if (SHLMapPatchSize > 128.0f) { SHLMapPatchSize = 128.0f; Console->Print("NOTE: SHLMap PatchSize clamped to 128.\n"); }
            }
        }


        // 1. Copy the properties.
        // GameEnt->m_Properties = E.MFProperties;      // Properties are obsolete and not needed no more.

        // Move all map primitives in this entity from world space into the local entity space.
        //
        // TODO: MapFileTerrainTs are, at this time, "incompatible" with rotation:
        // their dimensions are specified with a bounding box. That only really makes sense without rotation,
        // and we should probably switch to a origin, x- and y-axis representation first...
        bool          InvResult  = true;
        const MatrixT WorldToEnt = AllScriptEnts[EntNr]->GetTransform()->GetEntityToWorld().GetInverse(&InvResult);

        if (!InvResult)
            Console->Warning(cf::va("Entity %lu: Cannot transform from world space to entity space!\n", EntNr));

        if (WorldToEnt.IsEqual(MatrixT(), 0.001f))
        {
            // If WorldToEnt is (close to) the identity anyway, skip the Transform() call below, both as
            // a performance optimization but mostly in order to not negatively impact numerical precision.
            InvResult = false;
        }

        if (InvResult)
        {
            MFEntityList[EntNr].Transform(WorldToEnt);

            if (EntNr == 0)
            {
                // Along with everything else, these must be transformed into (the worldspawn's) entity space.
                for (unsigned int i = 0; i < FloodFillSources.Size(); i++)
                    FloodFillSources[i] = WorldToEnt.Mul1(FloodFillSources[i]);
            }
        }

        // 3. Fill-in the Terrains array.
        for (unsigned long TerrainNr = 0; TerrainNr < E.MFTerrains.Size(); TerrainNr++)
        {
            const MapFileTerrainT& Terrain = E.MFTerrains[TerrainNr];

            GameEnt->m_Terrains.PushBack(new SharedTerrainT(Terrain.Bounds, Terrain.SideLength, Terrain.HeightData, Terrain.Material));
        }

        // 4. Create a new BSP tree.
        GameEnt->m_BspTree = new cf::SceneGraph::BspTreeNodeT(LightMapPatchSize, SHLMapPatchSize);

        // 5. Build the collision model (if this entity has one that is made of map primitives).
        if (EntNr == 0 || E.MFBrushes.Size() > 0 || E.MFPatches.Size() > 0 || GameEnt->m_Terrains.Size() > 0)
        {
            ArrayT<cf::ClipSys::CollisionModelStaticT::TerrainRefT> ShTe;

            for (unsigned long TerrainNr = 0; TerrainNr < GameEnt->m_Terrains.Size(); TerrainNr++)
            {
                const SharedTerrainT* ST = GameEnt->m_Terrains[TerrainNr];

                ShTe.PushBack(cf::ClipSys::CollisionModelStaticT::TerrainRefT(&ST->Terrain, ST->Material, ST->BB));
            }

            // ###
            // ### For now, these consts MUST be kept in sync with those in SceneGraph/BezierPatchNode.cpp !!!
            // ### See there at BezierPatchNodeT::CreatePatchMeshes() for details.
            // ###
            const double COLLISION_MODEL_MAX_CURVE_ERROR  = 24.0;
            const double COLLISION_MODEL_MAX_CURVE_LENGTH = -1.0;
            const double MIN_NODE_SIZE = 40.0;

            // false: Use brushes with precomputed bevel planes (for EntNr == 0).
            // true:  Use generic brushes (for EntNr > 0).
            GameEnt->m_CollModel = new cf::ClipSys::CollisionModelStaticT(E, ShTe, EntNr > 0,
                MapT::RoundEpsilon,
                MapT::MinVertexDist,
                COLLISION_MODEL_MAX_CURVE_ERROR,
                COLLISION_MODEL_MAX_CURVE_LENGTH,
                MIN_NODE_SIZE);
        }

        // 6. Collect the geometry primitives for the BSP tree.
        ArrayT<VectorT> GameEntityOutsidePointSamples;

        for (unsigned long BrushNr = 0; BrushNr < E.MFBrushes.Size(); BrushNr++)
        {
            ComputeBrushFaces(E.MFBrushes[BrushNr], World, GameEnt->m_BspTree, EntNr == 0 ? DrawWorldOutsidePointSamples : GameEntityOutsidePointSamples, EntNr, BrushNr);
        }

        for (unsigned long BPNr = 0; BPNr < E.MFPatches.Size(); BPNr++)
        {
            const MapFileBezierPatchT& BP = E.MFPatches[BPNr];

            GameEnt->m_BspTree->OtherChildren.PushBack(new cf::SceneGraph::BezierPatchNodeT(
                World.LightMapMan, BP.SizeX, BP.SizeY, BP.ControlPoints, BP.SubdivsHorz, BP.SubdivsVert,
                BP.Material, 400.0f));
        }

        for (unsigned long TerrainNr = 0; TerrainNr < E.MFTerrains.Size(); TerrainNr++)
        {
            const MapFileTerrainT& Terrain = E.MFTerrains[TerrainNr];

            // This has already done been done above: GameEnt->Terrains.PushBack(new SharedTerrainT(...));
            GameEnt->m_BspTree->OtherChildren.PushBack(new cf::SceneGraph::TerrainNodeT(Terrain.Bounds, GameEnt->m_Terrains[TerrainNr]->Terrain, TerrainNr, Terrain.Material->Name, LightMapPatchSize));
        }

        for (unsigned long PlantNr = 0; PlantNr < E.MFPlants.Size(); PlantNr++)
        {
            const MapFilePlantT& Plant = E.MFPlants[PlantNr];

            GameEnt->m_BspTree->OtherChildren.PushBack(new cf::SceneGraph::PlantNodeT(World.PlantDescrMan.GetPlantDescription(Plant.DescrFileName) , Plant.RandomSeed, Plant.Position, Plant.Angles));
        }

        for (unsigned long ModelNr = 0; ModelNr < E.MFModels.Size(); ModelNr++)
        {
            std::string          ErrorMsg;
            const MapFileModelT& Model = E.MFModels[ModelNr];
            const CafuModelT*    CafuM = ModelMan.GetModel(GameDirectory+"/"+Model.Model, &ErrorMsg);

            if (ErrorMsg!="") Console->Warning(ErrorMsg);
            GameEnt->m_BspTree->OtherChildren.PushBack(new cf::SceneGraph::ModelNodeT(CafuM, Model.Label, Model.Origin, Model.Angles, Model.Scale, Model.SeqNumber, Model.FrameOffset, Model.FrameTimeScale, Model.Animate));
        }

        // 7. Compute the BSP tree.
        // For non-world entities, this is done immediately here rather than after the map has been loaded
        // completely, so that we don't have to keep the OutsidePointSamples array.
        if (EntNr > 0)
        {
            BspTreeBuilderT BspTreeBuilder(GameEnt->m_BspTree, false /*most simple tree*/, false /*Bsp split faces*/, true /*chop up faces*/);

            ArrayT<Vector3dT> EmptyFloodFillSources;
            std::string       EmptyMapFileName = "";  // Entity BSP trees aren't flood-filled, so they cannot leak, so we never need to write a .pts point file for them.

            // Temporarily filter the console output by redirecting everything through the warnings-only console.
            cf::ConsoleWarningsOnlyT ConWarnOnly(Console);
            cf::ConsoleI* PrevConsole = Console;
            Console = &ConWarnOnly;

            BspTreeBuilder.Build(false /*IsWorldspawn?*/, EmptyFloodFillSources, GameEntityOutsidePointSamples, EmptyMapFileName);

            // Restore the previous console.
            Console = PrevConsole;
        }
    }


    Console->Print("All game entities done, processing the worldspawn entity now.\n");

    Console->Print(cf::va("Face Children  : %10lu    Draw World Outer Point Samples: %5lu\n", World.m_StaticEntityData[0]->m_BspTree->FaceChildren.Size(), DrawWorldOutsidePointSamples.Size()));
    Console->Print(cf::va("PlayerStarts   : %10lu\n", FloodFillSources.Size()));
    Console->Print(cf::va("Other Children : %10lu\n", World.m_StaticEntityData[0]->m_BspTree->OtherChildren.Size()));
    Console->Print(cf::va("Entities       : %10lu\n", AllScriptEnts.Size()));
}
