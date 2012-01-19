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

#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "LuaAux.hpp"
#include "GameConfig.hpp"
#include "MapBezierPatch.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "MapEntity.hpp"
#include "MapFace.hpp"
#include "MapModel.hpp"
#include "MapPlant.hpp"
#include "MapTerrain.hpp"
#include "EditorMaterial.hpp"
#include "EditorMaterialManager.hpp"

#include "Math3D/Plane3.hpp"
#include "Models/Model_cmdl.hpp"
#include "TextParser/TextParser.hpp"
#include "SceneGraph/LightMapMan.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/CollisionModel_base.hpp"

#include "wx/wx.h"
#include "wx/progdlg.h"


static const double CAFU_ENG_SCALE=25.4;
static const bool   WriteComments =false;
static unsigned int MapFileVersion=0;


/// This function serializes a given float f1 to a string s, such that:
///   - s is minimal (uses the least number of decimal digits required),
///   - unserializing s back to a float f2 yields f1==f2.
/// See my post "float to string to float, with first float == second float"
/// to comp.lang.c++ on 2009-10-06 for additional details.
static std::string serialize(float f1)
{
    // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
    // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
    // that is, max_digits10. See http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
    const unsigned int DIGITS10    =std::numeric_limits<float>::digits10;
    const unsigned int MAX_DIGITS10=DIGITS10+3;

    std::string  s;
    unsigned int prec;

    for (prec=DIGITS10; prec<=MAX_DIGITS10; prec++)
    {
        std::stringstream ss;

        ss.precision(prec);
        ss << f1;

        s=ss.str();

        float f2;
        ss >> f2;

        if (f2==f1) break;
    }

    wxASSERT(prec<=MAX_DIGITS10);
    return s;
}


static std::string serialize(const Vector3fT& v)
{
    return serialize(v.x)+" "+serialize(v.y)+" "+serialize(v.z);
}


/***************************/
/*** Load/Save functions ***/
/***************************/

void MapElementT::Load_cmap(TextParserT& TP, MapDocumentT& MapDoc)
{
    if (TP.PeekNextToken()=="Group")
    {
        TP.AssertAndSkipToken("Group");
        const unsigned long GroupNr=TP.GetNextTokenAsInt();

        if (GroupNr<MapDoc.GetGroups().Size())
            SetGroup(MapDoc.GetGroups()[GroupNr]);
    }
}


void MapElementT::Save_cmap(std::ostream& OutFile, unsigned long ElemNr, const MapDocumentT& MapDoc) const
{
    const int GroupNr=MapDoc.GetGroups().Find(m_Group);

    if (GroupNr!=-1)
        OutFile << "    Group " << GroupNr << "\n";
}


