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

#include "FuncDoor.hpp"
#include "EntityCreateParams.hpp"
#include "Interpolator.hpp"
#include "../../GameWorld.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "SceneGraph/Node.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace GAME_NAME;


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off "warning C4355: 'this' : used in base member initializer list".
#pragma warning(disable:4355)
#endif


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntFuncDoorT::GetType() const
{
    return &TypeInfo;
 // return &EntFuncDoorT::TypeInfo;
}

void* EntFuncDoorT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntFuncDoorT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const luaL_Reg EntFuncDoorT::MethodsList[]=
{
    { "SetOrigin", EntFuncDoorT::SetOrigin },
 // { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT EntFuncDoorT::TypeInfo(GetBaseEntTIM(), "EntFuncDoorT", "BaseEntityT", CreateInstance, MethodsList);


EntFuncDoorT::EntFuncDoorT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  Params.RootNode->GetBoundingBox(),
                  0),
      InfraredCollMdl(NULL),
      InfraredClipMdl(GameWorld->GetClipWorld()),
      DoorState(Closed),
      OpenPos(),    // Properly initialized below.
      ClosedPos(m_Origin),
      MoveTime(GetProp("moveTime", 1.0f)),
      MoveFraction(0),
      OpenTime(GetProp("openTime", 5.0f)),
      OpenTimeLeft(0),
      TeamHead(this),
      TeamNext(NULL),
      RootNode(Params.RootNode)
{
    Register(new InterpolatorT<Vector3dT>(m_Origin));
    ClipModel.Register();   // The solid door itself.


    // See if we are part of a doors team.
    const std::string TeamName=GetProp("team", "");

    if (TeamName!="")
    {
        const ArrayT<unsigned long>& AllEntityIDs=GameWorld->GetAllEntityIDs();

        for (unsigned long EntityIDNr=0; EntityIDNr<AllEntityIDs.Size(); EntityIDNr++)
        {
            IntrusivePtrT<GameEntityI> Entity=GameWorld->GetGameEntityByID(AllEntityIDs[EntityIDNr]);

            if (Entity==NULL) continue;
            if (Entity->GetType()!=&TypeInfo) continue;

            const IntrusivePtrT<EntFuncDoorT> Door=static_pointer_cast<EntFuncDoorT>(Entity);

            if (Door->GetProp("team", "")==TeamName)
            {
                // Ok, we are in the same team. Add ourselves to the linked list.
                TeamHead=Door->TeamHead;
                TeamNext=TeamHead->TeamNext;
                TeamHead->TeamNext=this;
                break;
            }
        }
    }


    // FIXME: Both RootNode->GetBoundingBox() as well as m_Dimensions both have a "padding" of 100 units at each side!?!?!
    BoundingBox3dT InfraredBB=ClipModel.GetAbsoluteBB();

    const Vector3dT DoorSize  =InfraredBB.Max-InfraredBB.Min;
    const Vector3dT MoveDir   =normalizeOr0(GetProp("moveDir", Vector3dT(0, 0, 1)));
    const Vector3dT AbsMoveDir=Vector3dT(fabs(MoveDir.x), fabs(MoveDir.y), fabs(MoveDir.z));
    const double    lip       =GetProp("lip", 0.0);

    // Console->Print(convertToString(DoorSize)+cf::va("  %f \n", lip));
    OpenPos=ClosedPos+MoveDir*(dot(DoorSize, AbsMoveDir)-lip);    // Determine the final OpenPos.

    unsigned long SmallestSideNr =0;
    double        SmallestSideLen=DoorSize[0];

    for (unsigned long SideNr=1; SideNr<=2; SideNr++)
        if (DoorSize[SideNr]<SmallestSideLen)
        {
            SmallestSideNr =SideNr;
            SmallestSideLen=DoorSize[SideNr];
        }

    // Adjust the size of the InfraredBB. Its cross-section is as large as the BB of the door itself;
    // now extend it on the "front" and "back" of the door by "triggerPadding", the value that determines how far the "IR sensor reaches".
    const double Padding=fabs(GetProp("triggerPadding", 100.0));

    InfraredBB.Min[SmallestSideNr]-=Padding;
    InfraredBB.Max[SmallestSideNr]+=Padding;

    // Center the InfraredBB around the origin.
    const Vector3dT InfraredCenter=InfraredBB.GetCenter();

    InfraredBB.Max-=InfraredCenter;
    InfraredBB.Min-=InfraredCenter;

    InfraredCollMdl=cf::ClipSys::CollModelMan->GetCM(InfraredBB, MaterialManager->GetMaterial("Textures/meta/trigger"));

    InfraredClipMdl.SetCollisionModel(InfraredCollMdl);
    InfraredClipMdl.SetOrigin(InfraredCenter);
    InfraredClipMdl.SetUserData(this);
    InfraredClipMdl.Register();
}


