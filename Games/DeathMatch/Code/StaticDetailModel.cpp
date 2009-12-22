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

/**********************************/
/*** Static Detail Model (Code) ***/
/**********************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "StaticDetailModel.hpp"
#include "EntityCreateParams.hpp"
#include "TypeSys.hpp"
#include "ConsoleCommands/Console.hpp"
#include "GuiSys/GuiMan.hpp"
#include "GuiSys/Gui.hpp"
#include "GuiSys/GuiMan.hpp"        // TEMPORARY -- REMOVE!
#include "MaterialSystem/Mesh.hpp"   // TEMPORARY -- REMOVE!
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"


/******************************************************************************************/
#include "GameImpl.hpp"
#include "ScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


// Runs the given command (first function parameter) within the script of the current map.
// The command is run *ONLY* while the server is thinking. That is, the call chain can only be as follows:
//     1) HumanPlayerT::Think();           // Called during server thinking, client prediction and client repredection.
//     2) GuiT::ProcessDeviceEvent();      // Player stands before and uses this GUI.
//     3) call into the GUI script (event handlers).
//     4) The script calls this game.runMapCmd(xy) function.
//     5) The xy command is run if the server is thinking, nothing is done otherwise.
static int WorldGui_RunMapScriptCmd(lua_State* GuiLuaState)
{
    // *** WARNING ***
    // The GuiLuaState is maintained in the main executable, not in this DLL, so its probably *NOT* safe to modify it here
    // with the CRT being *statically* linked. Inspecting the stack is probably harmless, but returning values probably
    // implies DLL-local memory allocation and thus a corrputed heap!

    const cf::GameSys::GameImplT& GameImpl   =cf::GameSys::GameImplT::GetInstance();
    cf::GameSys::ScriptStateT*    ScriptState=GameImpl.GetScriptState();

    if (!GameImpl.IsSvThinking()) return 0;

    // Having (IsThinking==true && ScriptState==NULL) is impossible.
    assert(ScriptState!=NULL);

    ScriptState->RunCmd(luaL_checkstring(GuiLuaState, 1));
    return 0;
}


static const luaL_Reg GameFunctionsForWorldGUIs[]=
{
    { "runMapCmd", WorldGui_RunMapScriptCmd },
    { NULL, NULL }
};
/******************************************************************************************/


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

const cf::TypeSys::TypeInfoT EntStaticDetailModelT::TypeInfo(GetBaseEntTIM(), "EntStaticDetailModelT", "BaseEntityT", EntStaticDetailModelT::CreateInstance, NULL /*MethodsList*/);


