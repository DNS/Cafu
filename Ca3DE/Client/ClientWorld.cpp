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

#include "ClientWorld.hpp"
#include "../NetConst.hpp"
#include "../Both/EntityManager.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Network/Network.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "DebugLog.hpp"
#include "../../Games/BaseEntity.hpp"
#include "../../Games/Game.hpp"

#include <cassert>


CaClientWorldT::CaClientWorldT(const char* FileName, ModelManagerT& ModelMan, WorldT::ProgressFunctionT ProgressFunction, unsigned long OurEntityID_) /*throw (WorldT::LoadErrorT)*/
    : Ca3DEWorld(new Ca3DEWorldT(FileName, ModelMan, true, ProgressFunction)),
      EntityManager(*Ca3DEWorld->EntityManager),
      OurEntityID(OurEntityID_),
      ServerFrameNr(0xDEADBEAF),
      MAX_FRAMES(16) /*MUST BE POWER OF 2*/
{
    cf::GameSys::Game->Cl_LoadWorld(FileName, Ca3DEWorld->CollModel);

    ProgressFunction(-1.0f, "InitDrawing()");
    Ca3DEWorld->BspTree->InitDrawing();

    for (unsigned long EntityNr=0; EntityNr<Ca3DEWorld->GameEntities.Size(); EntityNr++)
        Ca3DEWorld->GameEntities[EntityNr]->BspTree->InitDrawing();

    Frames.PushBackEmpty(MAX_FRAMES);

    ProgressFunction(-1.0f, "Loading Materials");
    MatSys::Renderer->PreCache();
}


CaClientWorldT::~CaClientWorldT()
{
    delete Ca3DEWorld;                      // Deletes all game entities, and thus removes all entity objects from the games physics world.
    cf::GameSys::Game->Cl_UnloadWorld();    // Deletes the games physics world.
}


void CaClientWorldT::ReadEntityBaseLineMessage(NetDataT& InData)
{
    EntityManager.CreateNewEntityFromEntityBaseLineMessage(InData);
}


