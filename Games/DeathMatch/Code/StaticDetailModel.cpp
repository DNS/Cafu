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

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "StaticDetailModel.hpp"
#include "EntityCreateParams.hpp"
#include "../../GameWorld.hpp"
#include "TypeSys.hpp"
#include "ConsoleCommands/Console.hpp"
#include "GuiSys/GuiMan.hpp"
#include "GuiSys/Gui.hpp"
#include "GuiSys/GuiMan.hpp"        // TEMPORARY -- REMOVE!
#include "MaterialSystem/Mesh.hpp"   // TEMPORARY -- REMOVE!
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Models/Model_cmdl.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntStaticDetailModelT::GetType() const
{
    return &TypeInfo;
 // return &EntStaticDetailModelT::TypeInfo;
}

void* EntStaticDetailModelT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntStaticDetailModelT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const luaL_Reg EntStaticDetailModelT::MethodsList[]=
{
    { "IsPlayingAnim",   EntStaticDetailModelT::IsPlayingAnim },
    { "PlayAnim",        EntStaticDetailModelT::PlayAnim },
    { "GetSequNr",       EntStaticDetailModelT::GetSequNr },
    { "SetSequNr",       EntStaticDetailModelT::SetSequNr },
    { "RestartSequ",     EntStaticDetailModelT::RestartSequ },
    { "GetNumSequences", EntStaticDetailModelT::GetNumSequences },
 // { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT EntStaticDetailModelT::TypeInfo(GetBaseEntTIM(), "EntStaticDetailModelT", "BaseEntityT", EntStaticDetailModelT::CreateInstance, MethodsList);


EntStaticDetailModelT::EntStaticDetailModelT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT()),
                  NUM_EVENT_TYPES,
                  // Bad. Should either have a default ctor for 'EntityStateT', or even better get it passed as const reference.
                  EntityStateT(VectorT(),
                               0,       // StateOfExistance
                               1,       // Flags -- m_PlayAnim
                               0,       // ModelIndex
                               0,       // ModelSequNr
                               0.0,     // ModelFrameNr
                               0,       // Health
                               0,       // Armor
                               0,       // HaveItems
                               0,       // HaveWeapons
                               0,       // ActiveWeaponSlot
                               0,       // ActiveWeaponSequNr
                               0.0)),   // ActiveWeaponFrameNr
      m_Model(NULL),
      m_PlayAnim(State.Flags),
      m_SequNr(State.ModelSequNr),
      m_AnimExpr(),     // Inited in the ctor body below.
      m_LastStdAE(),
      GuiName(),
      Gui(NULL)
{
    // Werte die 'PropertyPairs' aus, die von der Basis-Klasse 'BaseEntity' noch nicht ausgewertet wurden!
    for (std::map<std::string, std::string>::const_iterator It=Properties.begin(); It!=Properties.end(); ++It)
    {
        const std::string& Key  =It->first;
        const std::string& Value=It->second;

        if (Key=="angles")
        {
            char s[1024];

            strncpy(s, Value.c_str(), 1023);
            s[1023]=0;

            const char* s1=strtok(s,    " "); if (s1) m_Bank   =(unsigned short)(atof(s1)*8192.0/45.0);
            const char* s2=strtok(NULL, " "); if (s2) m_Heading=(unsigned short)(atof(s2)*8192.0/45.0);
            const char* s3=strtok(NULL, " "); if (s3) m_Pitch  =(unsigned short)(atof(s3)*8192.0/45.0);
        }
        else if (Key=="model")
        {
            m_Model=GameWorld->GetModel(std::string("Games/DeathMatch/")+Value);
        }
        else if (Key=="sequence") m_SequNr=atoi(Value.c_str());    // Note that the "sequence" key is NOT guaranteed to come after the "model" key!
        else if (Key=="gui"     )
        {
            GuiName=std::string("Games/DeathMatch/")+Value;

            // Load the Gui. Note that this is done BOTH on the client as well as on the server.
            assert(cf::GuiSys::GuiMan!=NULL);
            Gui=cf::GuiSys::GuiMan->Register(GuiName);

            if (Gui!=NULL)
            {
                // Let the GUI know our map script state and name.
                Gui->SetEntityInfo(&GameWorld->GetScriptState(), Name);
            }
            else
            {
                // If the registration failed, don't try again.
                Console->Warning("World GUI \""+GuiName+"\" not registered.\n");
            }
        }
    }

    if (!m_Model)
        m_Model=GameWorld->GetModel("");


    m_LastStdAE=m_Model->GetAnimExprPool().GetStandard(m_SequNr, 0.0f);
    m_LastStdAE->SetForceLoop(true);
    m_AnimExpr =m_LastStdAE;
    m_SequNr   =m_LastStdAE->GetSequNr();   // Set m_SequNr to the m_LastStdAE's "normalized" value.


    // Set the proper Dimensions bounding box for this model.
    // Note that the bounding box depends on the current model sequence,
    // and it must be properly scaled and rotated for world space.
    VectorT V[8];
    m_Model->GetSharedPose(m_AnimExpr)->GetBB().AsBoxOfDouble().GetCornerVertices(V);

    for (unsigned int VertexNr=0; VertexNr<8; VertexNr++)
    {
        V[VertexNr]=scale(V[VertexNr], 25.4);
        V[VertexNr]=V[VertexNr].GetRotX(    -double(m_Pitch  )/8192.0*45.0);
        V[VertexNr]=V[VertexNr].GetRotY(     double(m_Bank   )/8192.0*45.0);
        V[VertexNr]=V[VertexNr].GetRotZ(90.0-double(m_Heading)/8192.0*45.0);
    }

    m_Dimensions=BoundingBox3T<double>(V[0]);
    for (unsigned int VertexNr=1; VertexNr<8; VertexNr++) m_Dimensions.Insert(V[VertexNr]);


    ClipModel.SetOrigin(m_Origin);
    // TODO: Optimize! This matrix computation takes many unnecessary muls and adds...!
    ClipModel.SetOrientation(cf::math::Matrix3x3T<double>::GetRotateZMatrix(90.0-double(m_Heading)/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateYMatrix(     double(m_Bank   )/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateXMatrix(    -double(m_Pitch  )/8192.0*45.0));
    ClipModel.Register();

    // Let the GUI script know that its entity has now been fully initialized,
    // it can take the opportunity for additional initializations.
    if (Gui) Gui->CallLuaFunc("OnEntityInit");
}


EntStaticDetailModelT::~EntStaticDetailModelT()
{
    if (Gui)
    {
        cf::GuiSys::GuiMan->Free(Gui);
    }
}


void EntStaticDetailModelT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    // Even though our m_Origin is never modified in Think(),
    // the code below is necessary because the client first creates new entities,
    // and only sets the proper state data after the constructor in a separate step.
    ClipModel.SetOrigin(m_Origin);
    // TODO: Optimize! This matrix computation takes many unnecessary muls and adds...!
    ClipModel.SetOrientation(cf::math::Matrix3x3T<double>::GetRotateZMatrix(90.0-double(m_Heading)/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateYMatrix(     double(m_Bank   )/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateXMatrix(    -double(m_Pitch  )/8192.0*45.0));
    ClipModel.Register();
}


void EntStaticDetailModelT::Think(float FrameTime, unsigned long ServerFrameNr)
{
    if (Gui)
    {
        // It is important that we advance the time on the server-side GUI, too,
        // so that it can for example work off the "pending interpolations" that the GUI scripts can create.
        // Note that we *never* get here on the client-side, and therefore the call is duplicated below in EntStaticDetailModelT::PostDraw() again.
        Gui->DistributeClockTickEvents(FrameTime);
    }
}


void EntStaticDetailModelT::ProcessEvent(unsigned int EventType, unsigned int /*NumEvents*/)
{
    switch (EventType)
    {
        case EVENT_TYPE_RESTART_SEQU:
            m_LastStdAE->SetFrameNr(0.0f);
            break;
    }
}


void EntStaticDetailModelT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    Vector3fT LgtPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    Vector3fT EyePos(MatSys::Renderer->GetCurrentEyePosition());

    const float DegBank =float(m_Bank )/8192.0f*45.0f;
    const float DegPitch=float(m_Pitch)/8192.0f*45.0f;

    LgtPos=LgtPos.GetRotY(-DegBank);
    EyePos=EyePos.GetRotY(-DegBank);
    MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, DegBank);

    LgtPos=LgtPos.GetRotX(DegPitch);
    EyePos=EyePos.GetRotX(DegPitch);
    MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, -DegPitch);

    MatSys::Renderer->SetCurrentLightSourcePosition(LgtPos.x, LgtPos.y, LgtPos.z);
    MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);


    AnimPoseT* Pose=m_Model->GetSharedPose(m_AnimExpr);
    Pose->Draw(-1 /*default skin*/, LodDist);


    // Note that we could *not* render the Gui in PostDraw(), because the model-to-world transformation matrix is not properly setup there (but is here)!
    if (Gui!=NULL && MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        Vector3fT GuiOrigin;
        Vector3fT GuiAxisX;
        Vector3fT GuiAxisY;

        if (Pose->GetGuiPlane(0, GuiOrigin, GuiAxisX, GuiAxisY))
        {
#if 1
            const MatrixT& ModelToWorld=MatSys::Renderer->GetMatrix(MatSys::RendererI::MODEL_TO_WORLD);

            // It's pretty easy to derive this matrix geometrically, see my TechArchive note from 2006-08-22.
            MatrixT M(GuiAxisX.x/640.0f, GuiAxisY.x/480.0f, 0.0f, GuiOrigin.x,
                      GuiAxisX.y/640.0f, GuiAxisY.y/480.0f, 0.0f, GuiOrigin.y,
                      GuiAxisX.z/640.0f, GuiAxisY.z/480.0f, 0.0f, GuiOrigin.z,
                                   0.0f,              0.0f, 0.0f,        1.0f);

            MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, ModelToWorld*M);

            Gui->Render();      // Set the zLayerCoating parameter to true here?
#else
            MatSys::Renderer->SetCurrentMaterial(cf::GuiSys::GuiMan->GetDefaultRM());

            // Just a single triangle that indicates the position and orientation of the GUI plane.
            static MatSys::MeshT Tri(MatSys::MeshT::Triangles);
            Tri.Vertices.Overwrite();
            Tri.Vertices.PushBackEmpty(3);

            Tri.Vertices[0].SetOrigin(GuiOrigin         ); Tri.Vertices[0].SetTextureCoord(0.0f, 0.0f); Tri.Vertices[0].SetColor(1, 1, 1, 1.0f);
            Tri.Vertices[1].SetOrigin(GuiOrigin+GuiAxisX); Tri.Vertices[1].SetTextureCoord(1.0f, 0.0f); Tri.Vertices[1].SetColor(1, 0, 0, 0.5f);
            Tri.Vertices[2].SetOrigin(GuiOrigin+GuiAxisY); Tri.Vertices[2].SetTextureCoord(0.0f, 1.0f); Tri.Vertices[2].SetColor(0, 1, 0, 0.5f);

            MatSys::Renderer->RenderMesh(Tri);
#endif
        }
    }
}


