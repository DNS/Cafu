/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompMapEntity.hpp"
#include "Load_MatReplMan.hpp"
#include "MapDocument.hpp"
#include "MapFace.hpp"
#include "MapBrush.hpp"
#include "MapBezierPatch.hpp"

#include "../GameConfig.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"

#include "Math3D/Vector3.hpp"
#include "Math3D/Plane3.hpp"
#include "TextParser/TextParser.hpp"
#include "SceneGraph/LightMapMan.hpp"

#include "wx/wx.h"
#include "wx/progdlg.h"


using namespace MapEditor;


static MatReplaceManT* g_MatReplaceMan=NULL;


MapFaceT MapFaceT::Create_D3_map(TextParserT& TP, const Vector3fT& Origin, EditorMatManT& MatMan)
{
    MapFaceT Face;

    TP.AssertAndSkipToken("(");
    Face.m_Plane.Normal.x= TP.GetNextTokenAsFloat();
    Face.m_Plane.Normal.y= TP.GetNextTokenAsFloat();
    Face.m_Plane.Normal.z= TP.GetNextTokenAsFloat();
    Face.m_Plane.Dist    =-TP.GetNextTokenAsFloat();    // Yes, D3's plane equation has Dist on the other side of the = than mine.
    TP.AssertAndSkipToken(")");

    // Materials are normally plane projected on faces.
    Face.m_SurfaceInfo.TexCoordGenMode=PlaneProj;

    // Translate the Face.m_Plane according to Origin.
    // This is done by getting a point on the plane (e.g. the Stuetzvektor),
    // offsetting it by Origin, and then recomputing the Face.m_Plane.Dist.
    Face.m_Plane.Dist=dot(Face.m_Plane.Normal*Face.m_Plane.Dist + Origin, Face.m_Plane.Normal);


    Vector3fT Span1;
    Vector3fT Span2;

    Face.m_Plane.GetSpanVectorsByRotation(Span1, Span2);

    // Assert that the output is as expected.
    wxASSERT(fabs(dot(Face.m_Plane.Normal, Span1))<0.01);
    wxASSERT(fabs(dot(Face.m_Plane.Normal, Span2))<0.01);
    wxASSERT(fabs(dot(Span1, Span2))<0.01);

    Face.m_PlanePoints[0]=Face.m_Plane.Normal*Face.m_Plane.Dist;

#if 1
    // We need to find three linear independent points in Face.m_Plane, which is pretty straightforward given Span1 and Span2.
    Face.m_PlanePoints[1]=Face.m_PlanePoints[0]+Span1*1000.0;
    Face.m_PlanePoints[2]=Face.m_PlanePoints[0]+Span2*1000.0;
#else
    // This method does not rely on Span1 and Span2.
    Vector3dT AbsNorm(fabs(Normal.x), fabs(Normal.y), fabs(Normal.z));

    // Note that the ">=" (vs. ">") are very important here!
    if (AbsNorm.x>=AbsNorm.y && AbsNorm.x>=AbsNorm.z)
    {
        // The x-component has the biggest magnitude.
        Face.m_PlanePoints[1][1]=Face.m_PlanePoints[0][1]+1000;
        Face.m_PlanePoints[1][2]=Face.m_PlanePoints[0][2];
        Face.m_PlanePoints[1][0]=(Dist-Normal.y*Face.m_PlanePoints[1][1]-Normal.z*Face.m_PlanePoints[1][2])/Normal.x;

        Face.m_PlanePoints[2][1]=Face.m_PlanePoints[0][1];
        Face.m_PlanePoints[2][2]=Face.m_PlanePoints[0][2]+1000;
        Face.m_PlanePoints[2][0]=(Dist-Normal.y*Face.m_PlanePoints[2][1]-Normal.z*Face.m_PlanePoints[2][2])/Normal.x;
    }
    else if (AbsNorm.y>=AbsNorm.x && AbsNorm.y>=AbsNorm.z)
    {
        // The y-component has the biggest magnitude.
        Face.m_PlanePoints[1][0]=Face.m_PlanePoints[0][0]+1000;
        Face.m_PlanePoints[1][2]=Face.m_PlanePoints[0][2];
        Face.m_PlanePoints[1][1]=(Dist-Normal.x*Face.m_PlanePoints[1][0]-Normal.z*Face.m_PlanePoints[1][2])/Normal.y;

        Face.m_PlanePoints[2][0]=Face.m_PlanePoints[0][0];
        Face.m_PlanePoints[2][2]=Face.m_PlanePoints[0][2]+1000;
        Face.m_PlanePoints[2][1]=(Dist-Normal.x*Face.m_PlanePoints[2][0]-Normal.z*Face.m_PlanePoints[2][2])/Normal.y;
    }
    else
    {
        // The z-component has the biggest magnitude.
        Face.m_PlanePoints[1][0]=Face.m_PlanePoints[0][0]+1000;
        Face.m_PlanePoints[1][1]=Face.m_PlanePoints[0][1];
        Face.m_PlanePoints[1][2]=(Dist-Normal.x*Face.m_PlanePoints[1][0]-Normal.y*Face.m_PlanePoints[1][1])/Normal.z;

        Face.m_PlanePoints[2][0]=Face.m_PlanePoints[0][0];
        Face.m_PlanePoints[2][1]=Face.m_PlanePoints[0][1]+1000;
        Face.m_PlanePoints[2][2]=(Dist-Normal.x*Face.m_PlanePoints[2][0]-Normal.y*Face.m_PlanePoints[2][1])/Normal.z;
    }
#endif

#ifndef NDEBUG
    // Verify the correctness of the planepts!
    Plane3fT CheckPlane(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);

    wxASSERT(dot(CheckPlane.Normal, Face.m_Plane.Normal)>=0.999 && fabs(CheckPlane.Dist-Face.m_Plane.Dist)<0.1);
#endif


    // Read the 2x3 texture matrix.
    TP.AssertAndSkipToken("(");

    Vector3dT Row1;
    TP.AssertAndSkipToken("(");
    Row1.x=TP.GetNextTokenAsFloat();
    Row1.y=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.Trans[0]=TP.GetNextTokenAsFloat();     // Texture offset (translation) in U-direction.
    TP.AssertAndSkipToken(")");

    Vector3dT Row2;
    TP.AssertAndSkipToken("(");
    Row2.x=TP.GetNextTokenAsFloat();
    Row2.y=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.Trans[1]=TP.GetNextTokenAsFloat();     // Texture offset (translation) in V-direction.
    TP.AssertAndSkipToken(")");

    TP.AssertAndSkipToken(")");


    // Read the material name and the remaining three zeros.
    wxString MatName=TP.GetNextToken();

    MatName=g_MatReplaceMan->GetReplacement(MatName);

    if (TP.GetNextToken()!="0") throw TextParserT::ParseError();
    if (TP.GetNextToken()!="0") throw TextParserT::ParseError();
    if (TP.GetNextToken()!="0") throw TextParserT::ParseError();


    Face.m_SurfaceInfo.Rotate=0;     // Maybe that's one of the above trailing zeros?


    // Compute the scales from the axis lengths.
    Vector3fT UAxis_=Span1*Row1.x + Span2*Row1.y; float LenU=length(UAxis_);
    Vector3fT VAxis_=Span1*Row2.x + Span2*Row2.y; float LenV=length(VAxis_);

    if (LenU<0.000001 || LenV<0.000001)
    {
        // The Row1 or Row2 components might all be zero.
        // I've seen this for example with the Doom3 "textures/common/caulk" material.
        UAxis_=Span1; LenU=1.0;
        VAxis_=Span2; LenV=1.0;
    }

    Face.m_SurfaceInfo.Scale[0]=LenU;
    Face.m_SurfaceInfo.Scale[1]=LenV;

    // Normalize the axes lengths.
    Face.m_SurfaceInfo.UAxis=UAxis_/LenU;
    Face.m_SurfaceInfo.VAxis=VAxis_/LenV;

    Face.m_Material=MatMan.FindMaterial(MatName, true /*return dummy if not found*/);

    return Face;
}


