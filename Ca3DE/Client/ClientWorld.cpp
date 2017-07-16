/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ClientWorld.hpp"
#include "../EngineEntity.hpp"
#include "../NetConst.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "GameSys/CompLightPoint.hpp"
#include "GameSys/CompPlayerPhysics.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/Interpolator.hpp"
#include "GameSys/World.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Network/Network.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "DebugLog.hpp"
#include "../Common/CompGameEntity.hpp"

#include <cassert>


CaClientWorldT::CaClientWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, WorldT::ProgressFunctionT ProgressFunction, unsigned long OurEntityID_) /*throw (WorldT::LoadErrorT)*/
    : Ca3DEWorldT(FileName, ModelMan, GuiRes, true, ProgressFunction),
      OurEntityID(OurEntityID_),
      m_FrameInfos(),
      m_ServerFrameNr(0xDEADBEEF),
      m_PlayerCommands(),
      m_PlayerCommandNr(0)
{
    ProgressFunction(-1.0f, "InitDrawing()");

    for (unsigned int EntNr = 0; EntNr < m_World->m_StaticEntityData.Size(); EntNr++)
        m_World->m_StaticEntityData[EntNr]->m_BspTree->InitDrawing();

    m_FrameInfos.PushBackEmpty(16);         // The size MUST be a power of 2.
    m_PlayerCommands.PushBackEmpty(128);    // The size MUST be a power of 2.

    ProgressFunction(-1.0f, "Loading Materials");
    MatSys::Renderer->PreCache();
}


bool CaClientWorldT::ReadEntityBaseLineMessage(NetDataT& InData)
{
    const unsigned int EntID    = InData.ReadLong();
    const unsigned int ParentID = InData.ReadLong();


    // Create a new entity with the given EntID and add it to the specified parent.
    cf::GameSys::EntityCreateParamsT Params(*m_ScriptWorld);

    Params.ForceID(EntID);

    IntrusivePtrT<cf::GameSys::EntityT> Parent  = m_ScriptWorld->GetRootEntity()->FindID(ParentID);
    IntrusivePtrT<cf::GameSys::EntityT> NewEnt  = new cf::GameSys::EntityT(Params);
    IntrusivePtrT<CompGameEntityT>      GameEnt = new CompGameEntityT();

    NewEnt->SetApp(GameEnt);

    if (Parent.IsNull())
    {
        // Even though we handle it and try to continue here, this is actually a serious error that should never happen!
        EnqueueString("CLIENT ERROR: Parent entity not found!\n");
        m_ScriptWorld->GetRootEntity()->AddChild(NewEnt);
    }
    else
    {
        Parent->AddChild(NewEnt);
    }

    // Now that we load all the map entities both on the server *and* on the client (rather than transferring
    // them from the server to the client), shouldn't the following assert() always hold??!?
    // This in turn should remove (??, or at least relax?) the co-use of the entity ID as index into
    // m_World->m_StaticEntityData. See the comments in the `Ca3DEWorldT` and `cf::GameSys::WorldT` ctors for details!
    assert(EntID >= m_World->m_StaticEntityData.Size());

    // Falls notwendig, Platz für die neue EntID schaffen.
    while (m_EngineEntities.Size() <= EntID) m_EngineEntities.PushBack(NULL);

    // Die EntID könnte durchaus wiederverwendet werden - was immer der Server wünscht.
    delete m_EngineEntities[EntID];

    // Neuen Entity tatsächlich erschaffen.
    m_EngineEntities[EntID] = new EngineEntityT(NewEnt, InData);
    return true;
}


