/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CollisionModelMan_impl.hpp"
#include "CollisionModel_static.hpp"

#include "MapFile.hpp"
#include "ConsoleCommands/Console.hpp"
#include "TextParser/TextParser.hpp"
#include "String.hpp"
#include "../Common/World.hpp"      // Needed (only) for some MapT::xy stuff.


using namespace cf::ClipSys;


CollModelManImplT::CollModelManImplT()
{
}


CollModelManImplT::~CollModelManImplT()
{
    // Make sure that the user code properly freed all the collision models.
    assert(cmInfos.Size() == 0);
}


const CollisionModelT* CollModelManImplT::GetCM(const std::string& FileName)
{
    // First see if we already have that file in stock.
    // If so, we can return the instance and just increase the reference count.
    for (unsigned long cmiNr = 0; cmiNr < cmInfos.Size(); cmiNr++)
    {
        cmInfoT& cmi = cmInfos[cmiNr];

        if (cmi.FileName == "") continue;
        if (cmi.FileName == FileName)
        {
            cmi.RefCount++;
            return cmi.Instance;
        }
    }

    // Okay, we have no such file yet.
    cmInfoT cmi;

    cmi.Instance = NULL;
    cmi.FileName = FileName;
    cmi.RefCount = 1;
    cmi.NoDelete = false;

    if (cf::String::EndsWith(FileName, ".cmap"))
    {
        TextParserT TP(FileName.c_str(), "({})");

        if (TP.IsAtEOF())
        {
            Console->Warning("Could not open collision model file \"" + FileName + "\".\n");
            return NULL;
        }

        try
        {
            MapFileReadHeader(TP);
            MapFileEntityT Worldspawn(0, TP);

            // Move the map primitives of all other entities into the "worldspawn" entity.
            // This code is analogous to that in `CaBSP/LoadWorld.cpp`.
            while (!TP.IsAtEOF())
            {
                const MapFileEntityT E(0, TP);

                // Should other types of primitives (terrains, plants, models) be moved as well?
                Worldspawn.MFBrushes.PushBack(E.MFBrushes);
                Worldspawn.MFPatches.PushBack(E.MFPatches);
            }

            // TODO: Account for the first map file entity to come with terrains! (Must keep the terrain instance in cmInfoT.)
            const ArrayT<CollisionModelStaticT::TerrainRefT> ShTe;

            const double COLLISION_MODEL_MAX_CURVE_ERROR  = 24.0;
            const double COLLISION_MODEL_MAX_CURVE_LENGTH = -1.0;
            const double COLLISION_MODEL_MIN_NODE_SIZE    = 40.0;

            CollisionModelStaticT* CM = new CollisionModelStaticT(Worldspawn, ShTe, true /*Use generic brushes.*/,
                MapT::RoundEpsilon, MapT::MinVertexDist, COLLISION_MODEL_MAX_CURVE_ERROR, COLLISION_MODEL_MAX_CURVE_LENGTH, COLLISION_MODEL_MIN_NODE_SIZE);

            if (!CM->GetBoundingBox().IsInited())
            {
                Console->Warning("Collision model \"" + FileName + "\" has no solid objects.\n");
                delete CM;
                return NULL;
            }

            cmi.Instance = CM;
        }
        catch (const TextParserT::ParseError&)
        {
            Console->Warning(cf::va("Problem with parsing the collision model file \"%s\" near byte %lu (%.3f%%) of the file.\n", FileName.c_str(), TP.GetReadPosByte(), TP.GetReadPosPercent() * 100.0));
        }
    }
    else Console->Warning("Type of collision model file \"" + FileName + "\" not recognized.\n");

    if (cmi.Instance == NULL) return NULL;

    cmInfos.PushBack(cmi);
    return cmi.Instance;
}


/*
const CollisionModelT* CollModelManImplT::GetCM(std::istream& InFile, cf::SceneGraph::aux::PoolT& Pool, const ArrayT<CollisionModelStaticT::TerrainRefT>& Terrains)
{
    cmInfoT cmi;

    cmi.Instance=new CollisionModelStaticT(InFile, Pool, Terrains);
    cmi.FileName="";
    cmi.RefCount=1;
    cmi.NoDelete=false;

    cmInfos.PushBack(cmi);
    return cmi.Instance;
}
*/


const CollisionModelT* CollModelManImplT::GetCM(unsigned long Width, unsigned long Height, const ArrayT<Vector3dT>& Mesh, MaterialT* Material, const double MIN_NODE_SIZE)
{
    cmInfoT cmi;

    cmi.Instance = new CollisionModelStaticT(Width, Height, Mesh, Material, MIN_NODE_SIZE);
    cmi.FileName = "";
    cmi.RefCount = 1;
    cmi.NoDelete = false;

    cmInfos.PushBack(cmi);
    return cmi.Instance;
}