EntStaticDetailModelT::EntStaticDetailModelT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  // Bad. Should either have a default ctor for 'EntityStateT', or even better get it passed as const reference.
                  EntityStateT(Params.Origin,
                               VectorT(),
                               BoundingBox3T<double>(Vector3dT()),
                               0,       // Heading
                               0,
                               0,
                               0,       // StateOfExistance
                               0,
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
      Model(""),
      ModelSequenceNr(0),
      ModelFrameNr(0.0f),
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

            const char* s1=strtok(s,    " "); if (s1) State.Bank   =(unsigned short)(atof(s1)*8192.0/45.0);
            const char* s2=strtok(NULL, " "); if (s2) State.Heading=(unsigned short)(atof(s2)*8192.0/45.0);
            const char* s3=strtok(NULL, " "); if (s3) State.Pitch  =(unsigned short)(atof(s3)*8192.0/45.0);
        }
        else if (Key=="model")
        {
            Model=ModelProxyT(std::string("Games/DeathMatch/")+Value);
        }
        else if (Key=="sequence") ModelSequenceNr=atoi(Value.c_str());   // Note that the "sequence" key is NOT guaranteed to come after the "model" key!
        else if (Key=="gui"     )
        {
            GuiName=std::string("Games/DeathMatch/")+Value;

            // Load the Gui. Note that this is done BOTH on the client as well as on the server.
            assert(cf::GuiSys::GuiMan!=NULL);
            Gui=cf::GuiSys::GuiMan->Register(GuiName);

            if (Gui!=NULL)
            {
                // Let the GUI know our name and instance pointer.
                Gui->SetEntityInfo(Name.c_str(), this);

                // Provide the GUI with another library that has game-specific functions,
                // for example and most importantly a function to access the map script.
                Gui->RegisterScriptLib("game", GameFunctionsForWorldGUIs);
            }
            else
            {
                // If the registration failed, don't try again.
                Console->Warning("World GUI \""+GuiName+"\" not registered.\n");
            }
        }
    }


    // Set the proper Dimensions bounding box for this model.
    // Note that the bounding box depends on the current model sequence,
    // and it must be properly scaled and rotated for world space.
    const float* BB=Model.GetSequenceBB(ModelSequenceNr, 0.0f);
    VectorT      V[8];
    char         VertexNr;

    V[0]=VectorT(BB[0], BB[1], BB[2]);
    V[1]=VectorT(BB[0], BB[1], BB[5]);
    V[2]=VectorT(BB[0], BB[4], BB[2]);
    V[3]=VectorT(BB[0], BB[4], BB[5]);
    V[4]=VectorT(BB[3], BB[1], BB[2]);
    V[5]=VectorT(BB[3], BB[1], BB[5]);
    V[6]=VectorT(BB[3], BB[4], BB[2]);
    V[7]=VectorT(BB[3], BB[4], BB[5]);

    for (VertexNr=0; VertexNr<8; VertexNr++)
    {
        V[VertexNr]=scale(V[VertexNr], 25.4);
        V[VertexNr]=V[VertexNr].GetRotX(    -double(State.Pitch  )/8192.0*45.0);
        V[VertexNr]=V[VertexNr].GetRotY(     double(State.Bank   )/8192.0*45.0);
        V[VertexNr]=V[VertexNr].GetRotZ(90.0-double(State.Heading)/8192.0*45.0);
    }

    State.Dimensions=BoundingBox3T<double>(V[0]);
    for (VertexNr=1; VertexNr<8; VertexNr++) State.Dimensions.Insert(V[VertexNr]);


    ClipModel.SetOrigin(State.Origin);
    // TODO: Optimize! This matrix computation takes many unnecessary muls and adds...!
    ClipModel.SetOrientation(cf::math::Matrix3x3T<double>::GetRotateZMatrix(90.0-double(State.Heading)/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateYMatrix(     double(State.Bank   )/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateXMatrix(    -double(State.Pitch  )/8192.0*45.0));
    ClipModel.Register();
}


EntStaticDetailModelT::~EntStaticDetailModelT()
{
    if (Gui)
    {
        cf::GuiSys::GuiMan->Free(Gui);
    }
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


void EntStaticDetailModelT::Cl_UnserializeFrom()
{
    // Even though our State.Origin is never modified in Think(),
    // the code below is necessary because the client first creates new entities,
    // and only sets the proper state data after the constructor in a separate step.
    ClipModel.SetOrigin(State.Origin);
    // TODO: Optimize! This matrix computation takes many unnecessary muls and adds...!
    ClipModel.SetOrientation(cf::math::Matrix3x3T<double>::GetRotateZMatrix(90.0-double(State.Heading)/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateYMatrix(     double(State.Bank   )/8192.0*45.0)
                           * cf::math::Matrix3x3T<double>::GetRotateXMatrix(    -double(State.Pitch  )/8192.0*45.0));
    ClipModel.Register();
}


void EntStaticDetailModelT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    Vector3fT LgtPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    Vector3fT EyePos(MatSys::Renderer->GetCurrentEyePosition());

    const float DegBank =float(State.Bank )/8192.0f*45.0f;
    const float DegPitch=float(State.Pitch)/8192.0f*45.0f;

    LgtPos=LgtPos.GetRotY(-DegBank);
    EyePos=EyePos.GetRotY(-DegBank);
    MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, DegBank);

    LgtPos=LgtPos.GetRotX(DegPitch);
    EyePos=EyePos.GetRotX(DegPitch);
    MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, -DegPitch);

    MatSys::Renderer->SetCurrentLightSourcePosition(LgtPos.x, LgtPos.y, LgtPos.z);
    MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);


    Model.Draw(ModelSequenceNr, ModelFrameNr, LodDist);


    // Note that we could *not* render the Gui in PostDraw(), because the model-to-world transformation matrix is not properly setup there (but is here)!
    if (Gui!=NULL && MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        Vector3fT GuiOrigin;
        Vector3fT GuiAxisX;
        Vector3fT GuiAxisY;

        if (Model.GetGuiPlane(ModelSequenceNr, ModelFrameNr, LodDist, GuiOrigin, GuiAxisX, GuiAxisY))
        {
#if 1
            const MatrixT& ModelToWorld=MatSys::Renderer->GetMatrix(MatSys::RendererI::MODEL_TO_WORLD);

            // It's pretty easy to derive this matrix geometrically, see my TechArchive note from 2006-08-22.
            MatrixT M(GuiAxisX.x/640.0f, GuiAxisY.x/480.0f, 0.0f, GuiOrigin.x,
                      GuiAxisX.y/640.0f, GuiAxisY.y/480.0f, 0.0f, GuiOrigin.y,
                      GuiAxisX.z/640.0f, GuiAxisY.z/480.0f, 0.0f, GuiOrigin.z,
                                   0.0f,              0.0f, 0.0f,        1.0f);

            MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, ModelToWorld*M);

            Gui->Render();
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
    if (Gui!=NULL)
    {
        // The Gui is inactive, so that the GuiMan doesn't render it (we do it ourselves above).
        // However that means that we have to drive the clock ourselves, too.
        // Note that we *never* get here on the server-side, and therefore the call is duplicated above in EntStaticDetailModelT::Think() again.
        // assert(!Gui->GetIsActive());
        Gui->DistributeClockTickEvents(FrameTime);
    }

    // Advance the client-local animation.
    ModelFrameNr=Model.AdvanceFrameNr(ModelSequenceNr, ModelFrameNr, FrameTime, true);


    /* glColor3f(1.0, 0.0, 0.0);

    VectorT V1=VectorT(State.Dimensions.Min.x, State.Dimensions.Min.y, State.Dimensions.Min.z)+State.Origin;
    VectorT V2=VectorT(State.Dimensions.Min.x, State.Dimensions.Min.y, State.Dimensions.Max.z)+State.Origin;
    VectorT V3=VectorT(State.Dimensions.Min.x, State.Dimensions.Max.y, State.Dimensions.Min.z)+State.Origin;
    VectorT V4=VectorT(State.Dimensions.Min.x, State.Dimensions.Max.y, State.Dimensions.Max.z)+State.Origin;
    VectorT V5=VectorT(State.Dimensions.Max.x, State.Dimensions.Min.y, State.Dimensions.Min.z)+State.Origin;
    VectorT V6=VectorT(State.Dimensions.Max.x, State.Dimensions.Min.y, State.Dimensions.Max.z)+State.Origin;
    VectorT V7=VectorT(State.Dimensions.Max.x, State.Dimensions.Max.y, State.Dimensions.Min.z)+State.Origin;
    VectorT V8=VectorT(State.Dimensions.Max.x, State.Dimensions.Max.y, State.Dimensions.Max.z)+State.Origin;

    glBegin(GL_LINES);
        glVertex3f(V1.x, V1.z, -V1.y); glVertex3f(V2.x, V2.z, -V2.y);
        glVertex3f(V3.x, V3.z, -V3.y); glVertex3f(V4.x, V4.z, -V4.y);
        glVertex3f(V5.x, V5.z, -V5.y); glVertex3f(V6.x, V6.z, -V6.y);
        glVertex3f(V7.x, V7.z, -V7.y); glVertex3f(V8.x, V8.z, -V8.y);

        glVertex3f(V1.x, V1.z, -V1.y); glVertex3f(V3.x, V3.z, -V3.y);
        glVertex3f(V3.x, V3.z, -V3.y); glVertex3f(V7.x, V7.z, -V7.y);
        glVertex3f(V7.x, V7.z, -V7.y); glVertex3f(V5.x, V5.z, -V5.y);
        glVertex3f(V5.x, V5.z, -V5.y); glVertex3f(V1.x, V1.z, -V1.y);

        glVertex3f(V2.x, V2.z, -V2.y); glVertex3f(V4.x, V4.z, -V4.y);
        glVertex3f(V4.x, V4.z, -V4.y); glVertex3f(V8.x, V8.z, -V8.y);
        glVertex3f(V8.x, V8.z, -V8.y); glVertex3f(V6.x, V6.z, -V6.y);
        glVertex3f(V6.x, V6.z, -V6.y); glVertex3f(V2.x, V2.z, -V2.y);
    glEnd(); */
}