unsigned long CaClientWorldT::ReadServerFrameMessage(NetDataT& InData)
{
    // Finish reading the SC1_FrameInfo message.
    m_ServerFrameNr                           = InData.ReadLong();  // Frame for which the server is sending (delta) info.
    const unsigned long DeltaFrameNr          = InData.ReadLong();  // Frame to decompress against.
    const unsigned int  SvLastPlayerCommandNr = InData.ReadLong();  // The last player command that the server has received and accounted for in this frame.

    cf::LogDebug(net, "    m_ServerFrameNr == %lu", m_ServerFrameNr);
    cf::LogDebug(net, "    DeltaFrameNr    == %lu", DeltaFrameNr);

    FrameInfoT&       CurrentFrame = m_FrameInfos[m_ServerFrameNr & (m_FrameInfos.Size() - 1)];
    const FrameInfoT* DeltaFrame;

    CurrentFrame.IsValid       = false;
    CurrentFrame.ServerFrameNr = m_ServerFrameNr;
    CurrentFrame.EntityIDsInPVS.Overwrite();

    if (DeltaFrameNr == 0)
    {
        // Das Frame ist gegen die BaseLines komprimiert
        DeltaFrame=NULL;
        CurrentFrame.IsValid=true;
    }
    else
    {
        // Das Frame ist gegen ein anderes Frame (delta-)komprimiert
        DeltaFrame = &m_FrameInfos[DeltaFrameNr & (m_FrameInfos.Size() - 1)];

        // Wir können nur dann richtig dekomprimieren, wenn das DeltaFrame damals gültig war (Ungültigkeit sollte hier niemals vorkommen!)
        // und es nicht zu alt ist (andernfalls wurde es schon mit einem jüngeren Frame überschrieben und ist daher nicht mehr verfügbar).
        if (DeltaFrame->IsValid && DeltaFrameNr == DeltaFrame->ServerFrameNr)
        {
            CurrentFrame.IsValid=true;
        }
        else
        {
            // Falls wir hier 'CurrentFrame.IsValid==false' haben, so heißt das, daß wir den Rest der Message lesen und ignorieren müssen,
            // denn er ist nicht verwertbar. Dazu arbeiten wir ganz normal mit dem ungültigen oder veralteten DeltaFrame, denn das CurrentFrame
            // ist ja eh ungültig. Danach müssen wir eine nicht-komprimierte (d.h. gegen die BaseLines komprimierte) Nachricht anfordern,
            // indem wir ganz am Ende dieser Funktion 0 anstatt 'm_ServerFrameNr' zurückgeben.
            EnqueueString("CLIENT WARNING: %s, L %u: %u %u %u!\n", __FILE__, __LINE__, DeltaFrame->IsValid, DeltaFrameNr, DeltaFrame->ServerFrameNr);
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
                ParseServerDeltaUpdateMessage(DeltaFrameEntityID, DeltaFrameNr, CurrentFrame.ServerFrameNr, NULL);

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
                ParseServerDeltaUpdateMessage(NewEntityID, DeltaFrameNr, CurrentFrame.ServerFrameNr, &DeltaMessage);

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
                ParseServerDeltaUpdateMessage(NewEntityID, 0, CurrentFrame.ServerFrameNr, &DeltaMessage);
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
            ParseServerDeltaUpdateMessage(DeltaFrameEntityID, DeltaFrameNr, CurrentFrame.ServerFrameNr, NULL);

        DeltaFrameIndex++;
        if (DeltaFrameIndex>=DeltaFrame->EntityIDsInPVS.Size()) DeltaFrameEntityID=0x99999999;
                                                           else DeltaFrameEntityID=DeltaFrame->EntityIDsInPVS[DeltaFrameIndex];
    }

    // Falls das CurrentFrame die ganze Zeit nicht gültig war, müssen wir 0 zurückgeben,
    // um vom Server gegen die BaseLines komprimierte Messages zu bekommen (siehe oben)!
    if (!CurrentFrame.IsValid)
    {
        EnqueueString("CLIENT INFO: CurrentFrame (%lu %lu) invalid, requesting baseline message.\n", CurrentFrame.ServerFrameNr, DeltaFrameNr);
        return 0;
    }


    /*
     * Run the reprediction for our local player entity.
     *
     * Just as with any other entity that is in our PVS, our local player entity's state has
     * been updated to the state of the latest server frame. In this server frame, all player
     * commands up to `SvLastPlayerCommandNr` have been accounted for.
     *
     * Now re-apply all player commands that are newer than `SvLastPlayerCommandNr` to our
     * local player entity in order to update it to the most recent state; these are
     * (m_PlayerCommandNr - SvLastPlayerCommandNr) many.
     *
     * As we have to provide also the "previous" player command to each call of Predict(),
     * the first iteration of the loop needs access to the player command at
     * `SvLastPlayerCommandNr` as well, thus the `+ 1` for NeedNumPCs below.
     */
    if (OurEntityID < m_EngineEntities.Size() && m_EngineEntities[OurEntityID] != NULL)
    {
        const unsigned int NeedNumPCs = m_PlayerCommandNr - SvLastPlayerCommandNr + 1;

        if (NeedNumPCs <= m_PlayerCommands.Size())
        {
            // If we run on the same host as the server (in a single-player game or as the local client
            // with a non-dedicated server), network messages are normally delivered without delay. In
            // such cases, we ideally have `SvLastPlayerCommandNr == m_PlayerCommandNr` here, meaning
            // that (re-)prediction is not necessary and thus not applied.
            for (unsigned int Nr = SvLastPlayerCommandNr + 1; Nr <= m_PlayerCommandNr; Nr++)
            {
                m_EngineEntities[OurEntityID]->Predict(
                    m_PlayerCommands[(Nr - 1) & (m_PlayerCommands.Size() - 1)],
                    m_PlayerCommands[ Nr      & (m_PlayerCommands.Size() - 1)]);
            }

            // Let all interpolators know about the reprediction.
            for (unsigned int CompNr = 0; true; CompNr++)
            {
                IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = m_EngineEntities[OurEntityID]->GetEntity()->GetComponent(CompNr);

                if (Comp == NULL) break;

                for (unsigned int i = 0; i < Comp->GetInterpolators().Size(); i++)
                    Comp->GetInterpolators()[i]->UpdateAfterReprediction();
            }
        }
        else
        {
            EnqueueString("WARNING - Reprediction impossible: Last ack'ed PlayerCommand is too old (%u, %u)!\n", SvLastPlayerCommandNr, m_PlayerCommandNr);
        }
    }

    return m_ServerFrameNr;
}


void CaClientWorldT::OurEntity_Predict(const PlayerCommandT& PlayerCommand, unsigned int PlayerCommandNr)
{
    // Player command numbering starts at 1 for each newly loaded world.
    // (Note that we use `PlayerCommandNr - 1` below.)
    assert(PlayerCommandNr >= 1);

    // Store the PlayerCommand for the reprediction.
    m_PlayerCommands[PlayerCommandNr & (m_PlayerCommands.Size() - 1)] = PlayerCommand;
    m_PlayerCommandNr = PlayerCommandNr;

    if (OurEntityID<m_EngineEntities.Size())
        if (m_EngineEntities[OurEntityID]!=NULL)
        {
            const PlayerCommandT& PrevPlayerCommand = m_PlayerCommands[(PlayerCommandNr - 1) & (m_PlayerCommands.Size() - 1)];

            m_EngineEntities[OurEntityID]->Predict(
                PrevPlayerCommand,
                PlayerCommand);

            // Let all interpolators know about the prediction step.
            for (unsigned int CompNr = 0; true; CompNr++)
            {
                IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = m_EngineEntities[OurEntityID]->GetEntity()->GetComponent(CompNr);

                if (Comp == NULL) break;

                for (unsigned int i = 0; i < Comp->GetInterpolators().Size(); i++)
                    Comp->GetInterpolators()[i]->UpdateAfterPrediction();
            }
        }
}


IntrusivePtrT<const cf::GameSys::ComponentTransformT> CaClientWorldT::OurEntity_GetCamera() const
{
    if (OurEntityID >= m_EngineEntities.Size()) return NULL;
    if (m_EngineEntities[OurEntityID] == NULL) return NULL;
    if (m_EngineEntities[OurEntityID]->GetEntity()->GetChildren().Size() < 1) return NULL;

    return m_EngineEntities[OurEntityID]->GetEntity()->GetChildren()[0]->GetTransform();
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


void CaClientWorldT::Draw(float FrameTime) const
{
    // Give all coroutines in the script state a chance to run.
    m_ScriptState->RunPendingCoroutines(FrameTime);


    // Note that besides the canonic way in cf::SceneGraph::BspTreeNodeT, there are two other
    // options for "disabling" the PVS: Either have the server disregard the PVS in the first
    // place, or have DrawEntities() render all entities that are in the m_EngineEntities
    // array. The respective effects are subtly different.
    const FrameInfoT& CurrentFrame = m_FrameInfos[m_ServerFrameNr & (m_FrameInfos.Size() - 1)];

    if (!CurrentFrame.IsValid)
    {
        // How we can get here:
        // After our join request, the server sends us the WorldInfo, the BaseLines, and the
        // first FrameInfo message. FrameInfo messages however are only transferred "unreliably",
        // and may be omitted or lost, e.g. if the maximum size of a network packet is exceeded.
        // This in turn can happen if a world has many entities that are all visible from (in
        // the PVS of) the player's starting point.
        // Thus, it is possible getting here without ever having seen a FrameInfo message.
        // Besides `!CurrentFrame.IsValid`, in such cases also the `m_ServerFrameNr` still has
        // its initial value 0xDEADBEEF.
        #ifdef DEBUG
            EnqueueString("CLIENT WARNING: %s, L %u: Frame %lu was invalid on entity draw attempt!", __FILE__, __LINE__, m_ServerFrameNr);
        #endif

        return;
    }


    // An early return can only happen if we've not yet received the baselines after the
    // SC1_WorldInfo. But then we should not have received any SC1_FrameInfos either, and thus
    // have already returned above (because CurrentFrame is not valid).
    if (OurEntityID >= m_EngineEntities.Size()) return;
    if (m_EngineEntities[OurEntityID] == NULL) return;

    IntrusivePtrT<cf::GameSys::EntityT> OurEnt = m_EngineEntities[OurEntityID]->GetEntity();

    if (OurEnt->GetChildren().Size() < 1) return;

    IntrusivePtrT<const cf::GameSys::ComponentTransformT> CameraTrafo = OurEnt->GetChildren()[0]->GetTransform();


    // Set up the entity state for rendering the video frame:
    //   - advance the interpolations,
    //   - actually set the interpolated values and
    //   - advance and apply and client-side effects.
    for (unsigned int i = 0; i < CurrentFrame.EntityIDsInPVS.Size(); i++)
    {
        const unsigned int                  ID  = CurrentFrame.EntityIDsInPVS[i];
        IntrusivePtrT<cf::GameSys::EntityT> Ent = m_EngineEntities[ID]->GetEntity();

        // Note that no effort is made to inform components that their values may have changed.
        // For example, ComponentCollisionModelT components are not informed if the origin or
        // orientation in the entity's ComponentTransformT has changed (and consequently the
        // ClipModel in the ClipWorld is not updated).
        for (unsigned int CompNr = 0; true; CompNr++)
        {
            IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = Ent->GetComponent(CompNr);

            if (Comp == NULL) break;

            for (unsigned int i = 0; i < Comp->GetInterpolators().Size(); i++)
            {
                Comp->GetInterpolators()[i]->AdvanceTime(FrameTime);
                Comp->GetInterpolators()[i]->SetCurrentValue();
            }
        }

        Ent->OnClientFrame(FrameTime);
    }


    // Update the sound system listener.
    {
        IntrusivePtrT<const cf::GameSys::ComponentPlayerPhysicsT> CompPlayerPhysics = dynamic_pointer_cast<cf::GameSys::ComponentPlayerPhysicsT>(OurEnt->GetComponent("PlayerPhysics"));
        const cf::math::Matrix3x3fT                               CameraMat(CameraTrafo->GetQuatWS());

        SoundSystem->UpdateListener(
            CameraTrafo->GetOriginWS().AsVectorOfDouble(),
            CompPlayerPhysics != NULL ? CompPlayerPhysics->GetVelocity() : Vector3dT(),
            CameraMat.GetAxis(0), CameraMat.GetAxis(2));
    }


    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());

    // In the OpenGL default coordinate system, the camera looks along the negative z-axis.
    // However, in Cafu we have to have it look along the (positive) x-axis in order to have it comply to
    // the conventions defined with the cf::math::AnglesT<T> class. Only then can we meaningfully use the
    // cf::math::AnglesT<T> angles in order to get and set the orientation of camera entities.
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,
        MatrixT::GetRotateXMatrix(-90.0f) *     // See the comment above
        MatrixT::GetRotateZMatrix( 90.0f) *     // for these two lines.
        CameraTrafo->GetEntityToWorld().GetInverse());


#if SHL_ENABLED
    MoveSHLSun(FrameTime);
#endif

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


    static float TotalTime=0.0;
    TotalTime+=FrameTime;

    // Add a small offset to the z-component of the eye position, which adds a mild nice moving effect to the specular highlights.
    const float     EyeOffsetZ = 8.0f*sinf(TotalTime);
    const Vector3dT DrawOrigin = CameraTrafo->GetOriginWS().AsVectorOfDouble();

    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentEyePosition(float(DrawOrigin.x), float(DrawOrigin.y), float(DrawOrigin.z)+EyeOffsetZ);    // Also required in some ambient shaders.


    const cf::SceneGraph::BspTreeNodeT* BspTree = m_World->m_StaticEntityData[0]->m_BspTree;
    BspTree->DrawAmbientContrib(DrawOrigin);

    // Draw the ambient contribution of the entities.
    DrawEntities(OurEntityID, false, DrawOrigin, CurrentFrame.EntityIDsInPVS);


    // Render the contribution of the point light sources (shadows, normal-maps, specular-maps).
    int LightSourceCount=0;

    for (unsigned long EntityIDNr=0; EntityIDNr<CurrentFrame.EntityIDsInPVS.Size(); EntityIDNr++)
    {
        unsigned long LightColorDiffuse=0;
        unsigned long LightColorSpecular=0;
        VectorT       LightPosition;
        float         LightRadius = 0.0f;
        bool          LightCastsShadows = false;
        unsigned long LightEntID = CurrentFrame.EntityIDsInPVS[EntityIDNr];

        if (!GetLightSourceInfo(LightEntID, LightColorDiffuse, LightColorSpecular, LightPosition, LightRadius, LightCastsShadows)) continue;
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
            BspTree->DrawStencilShadowVolumes(LightPosition, LightRadius);

         // static ConVarT LocalPlayerStencilShadows("cl_LocalPlayerStencilShadows", false, ConVarT::FLAG_MAIN_EXE, "Whether the local player casts stencil shadows.");
         // if (LocalPlayerStencilShadows.GetValueBool())
         // {
                // Our entity casts shadows, except when the light source is he himself.
                DrawEntities(OurEntityID == LightEntID ? OurEntityID : 0xFFFFFFFF /* an ugly, dirty, kaum nachvollziehbarer hack */,
                             OurEntityID == LightEntID,
                             DrawOrigin,
                             CurrentFrame.EntityIDsInPVS);
         // }
         // else
         // {
         //     // Our entity does not cast shadows at all, no matter if he himself or another entity is the light source.
         //     // ### In my last test, I did not observe any performance improvements with this, in comparison with the case above... ###
         //     DrawEntities(OurEntityID,
         //                  true,
         //                  DrawOrigin,
         //                  CurrentFrame.EntityIDsInPVS);
         // }
        }


        // Render the light-source dependent terms.
        MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::LIGHTING);

        BspTree->DrawLightSourceContrib(DrawOrigin, LightPosition);
        DrawEntities(OurEntityID,
                     false,
                     DrawOrigin,
                     CurrentFrame.EntityIDsInPVS);
    }


    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);

    // Render translucent nodes back-to-front.
    BspTree->DrawTranslucentContrib(DrawOrigin);

    // This is a quite good place to deal with the ParticleEngine,
    // because we come here exactly once per frame, only after everything else has already been drawn,
    // and with the OpenGL modelview matrix set to world space.
    ParticleEngineMS::DrawParticles();
    ParticleEngineMS::MoveParticles(FrameTime);

    // Zuletzt halbtransparente HUD-Elemente, Fonts usw. zeichnen.
    PostDrawEntities(FrameTime, CurrentFrame.EntityIDsInPVS);


    // Restore the values as they were obtained from the last server frame.
    // This is important for the next prediction step of the local human player entity,
    // because only with these values can the prediction work as it does on the server.
    for (unsigned int i = 0; i < CurrentFrame.EntityIDsInPVS.Size(); i++)
    {
        const unsigned int                  ID  = CurrentFrame.EntityIDsInPVS[i];
        IntrusivePtrT<cf::GameSys::EntityT> Ent = m_EngineEntities[ID]->GetEntity();

        // Note that no effort is made to inform components that their values may have changed.
        // For example, ComponentCollisionModelT components are not informed if the origin or
        // orientation in the entity's ComponentTransformT has changed (and consequently the
        // ClipModel in the ClipWorld is not updated).
        for (unsigned int CompNr = 0; true; CompNr++)
        {
            IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = Ent->GetComponent(CompNr);

            if (Comp == NULL) break;

            for (unsigned int i = 0; i < Comp->GetInterpolators().Size(); i++)
                Comp->GetInterpolators()[i]->SetTargetValue();
        }
    }
}