unsigned long CaClientWorldT::ReadServerFrameMessage(NetDataT& InData)
{
    FrameT        CurrentFrame;
    const FrameT* DeltaFrame;

    // Header der SC1_NewFrameInfo Message zu Ende lesen
    CurrentFrame.ServerFrameNr=InData.ReadLong();           // Frame for which the server is sending (delta) info
    CurrentFrame.DeltaFrameNr =InData.ReadLong();           // Frame to decompress against

    cf::LogDebug(net, "    CurrentFrame.ServerFrameNr==%lu", CurrentFrame.ServerFrameNr);
    cf::LogDebug(net, "    CurrentFrame.DeltaFrameNr ==%lu", CurrentFrame.DeltaFrameNr);

    ServerFrameNr=CurrentFrame.ServerFrameNr;

    if (CurrentFrame.DeltaFrameNr==0)
    {
        // Das Frame ist gegen die BaseLines komprimiert
        DeltaFrame=NULL;
        CurrentFrame.IsValid=true;
    }
    else
    {
        // Das Frame ist gegen ein anderes Frame (delta-)komprimiert
        DeltaFrame=&Frames[CurrentFrame.DeltaFrameNr & (MAX_FRAMES-1)];

        // Wir können nur dann richtig dekomprimieren, wenn das DeltaFrame damals gültig war (Ungültigkeit sollte hier niemals vorkommen!)
        // und es nicht zu alt ist (andernfalls wurde es schon mit einem jüngeren Frame überschrieben und ist daher nicht mehr verfügbar).
        if (DeltaFrame->IsValid && CurrentFrame.DeltaFrameNr==DeltaFrame->ServerFrameNr)
        {
            CurrentFrame.IsValid=true;
        }
        else
        {
            // Falls wir hier 'CurrentFrame.IsValid==false' haben, so heißt das, daß wir den Rest der Message lesen und ignorieren müssen,
            // denn er ist nicht verwertbar. Dazu arbeiten wir ganz normal mit dem ungültigen oder veralteten DeltaFrame, denn das CurrentFrame
            // ist ja eh ungültig. Danach müssen wir eine nicht-komprimierte (d.h. gegen die BaseLines komprimierte) Nachricht anfordern,
            // indem wir ganz am Ende dieser Funktion 0 anstatt 'ServerFrameNr' zurückgeben.
            EnqueueString("CLIENT WARNING: %s, L %u: %u %u %u!\n", __FILE__, __LINE__, DeltaFrame->IsValid, CurrentFrame.DeltaFrameNr, DeltaFrame->ServerFrameNr);
        }
    }


    unsigned long DeltaFrameIndex   =0;
    unsigned long DeltaFrameEntityID=0;

    if (DeltaFrame==NULL) DeltaFrameEntityID=0x99999999;
    else
    {
        if (DeltaFrameIndex>=DeltaFrame->EntityIDsInPVS.Size()) DeltaFrameEntityID=0x99999999;
                                                           else DeltaFrameEntityID=DeltaFrame->EntityIDsInPVS[DeltaFrameIndex];
    }

    // Read all 'SC1_EntityUpdate' and 'SC1_EntityRemove' messages.
    while (true)
    {
        if (InData.ReadPos>=InData.Data.Size()) break;              // InBuffer ist zu Ende
        if (InData.Data[InData.ReadPos]!=SC1_EntityUpdate &&
            InData.Data[InData.ReadPos]!=SC1_EntityRemove) break;   // Nächste Message ist keine 'SC1_EntityUpdate' oder 'SC1_EntityRemove' Message

        const char          Msg        =InData.ReadByte();
        const unsigned long NewEntityID=InData.ReadLong();

        cf::LogDebug(net, "    %s: NewEntityID==%lu", Msg==SC1_EntityUpdate ? "SC1_EntityUpdate" : "SC1_EntityRemove", NewEntityID);

        while (DeltaFrameEntityID<NewEntityID)
        {
            // Ein oder mehrere Entities vom DeltaFrame finden sich unverändert im CurrentFrame wieder,
            // für diese Entities hat der Server aber überhaupt keine Bits geschickt, d.h. noch nichtmal den Header!
            CurrentFrame.EntityIDsInPVS.PushBack(DeltaFrameEntityID);

            // Es geht nun also ganz um den Entity mit der ID 'DeltaFrameEntityID', von dem wir wissen, daß er im DeltaFrame vorkam.
            // Notwendig ist es, den Zustand dieses Entities vom DeltaFrame (Nummer CurrentFrame.DeltaFrameNr) in den Zustand des
            // CurrentFrames (Nummer CurrentFrame.ServerFrameNr) zu kopieren.
            CurrentFrame.IsValid&=      // Note that operator & doesn't short-circuit, like operator && does!
                EntityManager.ParseServerDeltaUpdateMessage(DeltaFrameEntityID, CurrentFrame.DeltaFrameNr, CurrentFrame.ServerFrameNr, NULL);

            DeltaFrameIndex++;
            if (DeltaFrameIndex>=DeltaFrame->EntityIDsInPVS.Size()) DeltaFrameEntityID=0x99999999;
                                                               else DeltaFrameEntityID=DeltaFrame->EntityIDsInPVS[DeltaFrameIndex];
        }

        // Ab hier gilt 'DeltaFrameEntityID>=NewEntityID'

        if (Msg==SC1_EntityRemove)
        {
            // Der Entity im DeltaFrame kommt im CurrentFrame nicht mehr vor
            if (DeltaFrameEntityID==NewEntityID)
            {
                DeltaFrameIndex++;
                if (DeltaFrameIndex>=DeltaFrame->EntityIDsInPVS.Size()) DeltaFrameEntityID=0x99999999;
                                                                   else DeltaFrameEntityID=DeltaFrame->EntityIDsInPVS[DeltaFrameIndex];
            }
            else
            {
                // Cannot remove an entity from the delta frame that wasn't in the delta frame in the first place.
                EnqueueString("CLIENT WARNING: %s, L %u: DeltaFrameEntityID!=NewEntityID on removal!\n", __FILE__, __LINE__);
            }

            continue;
        }

        if (DeltaFrameEntityID==NewEntityID)
        {
            const ArrayT<uint8_t> DeltaMessage=InData.ReadDMsg();

            // Der Entity vom DeltaFrame kommt auch im CurrentFrame vor, Änderungen ergeben sich aus der Delta-Dekompression bzgl. des DeltaFrame
            CurrentFrame.EntityIDsInPVS.PushBack(NewEntityID);

            CurrentFrame.IsValid&=      // Note that operator & doesn't short-circuit, like operator && does!
                EntityManager.ParseServerDeltaUpdateMessage(NewEntityID, CurrentFrame.DeltaFrameNr, CurrentFrame.ServerFrameNr, &DeltaMessage);

            DeltaFrameIndex++;
            if (DeltaFrameIndex>=DeltaFrame->EntityIDsInPVS.Size()) DeltaFrameEntityID=0x99999999;
                                                               else DeltaFrameEntityID=DeltaFrame->EntityIDsInPVS[DeltaFrameIndex];
            continue;
        }

        if (DeltaFrameEntityID>NewEntityID)
        {
            const ArrayT<uint8_t> DeltaMessage=InData.ReadDMsg();

            // Der Entity kommt im CurrentFrame neu dazu, delta'en bzgl. der BaseLine
            CurrentFrame.EntityIDsInPVS.PushBack(NewEntityID);
            // EnqueueString("Frame %lu, Entity mit ID %i kam hinzu.\n", CurrentFrame.ServerFrameNr, NewEntityID);

            CurrentFrame.IsValid&=      // Note that operator & doesn't short-circuit, like operator && does!
                EntityManager.ParseServerDeltaUpdateMessage(NewEntityID, 0, CurrentFrame.ServerFrameNr, &DeltaMessage);
            continue;
        }
    }

    // Entities, die im DeltaFrame noch 'übrig' sind, werden ins CurrentFrame übernommen
    while (DeltaFrameEntityID!=0x99999999)
    {
        // Gleicher Fall wie oben:
        // Ein oder mehrere Entities vom DeltaFrame finden sich unverändert im CurrentFrame wieder,
        // für diese Entities hat der Server aber überhaupt keine Bits geschickt, d.h. noch nichtmal den Header!
        CurrentFrame.EntityIDsInPVS.PushBack(DeltaFrameEntityID);

        CurrentFrame.IsValid&=      // Note that operator & doesn't short-circuit, like operator && does!
            EntityManager.ParseServerDeltaUpdateMessage(DeltaFrameEntityID, CurrentFrame.DeltaFrameNr, CurrentFrame.ServerFrameNr, NULL);

        DeltaFrameIndex++;
        if (DeltaFrameIndex>=DeltaFrame->EntityIDsInPVS.Size()) DeltaFrameEntityID=0x99999999;
                                                           else DeltaFrameEntityID=DeltaFrame->EntityIDsInPVS[DeltaFrameIndex];
    }

    // CurrentFrame speichern für die spätere Wiederverwendung
    Frames[ServerFrameNr & (MAX_FRAMES-1)]=CurrentFrame;

    // Falls das CurrentFrame die ganze Zeit nicht gültig war, müssen wir 0 zurückgeben,
    // um vom Server gegen die BaseLines komprimierte Messages zu bekommen (siehe oben)!
    if (!CurrentFrame.IsValid)
        EnqueueString("CLIENT INFO: CurrentFrame (%lu %lu) invalid, requesting baseline message.\n", CurrentFrame.ServerFrameNr, CurrentFrame.DeltaFrameNr);

    return CurrentFrame.IsValid ? ServerFrameNr : 0;
}