MapFaceT MapFaceT::Create_cmap(TextParserT& TP, EditorMatManT& MatMan)
{
    MapFaceT Face;

    TP.AssertAndSkipToken("(");
    Face.m_PlanePoints[0].x=TP.GetNextTokenAsFloat();
    Face.m_PlanePoints[0].y=TP.GetNextTokenAsFloat();
    Face.m_PlanePoints[0].z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    Face.m_PlanePoints[1].x=TP.GetNextTokenAsFloat();
    Face.m_PlanePoints[1].y=TP.GetNextTokenAsFloat();
    Face.m_PlanePoints[1].z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    Face.m_PlanePoints[2].x=TP.GetNextTokenAsFloat();
    Face.m_PlanePoints[2].y=TP.GetNextTokenAsFloat();
    Face.m_PlanePoints[2].z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    std::string TexName=TP.GetNextToken();

    if (MapFileVersion<=9)
    {
        // All faces below version 10 have no information about the creation of their texture
        // coordinates mode and it is therefore initialized as plane projected.
        Face.m_SurfaceInfo.TexCoordGenMode=PlaneProj;

        // Normally this code would be sufficient, but it fails with TexName was empty and non-quoted.
        // Face.m_SurfaceInfo.trans[0]=TP.GetNextTokenAsFloat();
        // Face.m_SurfaceInfo.trans[1]=TP.GetNextTokenAsFloat();

        // This code solves the problem.
        Face.m_SurfaceInfo.Trans[0]=TP.GetNextTokenAsFloat();
        std::string TestToken=TP.GetNextToken();
        if (TestToken=="(")
        {
            // The texture name was apparently missing, so fix the problem now as well as possible.
            Face.m_SurfaceInfo.Trans[1]=Face.m_SurfaceInfo.Trans[0];
            Face.m_SurfaceInfo.Trans[0]=atof(TexName.c_str());
            TexName="";
            TP.PutBack(TestToken);
        }
        else Face.m_SurfaceInfo.Trans[1]=atof(TestToken.c_str());

        TP.AssertAndSkipToken("(");
        Face.m_SurfaceInfo.UAxis.x=TP.GetNextTokenAsFloat();
        Face.m_SurfaceInfo.UAxis.y=TP.GetNextTokenAsFloat();
        Face.m_SurfaceInfo.UAxis.z=TP.GetNextTokenAsFloat();
        TP.AssertAndSkipToken(")");

        TP.AssertAndSkipToken("(");
        Face.m_SurfaceInfo.VAxis.x=TP.GetNextTokenAsFloat();
        Face.m_SurfaceInfo.VAxis.y=TP.GetNextTokenAsFloat();
        Face.m_SurfaceInfo.VAxis.z=TP.GetNextTokenAsFloat();
        TP.AssertAndSkipToken(")");

        Face.m_SurfaceInfo.Rotate=TP.GetNextTokenAsFloat();
        TP.GetNextToken();      // ID of SmoothingGroup this face is in.
    }
    else
    {
        Face.m_SurfaceInfo=SurfaceInfoT::Create_cmap(TP);
    }

    if (MapFileVersion<=6)
    {
        // Scale the coordinates down.
        Face.m_PlanePoints[0]/=CAFU_ENG_SCALE;
        Face.m_PlanePoints[1]/=CAFU_ENG_SCALE;
        Face.m_PlanePoints[2]/=CAFU_ENG_SCALE;
    }

    // Restore the scale values and renormalize the axes (when MapFileVersion<=9).
    const float LenU=length(Face.m_SurfaceInfo.UAxis);
    const float LenV=length(Face.m_SurfaceInfo.VAxis);

    if (MapFileVersion<=9)
    {
        // Renormalize the axes.
        Face.m_SurfaceInfo.UAxis/=LenU;
        Face.m_SurfaceInfo.VAxis/=LenV;
    }

    Face.m_Plane   =Plane3fT(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
    Face.m_Material=MatMan.FindMaterial(TexName, true /*return dummy if not found*/);

    if (MapFileVersion<=6)
    {
        Face.m_SurfaceInfo.Scale[0]=1.0/(LenU/CAFU_ENG_SCALE * Face.m_Material->GetWidth());
        Face.m_SurfaceInfo.Scale[1]=1.0/(LenV/CAFU_ENG_SCALE * Face.m_Material->GetHeight());
    }
    else if (MapFileVersion<=8)
    {
        Face.m_SurfaceInfo.Scale[0]=1.0/(LenU * Face.m_Material->GetWidth());
        Face.m_SurfaceInfo.Scale[1]=1.0/(LenV * Face.m_Material->GetHeight());
    }
    else if (MapFileVersion==9)
    {
        Face.m_SurfaceInfo.Scale[0]=1.0/LenU;
        Face.m_SurfaceInfo.Scale[1]=1.0/LenV;
    }

    if (MapFileVersion<=8)
    {
        Face.m_SurfaceInfo.Trans[0]/=Face.m_Material->GetWidth();
        Face.m_SurfaceInfo.Trans[1]/=Face.m_Material->GetHeight();
    }

    return Face;
}


void MapFaceT::Save_cmap(std::ostream& OutFile) const
{
    OutFile << "   "
            << " ( " << serialize(m_PlanePoints[0]) << " )"
            << " ( " << serialize(m_PlanePoints[1]) << " )"
            << " ( " << serialize(m_PlanePoints[2]) << " )"
            << " \"" << m_Material->GetName() << "\" ";

    m_SurfaceInfo.Save_cmap(OutFile);
}


MapBrushT* MapBrushT::Create_cmap(TextParserT& TP, MapDocumentT& MapDoc, unsigned long EntityNr, unsigned long BrushNr)
{
    MapBrushT* Brush=new MapBrushT();

    TP.AssertAndSkipToken("{");

    Brush->Load_cmap(TP, MapDoc);   // The method of the MapElementT base class.
    Brush->m_Faces.Clear();

    while (true)
    {
        std::string Token=TP.GetNextToken();
        TP.PutBack(Token);

        if (Token!="(") break;

        Brush->m_Faces.PushBack(MapFaceT::Create_cmap(TP, MapDoc.GetGameConfig()->GetMatMan()));
    }

    TP.AssertAndSkipToken("}");

    Brush->CompleteFaceVertices();

    if (!Brush->IsValid())
    {
        wxLogWarning("Entity %lu, primitive %lu: The brush could not be created from its planes.", EntityNr, BrushNr);
    }

    wxASSERT(Brush->IsValid());
    return Brush;
}


void MapBrushT::Save_cmap(std::ostream& OutFile, unsigned long PrimitiveNr, const MapDocumentT& MapDoc) const
{
    // Only write the "// Primitive XY" comment every 20-th primitive.
    // This keeps the diff smaller for cmap files that are under (Subversion) revision control,
    // because deletions or insertions of primitives cause all subsequent brushes to get renumbered.
    if ((PrimitiveNr % 20)==0 && WriteComments) OutFile << "\n  { // Primitive " << PrimitiveNr << "\n";
                                           else OutFile << "\n  {\n";

    MapElementT::Save_cmap(OutFile, PrimitiveNr, MapDoc);

    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
        if (m_Faces[FaceNr].GetVertices().Size()>0)
            m_Faces[FaceNr].Save_cmap(OutFile);

    OutFile << "  }\n";
}


void MapBezierPatchT::Load_cmap(TextParserT& TP, MapDocumentT& MapDoc)
{
    TP.AssertAndSkipToken("PatchDef");
    TP.AssertAndSkipToken("{");

    MapElementT::Load_cmap(TP, MapDoc);

    wxString texname=TP.GetNextToken();
    SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(texname, true /*Create dummy if not found.*/));

    if (MapFileVersion<=9)
    {
        // All bezier patches below version 10 have no surface information and are therefore
        // initialized with custom texture coordinates generation mode.
        SurfaceInfo.TexCoordGenMode=Custom;
    }
    else
    {
        SurfaceInfo=SurfaceInfoT::Create_cmap(TP);
    }

    TP.AssertAndSkipToken("(");

    unsigned long width =TP.GetNextTokenAsInt();
    unsigned long height=TP.GetNextTokenAsInt();

    SetSize(width, height);

    SubdivsHorz=(MapFileVersion>=8) ? TP.GetNextTokenAsInt() : -1;
    SubdivsVert=(MapFileVersion>=8) ? TP.GetNextTokenAsInt() : -1;

    TP.AssertAndSkipToken(")");

    for (unsigned long y=0; y<cv_Height; y++)
    {
        for (unsigned long x=0 ; x<cv_Width; x++)
        {
            TP.AssertAndSkipToken("(");

            Vector3fT Pos;
            Pos.x=TP.GetNextTokenAsFloat();
            Pos.y=TP.GetNextTokenAsFloat();
            Pos.z=TP.GetNextTokenAsFloat();
            // Scaling was required for mapfile_version <= 6.
            if (MapFileVersion<=6) Pos/=CAFU_ENG_SCALE;
            SetCvPos(x, y, Pos);

            Vector3fT TexCoord;
            TexCoord.x=TP.GetNextTokenAsFloat();
            TexCoord.y=TP.GetNextTokenAsFloat();
            SetCvUV(x, y, TexCoord);

            TP.AssertAndSkipToken(")");
        }
    }

    TP.AssertAndSkipToken("}");
}