MapBrushT* MapBrushT::Create_D3_map(TextParserT& TP, const Vector3fT& Origin, unsigned long EntityNr, unsigned long PrimitiveNr, EditorMatManT& MatMan)
{
    MapBrushT* Brush=new MapBrushT();

    if (TP.GetNextToken()!="{") throw TextParserT::ParseError();

    Brush->m_Faces.Clear();

    while (true)
    {
        std::string Token=TP.GetNextToken();
        TP.PutBack(Token);

        if (Token!="(") break;

        Brush->m_Faces.PushBack(MapFaceT::Create_D3_map(TP, Origin, MatMan));
    }

    if (TP.GetNextToken()!="}") throw TextParserT::ParseError();

    Brush->CompleteFaceVertices();

    if (!Brush->IsValid())
    {
        wxLogWarning("Entity %lu, primitive %lu: The brush could not be created from its planes.\n", EntityNr, PrimitiveNr);
    }

    wxASSERT(Brush->IsValid());
    return Brush;
}


void MapBezierPatchT::Load_D3_map(TextParserT& TP, unsigned long patchDef, EditorMatManT& MatMan)
{
    if (patchDef!=2 && patchDef!=3) throw TextParserT::ParseError();

    if (TP.GetNextToken()!="{") throw TextParserT::ParseError();

    wxString texname = TP.GetNextToken();
    texname=g_MatReplaceMan->GetReplacement(texname);
    SetMaterial(MatMan.FindMaterial(texname, true /*Create dummy if not found.*/));

    // Bezier patches have custom texture coordinates per default.
    SurfaceInfo.TexCoordGenMode=Custom;

    TP.AssertAndSkipToken("(");

    unsigned long width =TP.GetNextTokenAsInt();
    unsigned long height=TP.GetNextTokenAsInt();

    SetSize(width, height);

    // "patchDef3"s seem to *always* specify explicit number of subdivisions... will they obsolete or complement "patchDef2"s?
    SubdivsHorz=(patchDef==3) ? TP.GetNextTokenAsInt() : -1;
    SubdivsVert=(patchDef==3) ? TP.GetNextTokenAsInt() : -1;

    TP.GetNextToken();  // "Contents" (unused - skip).
    TP.GetNextToken();  // "Flags"    (unused - skip).
    TP.GetNextToken();  // "Value"    (unused - skip).

    TP.AssertAndSkipToken(")");


    TP.AssertAndSkipToken("(");

    // js todo: D3 patch loading: d3 patches and cawe patches are defined and saved differently, shoudl I revert to the D3 representation of a patch?
    for (unsigned long x=0; x<cv_Width; x++)
    {
        TP.AssertAndSkipToken("(");

        for (int y=int(cv_Height)-1; y>=0; y--)
        {
            TP.AssertAndSkipToken("(");

            Vector3fT Pos;
            Pos.x=TP.GetNextTokenAsFloat();
            Pos.y=TP.GetNextTokenAsFloat();
            Pos.z=TP.GetNextTokenAsFloat();
            SetCvPos(x, y, Pos);

            Vector3fT TexCoord;
            TexCoord.x=TP.GetNextTokenAsFloat();
            TexCoord.y=TP.GetNextTokenAsFloat();
            SetCvUV(x, y, TexCoord);

            TP.AssertAndSkipToken(")");
        }

        TP.AssertAndSkipToken(")");
    }

    TP.AssertAndSkipToken(")");
    if (TP.GetNextToken()!="}") throw TextParserT::ParseError();
}