bool CaClientWorldT::OurEntity_Repredict(unsigned long RemoteLastIncomingSequenceNr, unsigned long LastOutgoingSequenceNr)
{
    return EntityManager.Repredict(OurEntityID, RemoteLastIncomingSequenceNr, LastOutgoingSequenceNr);
}


void CaClientWorldT::OurEntity_Predict(const PlayerCommandT& PlayerCommand, unsigned long OutgoingSequenceNr)
{
    EntityManager.Predict(OurEntityID, PlayerCommand, OutgoingSequenceNr);
}


bool CaClientWorldT::OurEntity_GetCamera(bool UsePredictedState, Vector3dT& Origin, unsigned short& Heading, unsigned short& Pitch, unsigned short& Bank) const
{
    return EntityManager.GetCamera(OurEntityID, UsePredictedState, Origin, Heading, Pitch, Bank);
}


void CaClientWorldT::ComputeBFSPath(const VectorT& Start, const VectorT& End)
{
#if 0   // TODO: Move this into the scene graph.
    if (Start.IsEqual(End, MapT::RoundEpsilon)) { BFS_TreePoints.Clear(); return; }

    unsigned long LeafNr;

    BFS_Tree.Clear();         for (LeafNr=0; LeafNr<Ca3DEWorld.Map.Leaves.Size(); LeafNr++) BFS_Tree.PushBack((unsigned long)-1);
    BFS_TreePoints.Clear();   for (LeafNr=0; LeafNr<Ca3DEWorld.Map.Leaves.Size(); LeafNr++) BFS_TreePoints.PushBack(VectorT());
    ArrayT<bool> BFS_Visited; for (LeafNr=0; LeafNr<Ca3DEWorld.Map.Leaves.Size(); LeafNr++) BFS_Visited.PushBack(false);

    ArrayT<unsigned long> ToDoList;
    ToDoList.PushBack(Ca3DEWorld.Map.WhatLeaf(Start));

    BFS_Visited[ToDoList[0]]=true;

    while (ToDoList.Size())
    {
        // Nimm das erste Element aus der ToDoList...
        unsigned long CurrentLeafNr=ToDoList[0];

        // ...und rücke alles eins runter
        for (LeafNr=0; LeafNr+1<ToDoList.Size(); LeafNr++) ToDoList[LeafNr]=ToDoList[LeafNr+1];
        ToDoList.DeleteBack();

        // Alle Nachbarn betrachten
        // OPTIMIZE: Das geht natürlich besser, wenn man einen Adjaceny-Graph hat!
        for (LeafNr=0; LeafNr<Ca3DEWorld.Map.Leaves.Size(); LeafNr++)
        {
            if (BFS_Visited[LeafNr] || !Ca3DEWorld.Map.Leaves[CurrentLeafNr].BB.GetEpsilonBox(MapT::RoundEpsilon).Intersects(Ca3DEWorld.Map.Leaves[LeafNr].BB)) continue;

            for (unsigned long Portal1Nr=0; Portal1Nr<Ca3DEWorld.Map.Leaves[CurrentLeafNr].Portals.Size(); Portal1Nr++)
                for (unsigned long Portal2Nr=0; Portal2Nr<Ca3DEWorld.Map.Leaves[LeafNr].Portals.Size(); Portal2Nr++)
                    if (Ca3DEWorld.Map.Leaves[CurrentLeafNr].Portals[Portal1Nr].Overlaps(Ca3DEWorld.Map.Leaves[LeafNr].Portals[Portal2Nr], false, MapT::RoundEpsilon))
                    {
                        BFS_Visited[LeafNr]=true;           // Nachbarn 'markieren',
                        BFS_Tree   [LeafNr]=CurrentLeafNr;  // Vorgänger von -1 auf CurrentLeaf setzen
                        ToDoList.PushBack(LeafNr);          // und in ToDoList aufnehmen.

                        // Als Zugabe wollen wir noch den Eintrittspunkt festhalten
                        ArrayT< Polygon3T<double> > NewPolys;
                        Ca3DEWorld.Map.Leaves[LeafNr].Portals[Portal2Nr].GetChoppedUpAlong(Ca3DEWorld.Map.Leaves[CurrentLeafNr].Portals[Portal1Nr], MapT::RoundEpsilon, NewPolys);

                        VectorT Center;
                        for (unsigned long VertexNr=0; VertexNr<NewPolys[NewPolys.Size()-1].Vertices.Size(); VertexNr++)
                            Center=Center+NewPolys[NewPolys.Size()-1].Vertices[VertexNr];
                        BFS_TreePoints[LeafNr]=scale(Center, 1.0/double(NewPolys[NewPolys.Size()-1].Vertices.Size()));

                        // Es wäre nicht schlimm, wenn ein Leaf mehrfach in der ToDoListe landet, aber sinnvoll ist es auch nicht
                        Portal1Nr=Ca3DEWorld.Map.Leaves[CurrentLeafNr].Portals.Size();
                        break;
                    }
        }
    }

    BFS_EndLeafNr=Ca3DEWorld.Map.WhatLeaf(End);
    EnqueueString("Path from (%f %f %f) to (%f %f %f) calculated.", Start.x, Start.y, Start.z, End.x, End.y, End.z);
#endif
}