void MapBezierPatchT::Save_cmap(std::ostream& OutFile, unsigned long PrimitiveNr, const MapDocumentT& MapDoc) const
{
    // Only write the "// Primitive XY" comment every 20-th primitive.
    // This keeps the diff smaller for cmap files that are under (Subversion) revision control,
    // because deletions or insertions of primitives cause all subsequent brushes to get renumbered.
    if ((PrimitiveNr % 20)==0 && WriteComments) OutFile << "\n" << "PatchDef    // Primitive " << PrimitiveNr << "\n" << "{\n";
                                           else OutFile << "\n" << "PatchDef"                                 << "\n" << "{\n";

    MapElementT::Save_cmap(OutFile, PrimitiveNr, MapDoc);

    OutFile << "\"" << GetMaterial()->GetName() << "\"" << "\n";
    SurfaceInfo.Save_cmap(OutFile);
    OutFile << "( " << cv_Width << " " << cv_Height << " " << SubdivsHorz << " " << SubdivsVert << " )\n";

    for (unsigned long y=0; y<cv_Height; y++)
    {
        for (unsigned long x=0; x<cv_Width; x++)
        {
            const Vector3fT& Pos     =GetCvPos(x, y);
            const Vector3fT& TexCoord=GetCvUV (x, y);

            // Note that since mapfile_version 7, we no longer scale the vertex coordinates by CAFU_ENG_SCALE.
            OutFile << " ( " << serialize(Pos)
                    << " " << serialize(TexCoord.x)
                    << " " << serialize(TexCoord.y) << " )";
        }

        OutFile << "\n";
    }

    OutFile << "}\n";
}