bool CaClientWorldT::ParseServerDeltaUpdateMessage(unsigned long EntityID, unsigned long DeltaFrameNr, unsigned long ServerFrameNr, const ArrayT<uint8_t>* DeltaMessage)
{
    bool EntityIDIsOK=false;

    if (EntityID<m_EngineEntities.Size())
        if (m_EngineEntities[EntityID]!=NULL)
            EntityIDIsOK=true;

    if (!EntityIDIsOK)
    {
        // Gib Warnung aus. Aber nur, weil wir mit einer SC1_EntityUpdate Message nichts anfangen können, brauchen wir noch lange nicht zu disconnecten.
        // ONE reason for getting EntityID>=m_EngineEntities.Size() here is the way how baselines are sent:
        // When a client joins a level, there can be a LOT of entities. Usually, not all baselines of all entities fit into a single
        // realiable message at once, and thus the server sends them in batches, contained in subsequent realiable messages.
        // Between realiable messages however, the server sends also SC1_EntityUpdate messages.
        // These messages can already refer to entities that the client knows nothing about, because it has not yet seen the (reliable)
        // introductory baseline message, and so we get here.
        // I turn the "WARNING" into an "INFO", so that ordinary users get a better impression. ;)
        if (EntityID>=m_EngineEntities.Size()) EnqueueString("CLIENT INFO: %s, L %u: EntityID>=m_EngineEntities.Size()\n", __FILE__, __LINE__);
                                          else EnqueueString("CLIENT WARNING: %s, L %u: m_EngineEntities[EntityID]==NULL \n", __FILE__, __LINE__);
        EnqueueString("(EntityID==%u, m_EngineEntities.Size()==%u)\n", EntityID, m_EngineEntities.Size());
        return false;
    }

    // Gibt bei Scheitern Diagnose-Nachricht aus. Häufigster Grund für Scheitern dürfe eine zu alte DeltaFrameNr sein.
    // Der Calling-Code muß das erkennen und reagieren (durch Anfordern von nichtkomprimierten (gegen die BaseLine komprimierten) Messages).
    // Jedenfalls nicht Grund genug für ein Client-Disconnect.
    return m_EngineEntities[EntityID]->ParseServerDeltaUpdateMessage(DeltaFrameNr, ServerFrameNr, DeltaMessage);
}