EntFuncDoorT::~EntFuncDoorT()
{
    // Remove us from our doors team.
    if (this==TeamHead)
    {
        for (EntFuncDoorT* TeamMate=this->TeamNext; TeamMate!=NULL; TeamMate=TeamMate->TeamNext)
        {
            TeamMate->TeamHead=this->TeamNext;
        }
    }
    else
    {
        for (EntFuncDoorT* TeamMate=TeamHead; TeamMate!=NULL; TeamMate=TeamMate->TeamNext)
        {
            if (TeamMate->TeamNext==this)
            {
                TeamMate->TeamNext=this->TeamNext;
                break;
            }
        }
    }


    InfraredClipMdl.SetCollisionModel(NULL);
    InfraredClipMdl.SetUserData(NULL);

    cf::ClipSys::CollModelMan->FreeCM(InfraredCollMdl);
}


void EntFuncDoorT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();
}


void EntFuncDoorT::UpdateMovePos(float MoveFraction_)
{
    MoveFraction=MoveFraction_;

    m_Origin=OpenPos*MoveFraction + ClosedPos*(1.0f-MoveFraction);
    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();  // Re-register ourselves with the clip world.
}


void EntFuncDoorT::Think(float FrameTime, unsigned long ServerFrameNr)
{
    if (this!=TeamHead)
    {
        // Just let the team leader do all the thinking for us.
        return;
    }


    switch (DoorState)
    {
        case Closed:
            // We do nothing here until we're triggered.
            break;

        case Opening:
            MoveFraction+=FrameTime/MoveTime;

            if (MoveFraction>=1.0f)
            {
                MoveFraction=1.0f;
                OpenTimeLeft=OpenTime;
                DoorState=Open;
            }

            // Instruct the entire team to update the position.
            for (EntFuncDoorT* TeamMate=TeamHead; TeamMate!=NULL; TeamMate=TeamMate->TeamNext) TeamMate->UpdateMovePos(MoveFraction);
            break;

        case Open:
            // OpenTime being -1 (or generally negative) means that we stay open forever (or at least until being explicitly triggered again).
            if (OpenTime>=0)
            {
                OpenTimeLeft-=FrameTime;
                if (OpenTimeLeft<0.0f) DoorState=Closing;
            }
            break;

        case Closing:
            MoveFraction-=FrameTime/MoveTime;

            if (MoveFraction<=0.0f)
            {
                MoveFraction=0.0f;
                DoorState=Closed;
            }

            // Instruct the entire team to update the position.
            for (EntFuncDoorT* TeamMate=TeamHead; TeamMate!=NULL; TeamMate=TeamMate->TeamNext) TeamMate->UpdateMovePos(MoveFraction);
            break;
    }

    // If the team mates were interested...
 // for (EntFuncDoorT* TeamMate=TeamHead; TeamMate!=NULL; TeamMate=TeamMate->TeamNext) TeamMate->DoorState=DoorState;


    /* if (TranslationLinTimeLeft>0.0f)
    {
        const cf::ClipSys::TraceModelT    tm(CollisionModel->GetBoundingBox().AsBoxOfFloat());
        ArrayT<cf::ClipSys::ContactInfoT> Contacts;
        ArrayT<cf::ClipSys::ClipModelT*>  ClipModels;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////
        // This is a BAD BAD HACK.
        // It makes sure that memory is only allocated here in this DLL, not in the engine,
        // because calling Contacts.PushBack() in the engine causes the call to the destructor to crash,
        // because the destructor tries to free memory that was never allocated in the heap of the DLL.
        Contacts.PushBackEmpty(64);
        Contacts.Overwrite();
        /////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Find out if anything touches us.
        // Some of these objects might want to ride with us.
        GameWorld->GetClipWorld()->GetContacts(Contacts, ClipModels,
            m_Origin.AsVectorOfFloat(), (TranslationDest-TranslationSource).AsVectorOfFloat(), FrameTime/TranslationLinTimeTotal, tm, cf::math::Matrix3x3T<float>::Identity,
            MaterialT::Clip_Players | MaterialT::Clip_Monsters | MaterialT::Clip_Moveables, &ClipModel);

        ArrayT<BaseEntityT*> AlreadySeen;   // List of entities we have already dealt with.

        for (unsigned long ContactNr=0; ContactNr<Contacts.Size(); ContactNr++)
        {
            BaseEntityT* ContactEntity=static_cast<BaseEntityT*>(ClipModels[ContactNr]->GetUserData());

            // If we have a contact with a clip model with unknown owner entity, just ignore it.
            if (ContactEntity==NULL) continue;

            // Right now, there is no need to discriminate entities by type here (which ones should be pushed, which ones shouldn't?),
            // because of the contents flags (Clip_Players | Clip_Monsters | ...) that have been passed to the GetContacts() method above.
            // Well, maybe later...?
            // if (ContactEntity->GetType()==...) continue;

            // Here is such a case: movers don't push each other, no matter what!
            // Without this check, the two parts of the double slide door near the TechDemo map start are pushing each other
            // when they are both moving "down", into the ground.
            if (ContactEntity->GetType()==this->GetType()) continue;

            // Process each entity only once.
            if (AlreadySeen.Find(ContactEntity)>=0) continue;
            AlreadySeen.PushBack(ContactEntity);

            ArrayT<BaseEntityT*> Pushers;
            Pushers.PushBack(this);

            ContactEntity->OnPush(Pushers, (TranslationDest-TranslationSource)*(FrameTime/TranslationLinTimeTotal));
        }


        TranslationLinTimeLeft-=FrameTime;
        if (TranslationLinTimeLeft<0.0f) TranslationLinTimeLeft=0.0f;

        const float t=TranslationLinTimeLeft/TranslationLinTimeTotal;

        m_Origin=TranslationSource*t + TranslationDest*(1.0f-t);

        ClipModel.SetOrigin(m_Origin);
        ClipModel.Register();  // Re-register ourselves with the clip world.
    } */
}