void CaClientWorldT::Draw(float FrameTime, const Vector3dT& DrawOrigin, unsigned short DrawHeading, unsigned short DrawPitch, unsigned short DrawBank) const
{
    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT::GetRotateXMatrix(-90.0f));   // Start with the global Ca3DE coordinate system (not the OpenGL coordinate system).
    MatSys::Renderer->RotateY  (MatSys::RendererI::WORLD_TO_VIEW, -float(DrawBank   )*45.0f/8192.0f);    // *360/2^16
    MatSys::Renderer->RotateX  (MatSys::RendererI::WORLD_TO_VIEW,  float(DrawPitch  )*45.0f/8192.0f);
    MatSys::Renderer->RotateZ  (MatSys::RendererI::WORLD_TO_VIEW,  float(DrawHeading)*45.0f/8192.0f);


    // OBSOLETE: DrawableSkyDome.Draw();
#if SHL_ENABLED
    MoveSHLSun(FrameTime);
#endif
    MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -float(DrawOrigin.x), -float(DrawOrigin.y), -float(DrawOrigin.z));

#if 0   // TODO: Move this into the scene graph.
#ifdef DEBUG
    if (BFS_TreePoints.Size())
    {
        /* unsigned long CurrentLeaf=BFS_EndLeafNr;

        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINE_STRIP);
            while (CurrentLeaf!=(unsigned long)-1)
            {
                glVertex3d(BFS_TreePoints[CurrentLeaf].x, BFS_TreePoints[CurrentLeaf].y, BFS_TreePoints[CurrentLeaf].z);
                CurrentLeaf=BFS_Tree[CurrentLeaf];
            }
        glEnd();
        glEnable(GL_TEXTURE_2D); */
    }
