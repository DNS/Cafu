/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MapFile.hpp"
#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "TextParser/TextParser.hpp"


using namespace cf;


static void SkipGroupDef(TextParserT& TP)
{
    if (TP.PeekNextToken()=="Group")
    {
        TP.GetNextToken();  // The "Group" keyword.
        TP.GetNextToken();  // The group number.
    }
}


MapFileBrushT::MapFileBrushT(TextParserT& TP, unsigned long BrushNr)
{
    SkipGroupDef(TP);

    while (true)
    {
        std::string Token=TP.GetNextToken();

        if (Token=="}") break;     // End of brush

        // Read the three point plane definition.
        TP.PutBack(Token);
        MapFilePlaneT MFPlane;
        Vector3dT     Points[3];

        for (char PointNr=0; PointNr<3; PointNr++)
        {
            TP.AssertAndSkipToken("(");
            Points[PointNr].x=TP.GetNextTokenAsFloat();
            Points[PointNr].y=TP.GetNextTokenAsFloat();
            Points[PointNr].z=TP.GetNextTokenAsFloat();
            TP.AssertAndSkipToken(")");
        }

        try
        {
            // Die Reihenfolge der Punkte (im Uhrzeigersinn) ist wichtig, damit die Normalenvektoren stets aus dem Brush HERAUS zeigen!
            MFPlane.Plane=Plane3T<double>(Points[0], Points[1], Points[2], 0.1);
        }
        catch (const DivisionByZeroE&)
        {
            Console->Warning(cf::va("Entity ??, brush %lu: plane %lu has colinear points.\n", BrushNr, MFPlanes.Size()));
            throw TextParserT::ParseError();
        }


        // Read the texture definition.
        const std::string MaterialName=TP.GetNextToken();
        MFPlane.Material=MaterialManager->GetMaterial(MaterialName);

        if (MFPlane.Material==NULL)
        {
            Console->Warning(cf::va("Entity ??, Brush %lu, plane %lu: Unknown material \"%s\".", BrushNr, MFPlanes.Size(), MaterialName.c_str()));
            throw TextParserT::ParseError();
        }

        // Begin of surface information.
        TP.AssertAndSkipToken("(");

        // Texture generation mode has to be PlaneProj (==2) for all faces of a brush.
        TP.AssertAndSkipToken("2");

        // Texture shift.
        MFPlane.ShiftU=TP.GetNextTokenAsFloat();
        MFPlane.ShiftV=TP.GetNextTokenAsFloat();

        // The texture rotation is only relevant for CaWE. Just overread it here.
        TP.GetNextToken();

        // Texture U axis.
        TP.AssertAndSkipToken("(");
        MFPlane.U.x=TP.GetNextTokenAsFloat();
        MFPlane.U.y=TP.GetNextTokenAsFloat();
        MFPlane.U.z=TP.GetNextTokenAsFloat();
        TP.AssertAndSkipToken(")");

        // Texture V axis.
        TP.AssertAndSkipToken("(");
        MFPlane.V.x=TP.GetNextTokenAsFloat();
        MFPlane.V.y=TP.GetNextTokenAsFloat();
        MFPlane.V.z=TP.GetNextTokenAsFloat();
        TP.AssertAndSkipToken(")");

        // End of surface information.
        TP.AssertAndSkipToken(")");

        // Store plane in brush.
        MFPlanes.PushBack(MFPlane);
    }
}


MapFileBezierPatchT::MapFileBezierPatchT(TextParserT& TP)
{
    TP.AssertAndSkipToken("{");
    SkipGroupDef(TP);

    const std::string MaterialName=TP.GetNextToken();
    Material=MaterialManager->GetMaterial(MaterialName);

    if (Material==NULL)
    {
        Console->Warning("Bezier Patch: Unknown material \""+MaterialName+"\".\n");
        throw TextParserT::ParseError();
    }

    // Skip additional surface information, as it is only relevant for CaWE.
    TP.SkipBlock("(", ")", false);

    // Dimensions
    TP.AssertAndSkipToken("(");
    SizeX      =TP.GetNextTokenAsInt();
    SizeY      =TP.GetNextTokenAsInt();
    SubdivsHorz=TP.GetNextTokenAsInt();
    SubdivsVert=TP.GetNextTokenAsInt();
    TP.AssertAndSkipToken(")");

    for (unsigned long y=0; y<SizeY; y++)
        for (unsigned long x=0; x<SizeX; x++)
        {
            TP.AssertAndSkipToken("(");

            for (unsigned long Coord=0; Coord<5; Coord++)
            {
                ControlPoints.PushBack(TP.GetNextTokenAsFloat());
            }

            TP.AssertAndSkipToken(")");
        }

    TP.AssertAndSkipToken("}");
}