void EntStaticDetailModelT::PostDraw(float FrameTime, bool /*FirstPersonView*/)
{
    const int SequNr=(m_SequNr==255) ? -1 : m_SequNr;   // This is a hack, because m_SequNr actually has the wrong (unsigned) datatype...

    if (SequNr != m_LastStdAE->GetSequNr())
    {
        if (m_PlayAnim)
        {
            m_LastStdAE=m_Model->GetAnimExprPool().GetStandard(SequNr, 0.0f);
            m_LastStdAE->SetForceLoop(true);
            m_AnimExpr =m_Model->GetAnimExprPool().GetBlend(m_AnimExpr, m_LastStdAE, 3.0f);
        }
        else
        {
            m_LastStdAE->SetSequNr(m_SequNr);
        }

        m_SequNr=m_LastStdAE->GetSequNr();  // Set m_SequNr to the m_LastStdAE's "normalized" value.
    }

    if (Gui!=NULL)
    {
        // The Gui is inactive, so that the GuiMan doesn't render it (we do it ourselves above).
        // However that means that we have to drive the clock ourselves, too.
        // Note that we *never* get here on the server-side, and therefore the call is duplicated above in EntStaticDetailModelT::Think() again.
        // assert(!Gui->GetIsActive());
        Gui->DistributeClockTickEvents(FrameTime);
    }

    if (m_PlayAnim)
    {
        // Advance the client-local animation.
        m_AnimExpr->AdvanceTime(FrameTime);
    }
}