void MapTerrainT::Load_cmap(TextParserT& TP, MapDocumentT& MapDoc)
{
    TP.AssertAndSkipToken("TerrainDef");
    TP.AssertAndSkipToken("{");

    MapElementT::Load_cmap(TP, MapDoc);

    SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(TP.GetNextToken(), true));

    TP.AssertAndSkipToken("(");
    m_TerrainBounds.Min.x=TP.GetNextTokenAsFloat();
    m_TerrainBounds.Min.y=TP.GetNextTokenAsFloat();
    m_TerrainBounds.Min.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    m_TerrainBounds.Max.x=TP.GetNextTokenAsFloat();
    m_TerrainBounds.Max.y=TP.GetNextTokenAsFloat();
    m_TerrainBounds.Max.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    m_Resolution=TP.GetNextTokenAsInt();
    TP.AssertAndSkipToken(")");

    m_HeightData.Clear();
    m_HeightData.PushBackEmpty(m_Resolution*m_Resolution);

    for (unsigned long y=0; y<m_Resolution; y++)
        for (unsigned long x=0 ; x<m_Resolution; x++)
            m_HeightData[y*m_Resolution+x]=TP.GetNextTokenAsInt();

    TP.AssertAndSkipToken("}");
    m_NeedsUpdate=true;
}


void MapTerrainT::Save_cmap(std::ostream& OutFile, unsigned long PrimitiveNr, const MapDocumentT& MapDoc) const
{
    // Only write the "// Primitive XY" comment every 20-th primitive.
    // This keeps the diff smaller for cmap files that are under (Subversion) revision control,
    // because deletions or insertions of primitives cause all subsequent brushes to get renumbered.
    if ((PrimitiveNr % 20)==0 && WriteComments) OutFile << "\n" << "TerrainDef    // Primitive " << PrimitiveNr << "\n" << "{\n";
                                           else OutFile << "\n" << "TerrainDef"                                 << "\n" << "{\n";

    MapElementT::Save_cmap(OutFile, PrimitiveNr, MapDoc);

    OutFile << "  \"" << m_Material->GetName() << "\"" << "\n";
    OutFile << "  ( " << serialize(m_TerrainBounds.Min) << " )\n";
    OutFile << "  ( " << serialize(m_TerrainBounds.Max) << " )\n";
    OutFile << "  ( " << m_Resolution << " )" << "\n";

    OutFile << "  ";

    // Serialize the height data.
    for (unsigned long i=1; i<=m_HeightData.Size(); i++)
    {
        OutFile << std::setfill(' ') << std::setw(5) << m_HeightData[i-1];
        OutFile << (((i % 20)==0) ? "\n  " : " ");
    }

    OutFile << "\n}\n";
}