MapFileTerrainT::MapFileTerrainT(TextParserT& TP)
{
    TP.AssertAndSkipToken("{");
    SkipGroupDef(TP);

    const std::string MaterialName=TP.GetNextToken();
    Material=MaterialManager->GetMaterial(MaterialName);

    TP.AssertAndSkipToken("(");
    Bounds.Min.x=TP.GetNextTokenAsFloat();
    Bounds.Min.y=TP.GetNextTokenAsFloat();
    Bounds.Min.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    Bounds.Max.x=TP.GetNextTokenAsFloat();
    Bounds.Max.y=TP.GetNextTokenAsFloat();
    Bounds.Max.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    SideLength=TP.GetNextTokenAsInt();
    TP.AssertAndSkipToken(")");

    HeightData.Clear();
    HeightData.PushBackEmpty(SideLength*SideLength);

    for (unsigned long i=0; i<HeightData.Size(); i++)
        HeightData[i]=TP.GetNextTokenAsInt();

    TP.AssertAndSkipToken("}");
}


MapFilePlantT::MapFilePlantT(TextParserT& TP)
{
    TP.AssertAndSkipToken("{");
    SkipGroupDef(TP);

    DescrFileName=TP.GetNextToken();
    RandomSeed   =TP.GetNextTokenAsInt();

    TP.AssertAndSkipToken("(");
    Position.x=TP.GetNextTokenAsFloat();
    Position.y=TP.GetNextTokenAsFloat();
    Position.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    Angles.x=TP.GetNextTokenAsFloat();
    Angles.y=TP.GetNextTokenAsFloat();
    Angles.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("}");
}


MapFileModelT::MapFileModelT(TextParserT& TP)
{
    TP.AssertAndSkipToken("{");
    SkipGroupDef(TP);

    Model    =TP.GetNextToken();
    CollModel=TP.GetNextToken();
    Label    =TP.GetNextToken();

    TP.AssertAndSkipToken("(");
    Origin.x=TP.GetNextTokenAsFloat();
    Origin.y=TP.GetNextTokenAsFloat();
    Origin.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    Angles.x=TP.GetNextTokenAsFloat();
    Angles.y=TP.GetNextTokenAsFloat();
    Angles.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    Scale         =TP.GetNextTokenAsFloat();
    SeqNumber     =TP.GetNextTokenAsInt();
    FrameOffset   =TP.GetNextTokenAsFloat();
    FrameTimeScale=TP.GetNextTokenAsFloat();
    Animate       =(TP.GetNextTokenAsInt()!=0);

    TP.AssertAndSkipToken("}");
}


MapFileEntityT::MapFileEntityT(unsigned long Index, TextParserT& TP)
    : MFIndex(Index)
{
    TP.AssertAndSkipToken("{");
    SkipGroupDef(TP);

    while (true)
    {
        std::string Token=TP.GetNextToken();

        if (Token=="}") break;          // End of Entity.

        if (Token=="{")                 // Begin of Brush.
        {
            MFBrushes.PushBack(MapFileBrushT(TP, MFBrushes.Size()));
        }
        else if (Token=="PatchDef")     // Patch definition.
        {
            MFPatches.PushBack(MapFileBezierPatchT(TP));
        }
        else if (Token=="TerrainDef")   // Terrain definition.
        {
            MFTerrains.PushBack(MapFileTerrainT(TP));
        }
        else if (Token=="PlantDef")     // Plant definition.
        {
            MFPlants.PushBack(MapFilePlantT(TP));
        }
        else if (Token=="ModelDef")     // Model definition.
        {
            MFModels.PushBack(MapFileModelT(TP));
        }
        else                            // Property Pair.
        {
            const std::string Key  =Token;
            const std::string Value=TP.GetNextToken();

            if (Key=="{" || Key=="}" || Key=="(" || Key==")") throw TextParserT::ParseError();

            MFProperties[Key]=Value;
        }
    }
}


