/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompMapEntity.hpp"
#include "Load_MatReplMan.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "MapFace.hpp"

#include "../GameConfig.hpp"

#include "TextParser/TextParser.hpp"

#include "wx/wx.h"
#include "wx/progdlg.h"


using namespace MapEditor;


static MatReplaceManT* g_MatReplaceMan=NULL;


// This function reads a "side" chunk.
// The chunk name has already been read from TP, next token must be the opening "{".
MapFaceT MapFaceT::Create_HL2_vmf(TextParserT& TP, EditorMatManT& MatMan)
{
    MapFaceT Face;
    wxString MaterialName;

    Face.m_SurfaceInfo.TexCoordGenMode=PlaneProj;

    TP.AssertAndSkipToken("{");

    while (TP.PeekNextToken()!="}")
    {
        const std::string Key=TP.GetNextToken();

        if (Key=="plane")
        {
            TextParserT ValueTP(TP.GetNextToken().c_str(), "()", false);

            if (ValueTP.GetNextToken()!="(") throw TextParserT::ParseError();
            Face.m_PlanePoints[0].x=ValueTP.GetNextTokenAsFloat();
            Face.m_PlanePoints[0].y=ValueTP.GetNextTokenAsFloat();
            Face.m_PlanePoints[0].z=ValueTP.GetNextTokenAsFloat();
            if (ValueTP.GetNextToken()!=")") throw TextParserT::ParseError();

            if (ValueTP.GetNextToken()!="(") throw TextParserT::ParseError();
            Face.m_PlanePoints[1].x=ValueTP.GetNextTokenAsFloat();
            Face.m_PlanePoints[1].y=ValueTP.GetNextTokenAsFloat();
            Face.m_PlanePoints[1].z=ValueTP.GetNextTokenAsFloat();
            if (ValueTP.GetNextToken()!=")") throw TextParserT::ParseError();

            if (ValueTP.GetNextToken()!="(") throw TextParserT::ParseError();
            Face.m_PlanePoints[2].x=ValueTP.GetNextTokenAsFloat();
            Face.m_PlanePoints[2].y=ValueTP.GetNextTokenAsFloat();
            Face.m_PlanePoints[2].z=ValueTP.GetNextTokenAsFloat();
            if (ValueTP.GetNextToken()!=")") throw TextParserT::ParseError();

            Face.m_Plane=Plane3fT(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
        }
        else if (Key=="material")
        {
            MaterialName=g_MatReplaceMan->GetReplacement(TP.GetNextToken());
        }
        else if (Key=="uaxis")
        {
            TextParserT ValueTP(TP.GetNextToken().c_str(), "[]", false);

            if (ValueTP.GetNextToken()!="[") throw TextParserT::ParseError();
            Face.m_SurfaceInfo.UAxis.x=ValueTP.GetNextTokenAsFloat();
            Face.m_SurfaceInfo.UAxis.y=ValueTP.GetNextTokenAsFloat();
            Face.m_SurfaceInfo.UAxis.z=ValueTP.GetNextTokenAsFloat();
            Face.m_SurfaceInfo.Trans[0]=ValueTP.GetNextTokenAsFloat();
            if (ValueTP.GetNextToken()!="]") throw TextParserT::ParseError();

            // Should use the true texture width here, which is unfortunately impossible to know...
            Face.m_SurfaceInfo.Scale[0]=1.0f/(ValueTP.GetNextTokenAsFloat()*256.0f);
        }
        else if (Key=="vaxis")
        {
            TextParserT ValueTP(TP.GetNextToken().c_str(), "[]", false);

            if (ValueTP.GetNextToken()!="[") throw TextParserT::ParseError();
            Face.m_SurfaceInfo.VAxis.x=ValueTP.GetNextTokenAsFloat();
            Face.m_SurfaceInfo.VAxis.y=ValueTP.GetNextTokenAsFloat();
            Face.m_SurfaceInfo.VAxis.z=ValueTP.GetNextTokenAsFloat();
            Face.m_SurfaceInfo.Trans[1]=ValueTP.GetNextTokenAsFloat();
            if (ValueTP.GetNextToken()!="]") throw TextParserT::ParseError();

            // Should use the true texture height here, which is unfortunately impossible to know...
            Face.m_SurfaceInfo.Scale[1]=1.0f/(ValueTP.GetNextTokenAsFloat()*256.0f);
        }
        else if (Key=="rotation")
        {
            Face.m_SurfaceInfo.Rotate=TP.GetNextTokenAsFloat();
        }
        else
        {
            // It's a sub-chunk or a property that we don't know and thus knowingly don't handle.
            const std::string Value=TP.GetNextToken();

            if (Value=="{") TP.SkipBlock("{", "}", true);

            if (Key!="dispinfo" && Key!="id" && Key!="lightmapscale" && Key!="smoothing_groups")
            {
                // If Key has a value that I've never seen during development, log the occurrence.
                wxLogWarning(wxString("Unknown property or sub-chunk \"")+Key.c_str()+"\" in chunk \"side\".\n");
            }
        }
    }

    TP.AssertAndSkipToken("}");

    Face.m_Material=MatMan.FindMaterial(MaterialName, true /*return dummy if not found*/);
    return Face;
}


// This function reads a "solid" chunk.
// The chunk name has already been read from TP, next token must be the opening "{".
MapBrushT* MapBrushT::Create_HL2_vmf(TextParserT& TP, EditorMatManT& MatMan)
{
    MapBrushT* Brush=new MapBrushT();

    TP.AssertAndSkipToken("{");

    Brush->m_Faces.Clear();

    while (TP.PeekNextToken()!="}")
    {
        const std::string SubChName=TP.GetNextToken();

        if (SubChName=="side")
        {
            Brush->m_Faces.PushBack(MapFaceT::Create_HL2_vmf(TP, MatMan));
        }
        else
        {
            // It's a sub-chunk or a property that we don't know and thus knowingly don't handle.
            const std::string Value=TP.GetNextToken();

            if (Value=="{") TP.SkipBlock("{", "}", true);

            if (SubChName!="editor" && SubChName!="id")
            {
                // If Key has a value that I've never seen during development, log the occurrence.
                wxLogWarning(wxString("Unknown property or sub-chunk \"")+SubChName.c_str()+"\" in chunk \"solid\".\n");
            }
        }
    }

    TP.AssertAndSkipToken("}");

    Brush->CompleteFaceVertices();

    // Create the brush using the planes that were read from the vmf file.
    if (!Brush->IsValid())
    {
     // wxLogWarning(wxString::Format("Entity %lu, Primitive %lu: The brush could not be created from its planes.\n", EntityNr, PrimitiveNr));
        wxLogWarning(wxString::Format("A brush could not be created from its planes.\n"));
    }

    wxASSERT(Brush->IsValid());
    return Brush;
}


/// This method reads a "world" or "entity" chunk.
/// The chunk name has already been read from TP, next token must be the opening "{".
void CompMapEntityT::Load_HL2_vmf(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
{
    static MatReplaceManT MatReplaceMan("HL2", MapDoc.GetGameConfig()->GetMatMan().GetMaterials());

    if (EntityNr==0)
    {
        g_MatReplaceMan=&MatReplaceMan;
    }

    TP.AssertAndSkipToken("{");

    while (TP.PeekNextToken()!="}")
    {
        const std::string SubChName=TP.GetNextToken();

        if (SubChName=="solid")
        {
            if (TP.PeekNextToken()!="{")
            {
                // Yes, I've seen entities that contained lines like: "solid" "6"
                TP.GetNextToken();
                continue;
            }

            AddPrim(MapBrushT::Create_HL2_vmf(TP, MapDoc.GetGameConfig()->GetMatMan()));
        }
        else
        {
            // Ok, SubChName is likely the key of a property, and the next token is the value,
            // *unless* it's a sub-chunk that we don't know about.
            const std::string Value=TP.GetNextToken();

            if (Value=="{")
            {
                // It's a sub-chunk that we don't know and thus knowingly don't handle.
                // The best we can do is ignore (skip) it, but also record it,
                // because I'd like to know which chunk type I don't know.
                TP.SkipBlock("{", "}", true);

                if (SubChName!="editor" && SubChName!="group" && SubChName!="connections")
                {
                    wxLogWarning(wxString("Unknown sub-chunk \"")+SubChName+"\" in entity %lu.\n", EntityNr);
                }
            }
            else
            {
                // Add the property to the entity.
                if (EntityNr>0 && SubChName=="angle")
                {
                    // Convert an "angle" key into a triple of "angles".
                    // (Do "angle" keys actually still occur in VMF files btw.??)
                         if (Value=="-1") m_Properties.PushBack(EntPropertyT("angles", "-90 0 0"));
                    else if (Value=="-2") m_Properties.PushBack(EntPropertyT("angles", "90 0 0"));
                    else                  m_Properties.PushBack(EntPropertyT("angles", "0 "+Value+" 0"));
                }
                else
                {
                    m_Properties.PushBack(EntPropertyT(SubChName, Value));
                }
            }
        }

        if (ProgressDialog!=NULL) ProgressDialog->Update(int(TP.GetReadPosPercent()*100.0));
    }

    TP.AssertAndSkipToken("}");
}