void MapPlantT::Load_cmap(TextParserT& TP, MapDocumentT& MapDoc)
{
    TP.AssertAndSkipToken("PlantDef");
    TP.AssertAndSkipToken("{");

    MapElementT::Load_cmap(TP, MapDoc);

    m_DescrFileName=TP.GetNextToken();
    m_RandomSeed   =TP.GetNextTokenAsInt();

    TP.AssertAndSkipToken("(");
    m_Position.x=TP.GetNextTokenAsFloat();
    m_Position.y=TP.GetNextTokenAsFloat();
    m_Position.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    m_Angles[ROLL ]=TP.GetNextTokenAsFloat();
    m_Angles[PITCH]=TP.GetNextTokenAsFloat();
    m_Angles[YAW  ]=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("}");
    m_Tree=TreeT(MapDoc.GetPlantDescrMan().GetPlantDescription(std::string(m_DescrFileName)), m_RandomSeed);
}


void MapPlantT::Save_cmap(std::ostream& OutFile, unsigned long PlantNr, const MapDocumentT& MapDoc) const
{
    OutFile << "\n" << "  PlantDef" << " { ";

    MapElementT::Save_cmap(OutFile, PlantNr, MapDoc);

    OutFile << "\"" << m_DescrFileName << "\"" << " ";
    OutFile << m_RandomSeed << " ";
    OutFile << "( " << serialize(m_Position) << " ) ";
    OutFile << "( " << serialize(m_Angles[ROLL]) << " " << serialize(m_Angles[PITCH]) << " " << serialize(m_Angles[YAW]) << " ) ";

    OutFile << "}\n";
}


void MapModelT::Load_cmap(TextParserT& TP, MapDocumentT& MapDoc)
{
    TP.AssertAndSkipToken("ModelDef");
    TP.AssertAndSkipToken("{");

    MapElementT::Load_cmap(TP, MapDoc);

    m_ModelFileName    =TP.GetNextToken();
    m_Model            =MapDoc.GetGameConfig()->GetModel(m_ModelFileName);
    m_CollModelFileName=TP.GetNextToken();

    m_Label=TP.GetNextToken();

    TP.AssertAndSkipToken("(");
    m_Origin.x=TP.GetNextTokenAsFloat();
    m_Origin.y=TP.GetNextTokenAsFloat();
    m_Origin.z=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken("(");
    m_Angles[ROLL ]=TP.GetNextTokenAsFloat();
    m_Angles[PITCH]=TP.GetNextTokenAsFloat();
    m_Angles[YAW  ]=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken(")");

    m_Scale         =TP.GetNextTokenAsFloat();
    const int SequNr=TP.GetNextTokenAsInt();
    m_FrameOffset   =TP.GetNextTokenAsFloat();
    m_FrameTimeScale=TP.GetNextTokenAsFloat();
    m_Animated      =TP.GetNextTokenAsInt()!=0;

    m_AnimExpr=m_Model->GetAnimExprPool().GetStandard(SequNr, m_FrameOffset);
    TP.AssertAndSkipToken("}");
}


void MapModelT::Save_cmap(std::ostream& OutFile, unsigned long ModelNr, const MapDocumentT& MapDoc) const
{
    OutFile << "\n" << "  ModelDef" << " { ";

    MapElementT::Save_cmap(OutFile, ModelNr, MapDoc);

    OutFile << "\"" << m_ModelFileName     << "\" ";
    OutFile << "\"" << m_CollModelFileName << "\" ";
    OutFile << "\"" << m_Label             << "\" ";

    OutFile << "( " << serialize(m_Origin) << " ) ";
    OutFile << "( " << serialize(m_Angles[ROLL]) << " " << serialize(m_Angles[PITCH]) << " " << serialize(m_Angles[YAW]) << " ) ";

    OutFile << serialize(m_Scale)          << " ";
    OutFile << m_AnimExpr->GetSequNr()     << " ";
    OutFile << serialize(m_FrameOffset)    << " ";
    OutFile << serialize(m_FrameTimeScale) << " ";
    OutFile << int(m_Animated)             << " ";

    OutFile << "}\n";
}