bool CaClientWorldT::GetLightSourceInfo(unsigned long EntityID, unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    if (EntityID<m_EngineEntities.Size())
        if (m_EngineEntities[EntityID]!=NULL)
        {
            IntrusivePtrT<cf::GameSys::EntityT> Entity = m_EngineEntities[EntityID]->GetEntity();
            IntrusivePtrT<cf::GameSys::ComponentPointLightT> L = dynamic_pointer_cast<cf::GameSys::ComponentPointLightT>(Entity->GetComponent("PointLight"));

            if (L == NULL) return false;
            if (!L->IsOn()) return false;

            const Vector3fT Col = L->GetColor() * 255.0f;

            DiffuseColor  = (unsigned long)(Col.x) + ((unsigned long)(Col.y) << 8) + ((unsigned long)(Col.z) << 16);
            SpecularColor = DiffuseColor;
            Position      = Entity->GetTransform()->GetOriginWS().AsVectorOfDouble();
            Radius        = L->GetRadius();
            CastsShadows  = L->CastsShadows();

            return true;
        }

    return false;
}


void CaClientWorldT::DrawEntities(unsigned long OurEntityID, bool SkipOurEntity, const VectorT& ViewerPos, const ArrayT<unsigned long>& EntityIDs) const
{
    for (unsigned long IDNr = 0; IDNr < EntityIDs.Size(); IDNr++)
    {
        const unsigned long EntityID = EntityIDs[IDNr];

        if (EntityID < m_EngineEntities.Size())
            if (m_EngineEntities[EntityID] != NULL)
            {
                if (EntityID == OurEntityID && SkipOurEntity) continue;

                IntrusivePtrT<cf::GameSys::EntityT> Ent     = m_EngineEntities[EntityID]->GetEntity();
                const cf::SceneGraph::BspTreeNodeT* BspTree = GetGameEnt(Ent)->GetStaticEntityData()->m_BspTree;

                assert(EntityID == Ent->GetID());
                if (Ent->GetID() == 0) continue;    // Skip the world, it is handled as a special case in the caller code.

                MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
                MatSys::Renderer->PushLightingParameters();
                {
                    const MatrixT ModelToWorld = Ent->GetTransform()->GetEntityToWorld();

                    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, ModelToWorld);

                    // The lighting parameters are currently in world space, but must be given in model space.
                    const float*    PosL        = MatSys::Renderer->GetCurrentLightSourcePosition();
                    const Vector3fT LightPos    = ModelToWorld.InvXForm(Vector3fT(PosL));
                    const float     LightRadius = MatSys::Renderer->GetCurrentLightSourceRadius();

                    const float*    PosE   = MatSys::Renderer->GetCurrentEyePosition();
                    const Vector3fT EyePos = ModelToWorld.InvXForm(Vector3fT(PosE));

                    MatSys::Renderer->SetCurrentLightSourcePosition(LightPos.x, LightPos.y, LightPos.z);
                 // MatSys::Renderer->SetCurrentLightSourceRadius(LightRadius);
                    MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);

                    // Set the ambient light color for this entity.
                    // This is not a global, but rather a per-entity value that is derived from the lightmaps that are close to that entity.
                    const Vector3fT AmbientEntityLight = GetAmbientLightColorFromBB(
                        BoundingBox3dT(Vector3dT(-4, -4, -4), Vector3dT(4, 4, 4)), Ent->GetTransform()->GetOriginWS().AsVectorOfDouble());

                    MatSys::Renderer->SetCurrentAmbientLightColor(AmbientEntityLight.x, AmbientEntityLight.y, AmbientEntityLight.z);

                    // TODO: Move this into the `CompGameEntityT` (application) component?
                    // TODO: Is there a better way than `Nodes.Size() > 0` to check for empty (unusable) BSP trees??
                    if (BspTree && BspTree->Nodes.Size() > 0)
                    {
                        switch (MatSys::Renderer->GetCurrentRenderAction())
                        {
                            case MatSys::RendererI::AMBIENT:
                                BspTree->DrawAmbientContrib(EyePos.AsVectorOfDouble());
                                BspTree->DrawTranslucentContrib(EyePos.AsVectorOfDouble());
                                break;

                            case MatSys::RendererI::LIGHTING:
                                BspTree->DrawLightSourceContrib(EyePos.AsVectorOfDouble(), LightPos.AsVectorOfDouble());
                                break;

                            case MatSys::RendererI::STENCILSHADOW:
                                BspTree->DrawStencilShadowVolumes(LightPos.AsVectorOfDouble(), LightRadius);
                                break;
                        }
                    }

                    const bool FirstPersonView = (EntityID == OurEntityID) ||
                        (Ent->GetParent() != NULL && Ent->GetParent()->GetID() == OurEntityID);

                    Ent->RenderComponents(FirstPersonView,
                        length(ViewerPos.AsVectorOfFloat() - Ent->GetTransform()->GetOriginWS()));
                }
                MatSys::Renderer->PopLightingParameters();
                MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
            }
    }
}


void CaClientWorldT::PostDrawEntities(float FrameTime, const ArrayT<unsigned long>& EntityIDs) const
{
    for (unsigned long IDNr = 0; IDNr < EntityIDs.Size(); IDNr++)
    {
        const unsigned long EntityID    = EntityIDs[IDNr];
        const bool          FirstPerson = (EntityID == OurEntityID);

        if (EntityID < m_EngineEntities.Size() && m_EngineEntities[EntityID] != NULL)
        {
            IntrusivePtrT<cf::GameSys::EntityT> Ent = m_EngineEntities[EntityID]->GetEntity();

            for (unsigned int CompNr = 0; true; CompNr++)
            {
                IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = Ent->GetComponent(CompNr);

                if (Comp == NULL) break;

                Comp->PostRender(FirstPerson);
            }
        }
    }
}