cf::GuiSys::GuiI* EntStaticDetailModelT::GetGUI() const
{
    return Gui;
}


bool EntStaticDetailModelT::GetGuiPlane(Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    if (!Model.GetGuiPlane(ModelSequenceNr, ModelFrameNr, 0.0, GuiOrigin, GuiAxisX, GuiAxisY)) return false;

    // Okay, got the plane. Now transform it from model space into world space.
    GuiOrigin=scale(GuiOrigin, 25.4f);
    GuiOrigin=GuiOrigin.GetRotX(     -float(State.Pitch  )/8192.0f*45.0f);
    GuiOrigin=GuiOrigin.GetRotY(      float(State.Bank   )/8192.0f*45.0f);
    GuiOrigin=GuiOrigin.GetRotZ(90.0f-float(State.Heading)/8192.0f*45.0f);

    GuiAxisX=scale(GuiAxisX, 25.4f);
    GuiAxisX=GuiAxisX.GetRotX(     -float(State.Pitch  )/8192.0f*45.0f);
    GuiAxisX=GuiAxisX.GetRotY(      float(State.Bank   )/8192.0f*45.0f);
    GuiAxisX=GuiAxisX.GetRotZ(90.0f-float(State.Heading)/8192.0f*45.0f);

    GuiAxisY=scale(GuiAxisY, 25.4f);
    GuiAxisY=GuiAxisY.GetRotX(     -float(State.Pitch  )/8192.0f*45.0f);
    GuiAxisY=GuiAxisY.GetRotY(      float(State.Bank   )/8192.0f*45.0f);
    GuiAxisY=GuiAxisY.GetRotZ(90.0f-float(State.Heading)/8192.0f*45.0f);

    // The GuiOrigin must also be translated.
    GuiOrigin+=State.Origin.AsVectorOfFloat();

    return true;
}