void EntPropertyT::Load_cmap(TextParserT& TP)
{
    Key  =TP.GetNextToken();
    Value=TP.GetNextToken();

    if (Key=="angles")
    {
        const Vector3fT Angles=GetVector3f();

        float Pitch=Angles.x;
        float Yaw  =Angles.y;
        float Roll =Angles.z;

        // Angles get special treatment because we measure angles a little differently:
        // Pitch      :  Up (0) is along the z-axis. CaWE: Measured counter-clockwise. Cafu engine: Measured clockwise.
        // Heading/Yaw:  CaWE: North (0) is along the x-axis, measured counter-clockwise. Cafu engine: North is along the y-axis, measured clockwise.
        // Bank/Roll  :  Up (0) is along the z-axis. CaWE: Measured counter-clockwise. Cafu engine: Measured clockwise.
        Pitch=  -Pitch; while (Pitch<0.0) Pitch+=360.0; while (Pitch>360.0) Pitch-=360.0;
        Yaw  =90-Yaw;   while (Yaw  <0.0) Yaw  +=360.0; while (Yaw  >360.0) Yaw  -=360.0;
        Roll =  -Roll;  while (Roll <0.0) Roll +=360.0; while (Roll >360.0) Roll -=360.0;

        Value=wxString::Format("%g %g %g", Pitch, Yaw, Roll);
    }

    if (MapFileVersion<=6 && Key=="origin")
    {
        const Vector3fT Origin=GetVector3f()/CAFU_ENG_SCALE;

        Value=wxString::Format("%.1f %.1f %.1f", Origin.x, Origin.y, Origin.z);
    }
}


void EntPropertyT::Save_cmap(std::ostream& OutFile) const
{
    // Don't save properties with default values into the file.
    if (Key=="" || Value=="" || Value=="0") return;

    wxString WriteValue=Value;

    if (Key=="angles")
    {
        const Vector3fT Angles=GetVector3f();

        float Pitch=Angles.x;
        float Yaw  =Angles.y;
        float Roll =Angles.z;

        // Angles get special treatment because we measure angles a little differently:
        // Pitch      :  Up (0) is along the z-axis. CaWE: Measured counter-clockwise. Cafu engine: Measured clockwise.
        // Heading/Yaw:  CaWE: North (0) is along the x-axis, measured counter-clockwise. Cafu engine: North is along the y-axis, measured clockwise.
        // Bank/Roll  :  Up (0) is along the z-axis. CaWE: Measured counter-clockwise. Cafu engine: Measured clockwise.
        Pitch=  -Pitch; while (Pitch<0.0) Pitch+=360.0; while (Pitch>360.0) Pitch-=360.0;
        Yaw  =90-Yaw;   while (Yaw  <0.0) Yaw  +=360.0; while (Yaw  >360.0) Yaw  -=360.0;
        Roll =  -Roll;  while (Roll <0.0) Roll +=360.0; while (Roll >360.0) Roll -=360.0;

        WriteValue=wxString::Format("%g %g %g", Pitch, Yaw, Roll);
    }

    // Keys may contain white-space, e.g. importing D3 map files may sometimes bring some with them.
    if (Key.Find(' ')==-1) OutFile << "  "   << Key << " \""   << WriteValue << "\"\n";
                      else OutFile << "  \"" << Key << "\" \"" << WriteValue << "\"\n";
}


