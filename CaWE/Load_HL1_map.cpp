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

#include "EditorMaterial.hpp"
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


static unsigned long   MapFileVersion=0;
static MatReplaceManT* g_MatReplaceMan=NULL;


MapFaceT MapFaceT::Create_HL1_map(TextParserT& TP, EditorMatManT& MatMan)
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

    Face.m_Plane   =Plane3fT(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
    Face.m_Material=MatMan.FindMaterial(g_MatReplaceMan->GetReplacement(TP.GetNextToken()), true /*return dummy if not found*/);
    Face.m_SurfaceInfo.TexCoordGenMode=PlaneProj;

    TP.AssertAndSkipToken("[");
    Face.m_SurfaceInfo.UAxis.x=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.UAxis.y=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.UAxis.z=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.Trans[0]=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken("]");

    TP.AssertAndSkipToken("[");
    Face.m_SurfaceInfo.VAxis.x=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.VAxis.y=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.VAxis.z=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.Trans[1]=TP.GetNextTokenAsFloat();
    TP.AssertAndSkipToken("]");

    Face.m_SurfaceInfo.Rotate=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.Scale[0]=TP.GetNextTokenAsFloat();
    Face.m_SurfaceInfo.Scale[1]=TP.GetNextTokenAsFloat();

    TP.GetNextToken();  // Some old Q2 flags.
    TP.GetNextToken();  // Some old Q2 flags.

    if (TP.PeekNextToken()!="(" &&      // Next token doesn't indicate another face definition.
        TP.PeekNextToken()!="}")        // Next token doesn't indicate end of brush definition.
    {
        // Optional lightmap scale - intentionally ignored, we rather stay with our default value.
        TP.GetNextToken();
    }

    Face.m_SurfaceInfo.Scale[0]=1.0/(Face.m_SurfaceInfo.Scale[0]*Face.m_Material->GetWidth());
    Face.m_SurfaceInfo.Scale[1]=1.0/(Face.m_SurfaceInfo.Scale[1]*Face.m_Material->GetHeight());

    Face.m_SurfaceInfo.Trans[0]/=Face.m_Material->GetWidth();
    Face.m_SurfaceInfo.Trans[1]/=Face.m_Material->GetHeight();

    return Face;
}


MapBrushT* MapBrushT::Create_HL1_map(TextParserT& TP, unsigned long EntityNr, unsigned long BrushNr, EditorMatManT& MatMan)
{
    // if (MapFileVersion<220) throw ParseErrorT;

    MapBrushT* Brush=new MapBrushT();

    TP.AssertAndSkipToken("{");

    while (TP.PeekNextToken()=="(")
    {
        Brush->m_Faces.PushBack(MapFaceT::Create_HL1_map(TP, MatMan));
    }

    TP.AssertAndSkipToken("}");

    Brush->CompleteFaceVertices();

    if (!Brush->IsValid())
        wxLogWarning("Entity %lu, primitive %lu: The brush could not be created from its planes.\n", EntityNr, BrushNr);

    wxASSERT(Brush->IsValid());
    return Brush;
}


void MapEntityBaseT::Load_HL1_map(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
{
    static MatReplaceManT MatReplaceMan("HL1", MapDoc.GetGameConfig()->GetMatMan().GetMaterials());

    if (EntityNr==0)
    {
        g_MatReplaceMan=&MatReplaceMan;
        MapFileVersion=0;
    }

    unsigned long NrOfPrimitives=0;

    TP.AssertAndSkipToken("{");

    while (true)
    {
        const std::string Token=TP.PeekNextToken();

        if (Token=="}")
        {
            // End of entity definition.
            break;
        }
        else if (Token=="{")
        {
            // A brush definition.
            AddPrim(MapBrushT::Create_HL1_map(TP, EntityNr, NrOfPrimitives, MapDoc.GetGameConfig()->GetMatMan()));
            NrOfPrimitives++;
        }
        else
        {
            // A property definition.
            std::string Key  =TP.GetNextToken();
            std::string Value=TP.GetNextToken();

            if (EntityNr==0)
            {
                if (Key=="mapversion") MapFileVersion=atoi(Value.c_str());
                if (Key=="wad")        continue;
            }
            else
            {
                if (Key=="angle")
                {
                    Key="angles";

                         if (Value=="-1") Value="-90 0 0";
                    else if (Value=="-2") Value="90 0 0";
                    else                  Value="0 "+Value+" 0";
                }
            }

            // Add the property to the entity.
            // TODO: Is any special action required when Key=="angle(s)"??  See other file format loaders for a reference...
            m_Properties.PushBack(EntPropertyT(Key, Value));
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