#endif
#endif

    // Es gibt zwei Möglichkeiten, das PVS zu "disablen":
    // Entweder EntityManager.Draw() veranlassen, alle Entities des EngineEntities-Arrays zu zeichnen
    // (z.B. durch einen Trick, oder explizit ein Array der Größe EngineEntities.Size() übergeben, das an der Stelle i der Wert i hat),
    // oder indem die Beachtung des PVS auf Server-Seite (!) ausgeschaltet wird! Die Effekte sind jeweils verschieden!
    const FrameT& CurrentFrame=Frames[ServerFrameNr & (MAX_FRAMES-1)];

    static float TotalTime=0.0;
    TotalTime+=FrameTime;

    // Add a small offset to the z-component of the eye position, which adds a mild nice moving effect to the specular highlights.
    const float EyeOffsetZ=200.0f*sinf(TotalTime);

    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentEyePosition(float(DrawOrigin.x), float(DrawOrigin.y), float(DrawOrigin.z)+EyeOffsetZ);    // Also required in some ambient shaders.

    Ca3DEWorld->BspTree->DrawAmbientContrib(DrawOrigin);


    if (!CurrentFrame.IsValid)
    {
        // Eine Möglichkeit, wie man zu diesem Fehler kommt:
        // Bei einer World mit sehr vielen Entities, die auch alle vom Startpunkt aus sichtbar (d.h. im PVS) sind,
        // schickt uns der Server nach dem Join-Request die WorldInfo, die BaseLines, und auch die erste FrameInfo-Message.
        // Die FrameInfo-Message wird jedoch nur "unreliable" zu übertragen versucht, und daher vom Protokoll weggelassen,
        // wenn die max. Größe des Netzwerkpakets überschritten wird.
        // Somit können wir hierherkommen, ohne jemals eine FrameInfo-Message vom Server gesehen zu haben.
        // Erkennen kann man diesen Fall daran, daß 'ServerFrameNr' noch den Initialisierungswert 0xDEAFBEAF enthält.
        // Das Auftreten dieses Fehlers ist nicht schön, aber auch nicht sehr schlimm, solange es keine sauberere Lösung gibt.
#ifdef DEBUG
        EnqueueString("CLIENT WARNING: %s, L %u: Frame %lu was invalid on entity draw attempt!", __FILE__, __LINE__, ServerFrameNr);
#endif
        return;
    }

    // Draw the ambient contribution of the entities.
    EntityManager.DrawEntities(OurEntityID, false, DrawOrigin, CurrentFrame.EntityIDsInPVS);



    // Render the contribution of the point light sources (shadows, normal-maps, specular-maps).
    int LightSourceCount=0;

    for (unsigned long EntityIDNr=0; EntityIDNr<CurrentFrame.EntityIDsInPVS.Size(); EntityIDNr++)
    {
        unsigned long      LightColorDiffuse=0;
        unsigned long      LightColorSpecular=0;
        VectorT            LightPosition;
        float              LightRadius;
        bool               LightCastsShadows;
        const BaseEntityT* BaseEntity=EntityManager.GetBaseEntityByID(CurrentFrame.EntityIDsInPVS[EntityIDNr]);

        // The light source info is not taken from the BaseEntity directly because it yields the unpredicted light source position.
        // If once human player entities have no light source any more, we might get rid of the EntityManagerT::GetLightSourceInfo()
        // function chain again altogether.
        if (!BaseEntity) continue;
        // if (!BaseEntity->GetLightSourceInfo(LightColorDiffuse, LightColorSpecular, LightPosition, LightRadius)) continue;
        if (!EntityManager.GetLightSourceInfo(BaseEntity->ID, OurEntityID, LightColorDiffuse, LightColorSpecular, LightPosition, LightRadius, LightCastsShadows)) continue;
        if (!LightColorDiffuse && !LightColorSpecular) continue;

        // THIS IS *TEMPORARY* ONLY!
        // The purpose of limiting the number of light sources here is to compensate for the
        // severe problems with the stencil shadow code (fill-rate).
        static ConVarT MaxLights("cl_maxLights", 8, ConVarT::FLAG_MAIN_EXE, "Limits the number of simultaneously active dynamic lights.", 0, 255);

        if (LightSourceCount>=MaxLights.GetValueInt()) break;
        LightSourceCount++;


        MatSys::Renderer->SetCurrentLightSourcePosition(float(LightPosition.x), float(LightPosition.y), float(LightPosition.z));
        MatSys::Renderer->SetCurrentLightSourceRadius(LightRadius);
        MatSys::Renderer->SetCurrentLightSourceDiffuseColor (float(LightColorDiffuse  & 0xFF)/255.0f, float((LightColorDiffuse  >> 8) & 0xFF)/255.0f, float((LightColorDiffuse  >> 16) & 0xFF)/255.0f);
        MatSys::Renderer->SetCurrentLightSourceSpecularColor(float(LightColorSpecular & 0xFF)/255.0f, float((LightColorSpecular >> 8) & 0xFF)/255.0f, float((LightColorSpecular >> 16) & 0xFF)/255.0f);
     // MatSys::Renderer->SetCurrentEyePosition(...);   was already called above!


        // Render the stencil shadows.
        MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::STENCILSHADOW);     // Make sure thet stencil buffer is cleared.

        if (LightCastsShadows)
        {
            Ca3DEWorld->BspTree->DrawStencilShadowVolumes(LightPosition, LightRadius);

         // static ConVarT LocalPlayerStencilShadows("cl_LocalPlayerStencilShadows", false, ConVarT::FLAG_MAIN_EXE, "Whether the local player casts stencil shadows.");
         // if (LocalPlayerStencilShadows.GetValueBool())
         // {
                // Our entity casts shadows, except when the light source is he himself.
                EntityManager.DrawEntities(OurEntityID==BaseEntity->ID ? OurEntityID : 0xFFFFFFFF /* an ugly, dirty, kaum nachvollziehbarer hack */,
                                           OurEntityID==BaseEntity->ID,
                                           DrawOrigin,
                                           CurrentFrame.EntityIDsInPVS);
         // }
         // else
         // {
         //     // Our entity does not cast shadows at all, no matter if he himself or another entity is the light source.
         //     // ### In my last test, I did not observe any performance improvements with this, in comparison with the case above... ###
         //     EntityManager.DrawEntities(OurEntityID,
         //                                true,
         //                                DrawOrigin,
         //                                CurrentFrame.EntityIDsInPVS);
         // }
        }


        // Render the light-source dependent terms.
        MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::LIGHTING);

        Ca3DEWorld->BspTree->DrawLightSourceContrib(DrawOrigin, LightPosition);
        EntityManager.DrawEntities(OurEntityID,
                                   false,
                                   DrawOrigin,
                                   CurrentFrame.EntityIDsInPVS);
    }



    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);

    // Render translucent nodes back-to-front.
    Ca3DEWorld->BspTree->DrawTranslucentContrib(DrawOrigin);

    // Zuletzt halbtransparente HUD-Elemente, Fonts usw. zeichnen.
    EntityManager.PostDrawEntities(FrameTime, OurEntityID, CurrentFrame.EntityIDsInPVS);
}