void MapEntityBaseT::Load_cmap(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
{
    if (EntityNr==0)
    {
        MapFileVersion=0;

        if (TP.PeekNextToken()=="Version")
        {
            TP.GetNextToken();
            MapFileVersion=TP.GetNextTokenAsInt();
        }

        while (TP.PeekNextToken()=="GroupDef")
        {
            MapDoc.GetGroups().PushBack(new GroupT(GroupT::Create_cmap(TP)));
        }
    }

    unsigned long NrOfPrimitives=0;

    TP.AssertAndSkipToken("{");
    MapElementT::Load_cmap(TP, MapDoc);

    while (true)
    {
        std::string Token=TP.GetNextToken();
        TP.PutBack(Token);

        if (Token=="}")
        {
            // End of entity definition.
            break;
        }
        else if (Token=="{")
        {
            // A brush definition.
            AddPrim(MapBrushT::Create_cmap(TP, MapDoc, EntityNr, NrOfPrimitives));
            NrOfPrimitives++;
        }
        else if (Token=="PatchDef")
        {
            // A patch definition.
            MapBezierPatchT* BP=new MapBezierPatchT(MapDoc.GetGameConfig()->GetMatMan().GetDefaultMaterial(), MapDoc.GetLightMapMan());

            BP->Load_cmap(TP, MapDoc);
            AddPrim(BP);
            NrOfPrimitives++;
        }
        else if (Token=="TerrainDef")
        {
            // A terrain definition.
            MapTerrainT* Terrain=new MapTerrainT();

            Terrain->Load_cmap(TP, MapDoc);
            AddPrim(Terrain);
            NrOfPrimitives++;
        }
        else if (Token=="PlantDef")
        {
            // A plant definition.
            MapPlantT* Plant=new MapPlantT();

            Plant->Load_cmap(TP, MapDoc);
            AddPrim(Plant);
            NrOfPrimitives++;
        }
        else if (Token=="ModelDef")
        {
            // A model definition.
            MapModelT* Model=new MapModelT(MapDoc, "dummy", Vector3fT());

            Model->Load_cmap(TP, MapDoc);
            AddPrim(Model);
            NrOfPrimitives++;
        }
        else
        {
            // A property definition.
            EntPropertyT NewProp;
            NewProp.Load_cmap(TP);

            if (NewProp.Key=="wad")
                continue;

            if (NewProp.Key=="mapfile_version")
            {
                if (MapFileVersion<13) MapFileVersion=wxAtoi(NewProp.Value);

                if (MapFileVersion<6 || MapFileVersion>MapDocumentT::CMAP_FILE_VERSION)
                {
                    wxMessageBox(wxString::Format("Expected cmap file version 6 to %u, but found version %u.", MapDocumentT::CMAP_FILE_VERSION, MapFileVersion), "Could not load cmap file. Sorry.");
                    throw TextParserT::ParseError();
                }
                continue;
            }

            if (NewProp.Key=="angle")
            {
                // "angle" keys in cmap files?? This should never happen...
                NewProp.Key="angles";

                     if (NewProp.Value=="-1") NewProp.Value="-90 0 0";
                else if (NewProp.Value=="-2") NewProp.Value="90 0 0";
                else                          NewProp.Value="0 "+NewProp.Value+" 0";
            }

            // Insert the new property.
            // We cannot simply call   m_Properties.PushBack(NewProp);   here, because there might already be a property with this key.
            // In this case, instead of having another property with the same key, we want to keep the value of the last occurrence.
            FindProperty(NewProp.Key, NULL, true /*Create*/)->Value=NewProp.Value;
        }

        if (ProgressDialog!=NULL && (NrOfPrimitives % 5)==0) ProgressDialog->Update(int(TP.GetReadPosPercent()*100.0));
    }

    TP.AssertAndSkipToken("}");


    // "Post-process" the entity properties.
    if (EntityNr>0)
    {
        int Index=-1;

        // Set our origin from the "origin" property, then remove it from the properties list:
        // the origin is a special-case that is not defined by the EntityClassDefs.lua scripts.
        MapEntityT*   Ent =dynamic_cast<MapEntityT*>(this);
        EntPropertyT* Prop=FindProperty("origin", &Index);
        const bool    FoundOrigin=(Prop!=NULL);

        if (Ent!=NULL && Prop!=NULL)
        {
            Ent->SetOrigin(Prop->GetVector3f());
            m_Properties.RemoveAtAndKeepOrder(Index);
        }

/*      TODO: Turn this into a MapCheckDialogT issue!!
        // Convert terrain entities to real terrains (map files >10 don't contain terrain entities anymore).
        EntPropertyT* ClassName=Entity->FindProperty("classname");

        if (MapFileVersion<11 && ClassName!=NULL && ClassName->Value=="Terrain")
        {
            MapTerrainT*  Terrain=new MapTerrainT();
            EntPropertyT* HeightMap=Entity->FindProperty("heightmap_name");
            EntPropertyT* Material=Entity->FindProperty("material_name");

            if (HeightMap!=NULL) Terrain->LoadHeightData(m_MapDoc.GetGameConfig()->ModDir+"/"+HeightMap->Value);
            if (Material!=NULL) Terrain->SetMaterial(m_MapDoc.GetGameConfig()->GetMatMan().FindMaterial(Material->Value, true));

            Terrain->SetTerrainBounds(Entity->GetBB()); // Reset the terrains bounds to those of the entity.
            AddPrim(Terrain);
        }
*/

        // Set our class from the "classname" property, and remove it as well:
        // just like the "origin" property, it is a special case wrt. the EntityClassDefs.lua scripts.
        Prop=FindProperty("classname", &Index);

        if (Prop!=NULL && Prop->Value!="")
        {
            const wxString      ClassName  =Prop->Value;
            const EntityClassT* EntityClass=MapDoc.GetGameConfig()->FindClass(ClassName);

            SetClass(EntityClass!=NULL ? EntityClass : MapDoc.FindOrCreateUnknownClass(ClassName, FoundOrigin));
        }
        else
        {
            SetClass(MapDoc.FindOrCreateUnknownClass("undefined", FoundOrigin));
        }
    }

    // Remove our "classname" property, no matter which value it has.
    // For worlds, we've set our entity class to "worldspawn" in the constructor already, the map file cannot override this.
    // For entities, the proper class has been set above.
    int Index;
    if (FindProperty("classname", &Index)!=NULL)
        m_Properties.RemoveAtAndKeepOrder(Index);
}


void MapEntityBaseT::Save_cmap(const MapDocumentT& MapDoc, std::ostream& OutFile, unsigned long EntityNr, const BoundingBox3fT* Intersecting) const
{
    // If it's a solid custom entity but doesn't have any primitives, don't save it.
    if (EntityNr>0 && m_Class->IsSolidClass() && m_Primitives.Size()==0)
    {
        OutFile << "\n// Solid entity " << EntityNr << " of class " << m_Class->GetName() << " has no primitives - not saved.\n";
        return;
    }

    OutFile << "\n"
            << "{"; if (WriteComments) OutFile << " // Entity " << EntityNr; OutFile << "\n";

    // Save the groups info.
    MapElementT::Save_cmap(OutFile, EntityNr, MapDoc);

    // Save the properties.
    if (FindProperty("classname")==NULL)
    {
        EntPropertyT("classname", m_Class->GetName()).Save_cmap(OutFile);
    }

    for (unsigned long PropNr=0; PropNr<m_Properties.Size(); PropNr++)
        m_Properties[PropNr].Save_cmap(OutFile);

    if (EntityNr>0 && !m_Class->IsSolidClass())
    {
        const MapEntityT* Ent=dynamic_cast<const MapEntityT*>(this);

        // Note that convertToString(m_Origin) returns not "x y z" but "(x, y, z)".
        EntPropertyT("origin", serialize(Ent->GetOrigin())).Save_cmap(OutFile);
    }

    // Save the primitives.
    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
    {
        const MapPrimitiveT* Prim=m_Primitives[PrimNr];

        if (!Intersecting || Prim->GetBB().Intersects(*Intersecting))
        {
            Prim->Save_cmap(OutFile, PrimNr, MapDoc);
        }
    }

    OutFile << "}\n";
}