cf::GuiSys::GuiI* EntStaticDetailModelT::GetGUI() const
{
    return Gui;
}


bool EntStaticDetailModelT::GetGuiPlane(unsigned int GFNr, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    if (!m_Model->GetSharedPose(m_AnimExpr)->GetGuiPlane(GFNr, GuiOrigin, GuiAxisX, GuiAxisY)) return false;

    // Okay, got the plane. Now transform it from model space into world space.
    GuiOrigin=scale(GuiOrigin, 25.4f);
    GuiOrigin=GuiOrigin.GetRotX(     -float(m_Pitch  )/8192.0f*45.0f);
    GuiOrigin=GuiOrigin.GetRotY(      float(m_Bank   )/8192.0f*45.0f);
    GuiOrigin=GuiOrigin.GetRotZ(90.0f-float(m_Heading)/8192.0f*45.0f);

    GuiAxisX=scale(GuiAxisX, 25.4f);
    GuiAxisX=GuiAxisX.GetRotX(     -float(m_Pitch  )/8192.0f*45.0f);
    GuiAxisX=GuiAxisX.GetRotY(      float(m_Bank   )/8192.0f*45.0f);
    GuiAxisX=GuiAxisX.GetRotZ(90.0f-float(m_Heading)/8192.0f*45.0f);

    GuiAxisY=scale(GuiAxisY, 25.4f);
    GuiAxisY=GuiAxisY.GetRotX(     -float(m_Pitch  )/8192.0f*45.0f);
    GuiAxisY=GuiAxisY.GetRotY(      float(m_Bank   )/8192.0f*45.0f);
    GuiAxisY=GuiAxisY.GetRotZ(90.0f-float(m_Heading)/8192.0f*45.0f);

    // The GuiOrigin must also be translated.
    GuiOrigin+=m_Origin.AsVectorOfFloat();

    return true;
}