void CompMapEntityT::Load_D3_map(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
{
    EditorMatManT& MatMan=MapDoc.GetGameConfig()->GetMatMan();
    static MatReplaceManT MatReplaceMan("Doom 3", MatMan.GetMaterials());

    if (EntityNr==0)
    {
        g_MatReplaceMan=&MatReplaceMan;

        // See if the Doom3 map file begins with "Version 2".
        if (TP.GetNextToken()!="Version") throw TextParserT::ParseError();
        const int MapFileVersion=TP.GetNextTokenAsInt();

        if (MapFileVersion!=2)
        {
            wxMessageBox(wxString::Format("Expected Doom3 map file version 2, but found version %i.", MapFileVersion), "Could not import the D3 map file.");
            throw TextParserT::ParseError();
        }
    }

    TP.AssertAndSkipToken("{");

    while (true)
    {
        std::string Token=TP.GetNextToken();
        TP.PutBack(Token);

        if (Token=="{") break;  // This is the beginning of a primitive - done reading properties.
        if (Token=="}") break;  // This is the end of the entity - done reading properties, too.

        std::string Key  =TP.GetNextToken();
        std::string Value=TP.GetNextToken();

        if (Key=="angle")
        {
            Key="angles";

                 if (Value=="-1") Value="-90 0 0";
            else if (Value=="-2") Value="90 0 0";
            else                  Value="0 "+Value+" 0";
        }

        // Add the property to the entity.
        m_Properties.PushBack(EntPropertyT(Key, Value));
    }

    Vector3fT     OriginVec;
    EntPropertyT* OriginProp=FindProperty("origin");

    if (OriginProp) OriginVec=OriginProp->GetVector3f();

    unsigned long NrOfPrims=0;

    while (true)
    {
        std::string Token=TP.GetNextToken();

        if (Token!="{")
        {
            TP.PutBack(Token);
            break;
        }

        // The "{" signals the begin of a primitive.
        Token=TP.GetNextToken();

        if (Token=="brushDef3")
        {
            // It seems that in D3 map files, brushes are offset by the "origin" property of the entity they are in.
            AddPrim(MapBrushT::Create_D3_map(TP, OriginVec, EntityNr, NrOfPrims, MatMan));
        }
        else if (Token=="patchDef2")
        {
            MapBezierPatchT* BP=new MapBezierPatchT(MatMan.GetDefaultMaterial(), MapDoc.GetLightMapMan());

            BP->Load_D3_map(TP, 2, MatMan);
            AddPrim(BP);
        }
        else if (Token=="patchDef3")
        {
            MapBezierPatchT* BP=new MapBezierPatchT(MatMan.GetDefaultMaterial(), MapDoc.GetLightMapMan());

            BP->Load_D3_map(TP, 3, MatMan);
            AddPrim(BP);
        }
        else throw TextParserT::ParseError();

        // The "}" signals the end of the primitive.
        if (TP.GetNextToken()!="}") throw TextParserT::ParseError();


        if (ProgressDialog!=NULL && (NrOfPrims % 5)==0) ProgressDialog->Update(int(TP.GetReadPosPercent()*100.0));

        NrOfPrims++;
    }

    TP.AssertAndSkipToken("}");
}