void EntFuncDoorT::OnTrigger(BaseEntityT* Activator)
{
    if (this!=TeamHead)
    {
        // Notify the team head/master/leader that we have been triggered.
        TeamHead->OnTrigger(Activator);
        return;
    }

    // Ok, we're the team leader, update the DoorState.
    switch (DoorState)
    {
        case Closed:
            DoorState=Opening;
            break;

        case Opening:
        case Open:
            // If we're currently opening or open anyway, just do nothing.
            break;

        case Closing:
            DoorState=Opening;
            break;
    }
}


void EntFuncDoorT::Draw(bool FirstPersonView, float LodDist) const
{
    Vector3dT EyePos(MatSys::Renderer->GetCurrentEyePosition());
    Vector3dT LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    float     LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();


    // BIG PROBLEM:
    // The EyePos as obtained from MatSys::Renderer->GetCurrentEyePosition() currently oscillates up and down,
    // as added by the code in ClientWorld.cpp for eye-candy specular-effect...

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // MatSys::Renderer->...();
    MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, 1.0f/25.4f);
    MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, -90.0f);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // EyePos=EyePos-Entity->m_Origin;         // Convert into unrotated model space.
    EyePos=scale(EyePos, 25.4);
    EyePos=EyePos.GetRotZ(90.0);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
 // LightPos=LightPos-Entity->m_Origin;         // Convert into unrotated model space.
    LightPos=scale(LightPos, 25.4);
    LightPos=LightPos.GetRotZ(90.0);

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
    LightRadius*=25.4f;

    // UNDO things the EngineEntityT::Draw() code did with .mdl models in mind...
    // Set the modified (now in model space) lighting parameters.
    MatSys::Renderer->SetCurrentLightSourcePosition(float(LightPos.x), float(LightPos.y), float(LightPos.z));
    MatSys::Renderer->SetCurrentLightSourceRadius(LightRadius);
    MatSys::Renderer->SetCurrentEyePosition(float(EyePos.x), float(EyePos.y), float(EyePos.z));


    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
            RootNode->DrawAmbientContrib(EyePos);
            RootNode->DrawTranslucentContrib(EyePos);
            break;

        case MatSys::RendererI::LIGHTING:
            RootNode->DrawLightSourceContrib(EyePos, LightPos);
            break;

        case MatSys::RendererI::STENCILSHADOW:
            RootNode->DrawStencilShadowVolumes(LightPos, LightRadius);
            break;
    }
}


int EntFuncDoorT::SetOrigin(lua_State* LuaState)
{
    // ############### FIXME: This is the code from EntFuncMoverT - it doesn't really make sense for doors!!! ##################

    /* // Call the overridden base class implementation of this method first.
    BaseEntityT::SetOrigin(LuaState);

    EntFuncDoorT* Ent=(EntFuncDoorT*)cf::GameSys::ScriptStateT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    Ent->ClipModel.SetOrigin(Ent->m_Origin);
    Ent->ClipModel.Register();  // Re-register ourselves with the clip world. */
    return 0;
}