int EntStaticDetailModelT::IsPlayingAnim(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntStaticDetailModelT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntStaticDetailModelT> >(1, TypeInfo);

    lua_pushboolean(LuaState, Ent->m_PlayAnim);
    return 1;
}


int EntStaticDetailModelT::PlayAnim(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntStaticDetailModelT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntStaticDetailModelT> >(1, TypeInfo);

    if (lua_isboolean(LuaState, 2))
    {
        Ent->m_PlayAnim=lua_toboolean(LuaState, 2)!=0 ? 1 : 0;
        return 0;
    }

    Ent->m_PlayAnim=lua_tonumber(LuaState, 2)!=0 ? 1 : 0;
    return 0;
}


int EntStaticDetailModelT::GetSequNr(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntStaticDetailModelT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntStaticDetailModelT> >(1, TypeInfo);

    lua_pushinteger(LuaState, Ent->m_LastStdAE->GetSequNr());
    return 1;
}


int EntStaticDetailModelT::SetSequNr(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntStaticDetailModelT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntStaticDetailModelT> >(1, TypeInfo);

    Ent->m_LastStdAE->SetSequNr(std::max(0, luaL_checkinteger(LuaState, 2)));
    Ent->m_LastStdAE->SetFrameNr(0.0f);
    Ent->m_SequNr=Ent->m_LastStdAE->GetSequNr();    // Set m_SequNr to the m_LastStdAE's "normalized" value.
    return 0;
}


int EntStaticDetailModelT::RestartSequ(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntStaticDetailModelT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntStaticDetailModelT> >(1, TypeInfo);

    Ent->m_LastStdAE->SetFrameNr(0.0f);
    Ent->PostEvent(EVENT_TYPE_RESTART_SEQU);
    return 0;
}


int EntStaticDetailModelT::GetNumSequences(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntStaticDetailModelT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntStaticDetailModelT> >(1, TypeInfo);

    lua_pushinteger(LuaState, Ent->m_Model->GetAnims().Size());
    return 1;
}