void MapFileEntityT::Transform(const MatrixT& Mat)
{
    for (unsigned int i = 0; i < MFBrushes.Size(); i++)
    {
        for (unsigned int p = 0; p < MFBrushes[i].MFPlanes.Size(); p++)
        {
            MapFilePlaneT&  MFP = MFBrushes[i].MFPlanes[p];
            const Vector3dT St  = Mat.Mul1(MFP.Plane.Normal * MFP.Plane.Dist);  // A point on the transformed plane ("Stützvektor").

            MFP.Plane.Normal = normalizeOr0(Mat.Mul0(MFP.Plane.Normal), 0.0001);
            MFP.Plane.Dist   = dot(St, MFP.Plane.Normal);

            MFP.U = Mat.Mul0(MFP.U);
            MFP.V = Mat.Mul0(MFP.V);

            // This follows the code in FaceNodeT::InitRenderMeshesAndMats().
            MFP.ShiftU -= dot(Vector3dT(Mat[0][3], Mat[1][3], Mat[2][3]), MFP.U) / dot(MFP.U, MFP.U);
            MFP.ShiftV -= dot(Vector3dT(Mat[0][3], Mat[1][3], Mat[2][3]), MFP.V) / dot(MFP.V, MFP.V);
        }
    }

    for (unsigned int i = 0; i < MFPatches.Size(); i++)
    {
        ArrayT<float>& CPs = MFPatches[i].ControlPoints;

        for (unsigned long j = 0; j < CPs.Size(); j += 5)
        {
            const Vector3fT v = Mat.Mul1(Vector3fT(CPs[j + 0], CPs[j + 1], CPs[j + 2]));

            CPs[j + 0] = v.x;
            CPs[j + 1] = v.y;
            CPs[j + 2] = v.z;
        }
    }

    for (unsigned int i = 0; i < MFTerrains.Size(); i++)
    {
        MFTerrains[i].Bounds.Min = Mat.Mul1(MFTerrains[i].Bounds.Min);
        MFTerrains[i].Bounds.Max = Mat.Mul1(MFTerrains[i].Bounds.Max);
    }

    for (unsigned int i = 0; i < MFPlants.Size(); i++)
    {
        MFPlants[i].Position = Mat.Mul1(MFPlants[i].Position);
    }

    for (unsigned int i = 0; i < MFModels.Size(); i++)
    {
        MFModels[i].Origin = Mat.Mul1(MFModels[i].Origin);
    }
}


static void MapFileVersionError(const std::string& Msg)
{
    Console->Print("Bad map file version: Expected 14, " + Msg + ".\n");
    Console->Print("To fix this, you can load your cmap file into CaWE and re-save it.\n");
    Console->Print("This will automatically update your file to the required version!\n");

    Console->Warning("Bad map file version: Expected 14, " + Msg + ".\n");
    throw TextParserT::ParseError();
}


void cf::MapFileReadHeader(TextParserT& TP)
{
    if (TP.IsAtEOF())
    {
        Console->Warning("Unable to open map file.\n");
        throw TextParserT::ParseError();
    }

    if (TP.PeekNextToken()!="Version")
        MapFileVersionError("but could not find the \"Version\" keyword");

    TP.AssertAndSkipToken("Version");
    const std::string Version=TP.GetNextToken();

    if (Version != "14")
        MapFileVersionError("got "+Version);

    // Skip any group definitions.
    while (TP.PeekNextToken()=="GroupDef")
    {
        // Example line:
        //   GroupDef 0 "control room" "rgb(189, 206, 184)" 1 1 0
        for (unsigned int TokenNr=0; TokenNr<7; TokenNr++) TP.GetNextToken();
    }
}
