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

#include "MapFile.hpp"
#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "TextParser/TextParser.hpp"

#include <sstream>


using namespace cf;


const double cf::CA3DE_SCALE=25.4;


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
            Points[PointNr].x=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
            Points[PointNr].y=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
            Points[PointNr].z=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
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
        MFPlane.U.x=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
        MFPlane.U.y=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
        MFPlane.U.z=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
        TP.AssertAndSkipToken(")");

        // Texture V axis.
        TP.AssertAndSkipToken("(");
        MFPlane.V.x=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
        MFPlane.V.y=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
        MFPlane.V.z=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
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

                if (Coord<3) ControlPoints[ControlPoints.Size()-1]*=float(CA3DE_SCALE);
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
    Bounds.Min.x=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
    Bounds.Min.y=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
    Bounds.Min.z=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    Bounds.Max.x=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
    Bounds.Max.y=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
    Bounds.Max.z=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
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
    Position.x=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
    Position.y=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
    Position.z=TP.GetNextTokenAsFloat()*CA3DE_SCALE;
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
    Origin.x=TP.GetNextTokenAsFloat()*float(CA3DE_SCALE);
    Origin.y=TP.GetNextTokenAsFloat()*float(CA3DE_SCALE);
    Origin.z=TP.GetNextTokenAsFloat()*float(CA3DE_SCALE);
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    Angles.x=TP.GetNextTokenAsFloat();
    Angles.y=TP.GetNextTokenAsFloat();
    Angles.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    Scale         =TP.GetNextTokenAsFloat()*float(CA3DE_SCALE);
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

            if (Key=="light_radius")
            {
                // Translate light radius to Cafu units.
                double LightRadius=atof(Value.c_str())*CA3DE_SCALE;

                std::stringstream NewValue;

                NewValue << int(LightRadius);

                MFProperties[Key]=NewValue.str();
            }
        }
    }
}


static void MapFileVersionError(const std::string& Msg)
{
    Console->Print("Bad map file version: Expected 13, "+Msg+".\n");
    Console->Print("To fix this, you can load your cmap file into CaWE and re-save it.\n");
    Console->Print("This will automatically update your file to the required version!\n");

    Console->Warning("Bad map file version: Expected 13, "+Msg+".\n");
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

    if (Version!="13")
        MapFileVersionError("got "+Version);

    // Skip any group definitions.
    while (TP.PeekNextToken()=="GroupDef")
    {
        // Example line:
        //   GroupDef 0 "control room" "rgb(189, 206, 184)" 1 1 0
        for (unsigned int TokenNr=0; TokenNr<7; TokenNr++) TP.GetNextToken();
    }
}