const CollisionModelT* CollModelManImplT::GetCM(const BoundingBox3T<double>& BB, MaterialT* Material)
{
    // Construct a single-brush entity from the BB in order to create the collision model.
    MapFileEntityT Entity;
    Entity.MFBrushes.PushBackEmpty();

    MapFileBrushT& Brush = Entity.MFBrushes[0];
    Brush.MFPlanes.PushBackEmpty(6);

    Brush.MFPlanes[0].Plane = Plane3dT(Vector3dT(-1,  0,  0), -BB.Min.x);
    Brush.MFPlanes[1].Plane = Plane3dT(Vector3dT( 0, -1,  0), -BB.Min.y);
    Brush.MFPlanes[2].Plane = Plane3dT(Vector3dT( 0,  0, -1), -BB.Min.z);
    Brush.MFPlanes[3].Plane = Plane3dT(Vector3dT( 1,  0,  0),  BB.Max.x);
    Brush.MFPlanes[4].Plane = Plane3dT(Vector3dT( 0,  1,  0),  BB.Max.y);
    Brush.MFPlanes[5].Plane = Plane3dT(Vector3dT( 0,  0,  1),  BB.Max.z);

    for (unsigned long PlaneNr = 0; PlaneNr < 6; PlaneNr++)
    {
        MapFilePlaneT& P = Brush.MFPlanes[PlaneNr];

        P.Material = Material;
        P.Plane.GetSpanVectorsByRotation(P.U, P.V);
        P.ShiftU = 0;
        P.ShiftV = 0;
    }


    const double COLLISION_MODEL_MAX_CURVE_ERROR  = 24.0;   // Unused with this model.
    const double COLLISION_MODEL_MAX_CURVE_LENGTH = -1.0;   // Unused with this model.
    const double COLLISION_MODEL_MIN_NODE_SIZE    = 64.0;   // Unused with this model.

    cmInfoT cmi;

    cmi.Instance = new CollisionModelStaticT(Entity, ArrayT<CollisionModelStaticT::TerrainRefT>(), true /*Use generic brushes.*/,
        MapT::RoundEpsilon, MapT::MinVertexDist, COLLISION_MODEL_MAX_CURVE_ERROR, COLLISION_MODEL_MAX_CURVE_LENGTH, COLLISION_MODEL_MIN_NODE_SIZE);
    cmi.FileName = "";
    cmi.RefCount = 1;
    cmi.NoDelete = false;

    cmInfos.PushBack(cmi);
    return cmi.Instance;
}


const CollisionModelT* CollModelManImplT::GetCM(const CollisionModelT* CollisionModel)
{
    // Keep NULL pointers from entering the records, they need never be reference counted.  ;-)
    if (CollisionModel == NULL) return NULL;

    for (unsigned long cmiNr = 0; cmiNr < cmInfos.Size(); cmiNr++)
    {
        cmInfoT& cmi = cmInfos[cmiNr];

        if (cmi.Instance == CollisionModel)
        {
            cmi.RefCount++;
            return cmi.Instance;
        }
    }


    cmInfoT cmi;

    cmi.Instance = CollisionModel;
    cmi.FileName = "";
    cmi.RefCount = 1;   // The passed-in instance is not reference-counted, the caller remains responsible for its deletion.
    cmi.NoDelete = true;

    cmInfos.PushBack(cmi);
    return cmi.Instance;
}


const std::string& CollModelManImplT::GetFileName(const CollisionModelT* CollisionModel) const
{
    for (unsigned long cmiNr = 0; cmiNr < cmInfos.Size(); cmiNr++)
        if (cmInfos[cmiNr].Instance == CollisionModel)
            return cmInfos[cmiNr].FileName;

    // If we ever get here, an unknown collision model instance was passed in.
    assert(false);

    static const std::string EmptyString = "";
    return EmptyString;
}


void CollModelManImplT::FreeCM(const CollisionModelT* CollisionModel)
{
    if (CollisionModel == NULL) return;

    for (unsigned long cmiNr = 0; cmiNr < cmInfos.Size(); cmiNr++)
    {
        cmInfoT& cmi = cmInfos[cmiNr];

        if (cmi.Instance == CollisionModel)
        {
            cmi.RefCount--;

            if (cmi.RefCount == 0)
            {
                if (!cmi.NoDelete) delete cmi.Instance;
                cmInfos.RemoveAt(cmiNr);
            }

            return;
        }
    }

    // If we ever get here, there was an attempt to free an unknown collision model instance.
    assert(false);
}


unsigned long CollModelManImplT::GetUniqueCMCount() const
{
    return cmInfos.Size();
}
