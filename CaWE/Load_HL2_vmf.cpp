/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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
#include "GameConfig.hpp"
#include "Load_MatReplMan.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "MapEntity.hpp"
#include "MapFace.hpp"

#include "TextParser/TextParser.hpp"

#include "wx/wx.h"
#include "wx/progdlg.h"


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
void MapEntityBaseT::Load_HL2_vmf(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
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
                    wxLogWarning(wxString("Unknown sub-chunk \"")+SubChName+"\" in entity %lu.\n", EntityNr);
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
