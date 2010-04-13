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

#include "ClientStateInGame.hpp"
#include "Client.hpp"
#include "ClientWorld.hpp"
#include "PathRecorder.hpp"

#include "../NetConst.hpp"
#include "GuiSys/Gui.hpp"
#include "GuiSys/GuiMan.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "Math3D/Matrix.hpp"
#include "Network/Network.hpp"
#include "OpenGL/OpenGLWindow.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "SceneGraph/Node.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "DebugLog.hpp"
#include "../../Games/BaseEntity.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#define WSAECONNRESET  ECONNRESET
#define WSAEMSGSIZE    EMSGSIZE
#define WSAEWOULDBLOCK EWOULDBLOCK
#endif


static ClientStateInGameT* ClientIGSPtr=NULL;


int ClientStateInGameT::ConFunc_say_Callback(lua_State* LuaState)
{
    if (!ClientIGSPtr) return 0;

    NetDataT NewReliableMsg;

    NewReliableMsg.WriteByte  (CS1_SayToAll);
    NewReliableMsg.WriteString(luaL_checkstring(LuaState, 1));

    ClientIGSPtr->ReliableDatas.PushBack(NewReliableMsg.Data);
    return 0;
}

static ConFuncT ConFunc_say("say", ClientStateInGameT::ConFunc_say_Callback, ConFuncT::FLAG_MAIN_EXE, "Sends the given string to all connected clients.");


#if 0
// This is a nice auxiliary function for recording demo movies (e.g. using Fraps),
// as we can easily script fake-chats with it, see example in ChatInput.cgui.
int ClientStateInGameT::ConFunc_chatPrint_Callback(lua_State* LuaState)
{
    if (!ClientIGSPtr) return 0;

    ClientIGSPtr->ChatScrollInfo.Print(luaL_checkstring(LuaState, 1));
    return 0;
}

static ConFuncT ConFunc_chatPrint("chatPrint", ClientStateInGameT::ConFunc_chatPrint_Callback, ConFuncT::FLAG_MAIN_EXE, "Prints the given string as a chat message area.");
#endif


int ClientStateInGameT::ConFunc_showPath_Callback(lua_State* LuaState)
{
    if (!ClientIGSPtr) return 0;
    if (lua_gettop(LuaState)!=6) return luaL_error(LuaState, "Usage:   showPath(Ax, Ay, Az, Bx, By, Bz)   (shows the path from A to B).\n");

    VectorT Start(luaL_checknumber(LuaState, 1), luaL_checknumber(LuaState, 2), luaL_checknumber(LuaState, 3));
    VectorT End  (luaL_checknumber(LuaState, 4), luaL_checknumber(LuaState, 5), luaL_checknumber(LuaState, 6));

    ClientIGSPtr->World->ComputeBFSPath(Start, End);
    return 0;
}

static ConFuncT ConFunc_showPath("showPath", ClientStateInGameT::ConFunc_showPath_Callback, ConFuncT::FLAG_MAIN_EXE, "Shows the \"shortest\" path from one point in space to another.");


int ClientStateInGameT::ConFunc_recordPath_Callback(lua_State* LuaState)
{
    if (!ClientIGSPtr) return 0;

    PathRecorderT*& Recorder=ClientIGSPtr->m_PathRecorder;
    const char*     FileName=lua_tostring(LuaState, 1);

    if (FileName)
    {
        // Already recording to the file with the given name? Continue recording.
        // If we were not recording before, or are recording to a different file, start recording.
        if (!Recorder || Recorder->GetFileName()!=FileName)
        {
            delete Recorder;
            Recorder=new PathRecorderT(FileName);
        }
    }
    else
    {
        // Stop recording.
        delete Recorder;
        Recorder=NULL;
    }

    return 0;
}

static ConFuncT ConFunc_recordPath("recordPath", ClientStateInGameT::ConFunc_recordPath_Callback, ConFuncT::FLAG_MAIN_EXE, "Records the players path into a pointfile (load into CaWE to view).");


ClientStateInGameT::ClientStateInGameT(ClientT& Client_)
    : Client(Client_),
      Font_v("Fonts/Arial"),
      Font_f("Fonts/FixedWidth"),
      World(NULL),
      IsLoadingWorld(false),
      WasLMBOnceUp(false),
      ClientFrameNr(0),
      m_PathRecorder(NULL)
{
    assert(Client.Socket!=INVALID_SOCKET);

    assert(ClientIGSPtr==NULL);
    ClientIGSPtr=this;
}


ClientStateInGameT::~ClientStateInGameT()
{
    delete m_PathRecorder;
    m_PathRecorder=NULL;

    if (Client.MainMenuGui!=NULL)
    {
        Client.MainMenuGui->Activate();

        // Bringing the main menu to front here is not good:
        // It should have been in front of the client window anyway, and if
        // we get here because "disconnect()" was typed into the console,
        // the console is suddenly "behind" the main menu, and thus inaccessible.
     // cf::GuiSys::GuiMan->BringToFront(Client.MainMenuGui);
    }

    try
    {
        const ArrayT< ArrayT<char> > EmptyReliableMessage;
        NetDataT                     UnreliableMessage;

        // Disconnect Msg senden, es gibt kein ACK
        UnreliableMessage.WriteByte(CS1_Disconnect);
        GameProtocol.GetTransmitData(EmptyReliableMessage, UnreliableMessage.Data).Send(Client.Socket, Client.ServerAddress);
    }
    catch (const NetworkError& /*E*/) {}    // Ignoriere mögliche Network-Errors.

    delete World;
    World=NULL;

    assert(ClientIGSPtr==this);
    ClientIGSPtr=NULL;
}


int ClientStateInGameT::GetID() const
{
    return ClientT::INGAME;
}


bool ClientStateInGameT::ProcessInputEvent(const CaKeyboardEventT& KE)
{
    if (KE.Type!=CaKeyboardEventT::CKE_KEYDOWN) return false;

    switch (KE.Key)
    {
        case CaKeyboardEventT::CK_ESCAPE:
        {
            if (Client.MainMenuGui!=NULL)
            {
                Client.MainMenuGui->Activate();
                cf::GuiSys::GuiMan->BringToFront(Client.MainMenuGui);
            }
            break;
        }

        case CaKeyboardEventT::CK_T:          // talk to other clients
        case CaKeyboardEventT::CK_Y:
        {
#ifdef DEBUG
            Console->DevWarning("Should replace the static string \"DeathMatch\" with the true GameName here!\n");
#endif
            cf::GuiSys::GuiI* ChatInputGui=cf::GuiSys::GuiMan->Find(std::string("Games/")+/*GameName*/"DeathMatch"+"/GUIs/ChatInput.cgui", true);

            // Could be NULL on file not found, parse error, etc.
            if (ChatInputGui!=NULL)
            {
                ChatInputGui->Activate();
                cf::GuiSys::GuiMan->BringToFront(ChatInputGui);
            }
            break;
        }

        case CaKeyboardEventT::CK_SPACE:
            PlayerCommand.Keys|=PCK_Jump;
            break;

        case CaKeyboardEventT::CK_1:
        case CaKeyboardEventT::CK_NUMPAD1:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x10000000;
            break;

        case CaKeyboardEventT::CK_2:
        case CaKeyboardEventT::CK_NUMPAD2:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x20000000;
            break;

        case CaKeyboardEventT::CK_3:
        case CaKeyboardEventT::CK_NUMPAD3:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x30000000;
            break;

        case CaKeyboardEventT::CK_4:
        case CaKeyboardEventT::CK_NUMPAD4:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x40000000;
            break;

        case CaKeyboardEventT::CK_5:
        case CaKeyboardEventT::CK_NUMPAD5:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x50000000;
            break;

        case CaKeyboardEventT::CK_6:
        case CaKeyboardEventT::CK_NUMPAD6:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x60000000;
            break;

        case CaKeyboardEventT::CK_7:
        case CaKeyboardEventT::CK_NUMPAD7:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x70000000;
            break;

        case CaKeyboardEventT::CK_8:
        case CaKeyboardEventT::CK_NUMPAD8:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x80000000;
            break;

        case CaKeyboardEventT::CK_9:
        case CaKeyboardEventT::CK_NUMPAD9:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0x90000000;
            break;

        case CaKeyboardEventT::CK_0:
        case CaKeyboardEventT::CK_NUMPAD0:
            PlayerCommand.Keys&=0x0FFFFFFF;
            PlayerCommand.Keys|=0xA0000000;
            break;

        default:
            break;
    }

    return true;
}


// This convar is declared here rather than (statically local) in the CM_MOVE_Y case as indicated below
// so that it is registered with the console interpreter early during initialization, and not only when
// the program flow first enters the CM_MOVE_Y case.
// Otherwise, when the Main Menu GUI tried to access this variable (through the console interpreter),
// it might not be available yet, which may make the "Input Controls" dialog look broken to the user.
static ConVarT MouseReverseY("mouseRevY", false, ConVarT::FLAG_MAIN_EXE | ConVarT::FLAG_PERSISTENT, "Flips the interpretation of the mouse up/down axis (normal vs. aircraft-style).");

bool ClientStateInGameT::ProcessInputEvent(const CaMouseEventT& ME)
{
    switch (ME.Type)
    {
        case CaMouseEventT::CM_MOVE_X:   // X-Axis.
            PlayerCommand.DeltaHeading+=(unsigned short)(ME.Amount*30);
            break;

        case CaMouseEventT::CM_MOVE_Y:   // Y-Axis.
        {
            // static ConVarT MouseReverseY(...);

            if (MouseReverseY.GetValueBool()) PlayerCommand.DeltaPitch-=(unsigned short)(ME.Amount*30);
                                         else PlayerCommand.DeltaPitch+=(unsigned short)(ME.Amount*30);
            break;
        }

        default:
            // Ignore other ME types.
            break;
    }

    return true;
}


static MatSys::RenderMaterialT* LogoRenderMat =NULL;
static MatSys::RenderMaterialT* LoadingBarLMat=NULL;
static MatSys::RenderMaterialT* LoadingBarRMat=NULL;
static MatSys::RenderMaterialT* LoadingBar0Mat=NULL;
static MatSys::RenderMaterialT* LoadingBar1Mat=NULL;
static FontT*                   LoadingFont   =NULL;
static float                    LoadingProgressPercent=0.0f;
static std::string              LoadingProgressText="";


static ConVarT UsePrediction("usePrediction", true, ConVarT::FLAG_MAIN_EXE, "Toggles whether client prediction is used (recommended!).");


void ClientStateInGameT::Render(float FrameTime)
{
    Graphs.ClearForFrame(ClientFrameNr);

    // Bestimme die FrameTime des letzten Frames
    if (FrameTime<0.0001f) FrameTime=0.0001f;

    Graphs.FPS[ClientFrameNr & (512-1)]=1.0f/FrameTime;


    if (World)
    {
        const EntityStateT* OurEntityCurrentState=World->OurEntity_GetState(UsePrediction.GetValueBool());

        if (OurEntityCurrentState!=NULL)
        {
            Graphs.Heading[ClientFrameNr & (512-1)]=(OurEntityCurrentState->Heading >> 5) & 511;
            Graphs.PosY   [ClientFrameNr & (512-1)]=((unsigned short)(OurEntityCurrentState->Origin.y/20.0)) & 511;
            Graphs.PosZ   [ClientFrameNr & (512-1)]=((unsigned short)(OurEntityCurrentState->Origin.z/20.0)) & 511;

            World->Draw(FrameTime, OurEntityCurrentState);

            // Die Framerate ist zwar World-unabhängig, ihre Anzeige hier aber besser aufgehoben (aus rein kosmetischen Gründen).
            static ConVarT ShowFrameRate("showFPS", false, ConVarT::FLAG_MAIN_EXE, "Toggles whether the frames-per-second number is shown.");
            static ConVarT ShowPosition("showPos", false, ConVarT::FLAG_MAIN_EXE, "Toggles whether the current players position is shown.");

            if (ShowFrameRate.GetValueBool()) Font_f.Print(SingleOpenGLWindow->GetWidth()-100, SingleOpenGLWindow->GetHeight()-16, 0x00FFFFFF, "FPS %5.1f", 1.0f/FrameTime);

            if (ShowPosition.GetValueBool())
            {
                // unsigned long LeafNr      =World->GetCa3DEWorldP()->Map.WhatLeaf(OurEntityCurrentState->Origin);
                // char          LeafContents='o';

                // if (World->GetCa3DEWorldP()->Map.Leaves[LeafNr].IsInnerLeaf)
                //     LeafContents=World->GetCa3DEWorldP()->Map.Leaves[LeafNr].IsWaterLeaf ? 'w' : 'i';

                Font_f.Print(SingleOpenGLWindow->GetWidth()-130, 15, 0x00FFFFFF, "X %10.1f", OurEntityCurrentState->Origin.x);
                Font_f.Print(SingleOpenGLWindow->GetWidth()-130, 35, 0x00FFFFFF, "Y %10.1f", OurEntityCurrentState->Origin.y);
                Font_f.Print(SingleOpenGLWindow->GetWidth()-130, 55, 0x00FFFFFF, "Z %10.1f", OurEntityCurrentState->Origin.z);
                Font_f.Print(SingleOpenGLWindow->GetWidth()-130, 75, 0x00FFFFFF, "Hdg %8u", OurEntityCurrentState->Heading);
             // Font_f.Print(SingleOpenGLWindow->GetWidth()-100, SingleOpenGLWindow->GetHeight()-32, 0x00FFFFFF, "L %4u %c", LeafNr, LeafContents);
            }

            /*if (ShowLeaf)
            {
                ;
            }

            if (ShowPointPath)
            {
                ;
            }*/
        }
        else
        {
            Font_f.Print(30, SingleOpenGLWindow->GetHeight()*2/3, 0x00004080, "Receiving entity baselines...");
        }
    }
    else
    {
        // Falls wir noch keine World haben (weil nach dem Starten vom Server bisher nichts kam), können wir nicht viel tun.
        if (IsLoadingWorld)
        {
         // MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
         // MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );
            MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );

            MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
            MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());
            MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, float(SingleOpenGLWindow->GetWidth()), float(SingleOpenGLWindow->GetHeight()), 0.0f, -1.0f, 1.0f));

            MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);

            MatSys::MeshT M(MatSys::MeshT::Quads);
            M.Vertices.PushBackEmpty(4);
            M.Vertices[0].SetTextureCoord(0, 0);
            M.Vertices[1].SetTextureCoord(1, 0);
            M.Vertices[2].SetTextureCoord(1, 1);
            M.Vertices[3].SetTextureCoord(0, 1);

            // Render the big logo.
            {
                // Do floating-point (rather than unsigned long) math for the origin, or else we get into trouble with negative numbers.
                // The coordinates have also been tested with a hor. and ver. stripe pattern texture for making sure that there is no
                // inadvertent scaling by plus or minus one pixel.
                M.Vertices[0].SetOrigin(SingleOpenGLWindow->GetWidth()/2.0f-512.0f, SingleOpenGLWindow->GetHeight()/2.0f-256.0f-20.0f); // links  oben
                M.Vertices[1].SetOrigin(SingleOpenGLWindow->GetWidth()/2.0f+512.0f, SingleOpenGLWindow->GetHeight()/2.0f-256.0f-20.0f); // rechts oben
                M.Vertices[2].SetOrigin(SingleOpenGLWindow->GetWidth()/2.0f+512.0f, SingleOpenGLWindow->GetHeight()/2.0f+256.0f-20.0f); // rechts unten
                M.Vertices[3].SetOrigin(SingleOpenGLWindow->GetWidth()/2.0f-512.0f, SingleOpenGLWindow->GetHeight()/2.0f+256.0f-20.0f); // links  unten

                MatSys::Renderer->SetCurrentMaterial(LogoRenderMat);
                MatSys::Renderer->RenderMesh(M);
            }


        #if 0
            const unsigned long BarHalfWidth=SingleOpenGLWindow->GetWidth()/2-40;

            // Render the left end of the loading bar.
            {
                M.Vertices[0].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth-16, SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[1].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth   , SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[2].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth   , SingleOpenGLWindow->GetHeight()*9/10-20+32);
                M.Vertices[3].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth-16, SingleOpenGLWindow->GetHeight()*9/10-20+32);

                MatSys::Renderer->SetCurrentMaterial(LoadingBarLMat);
                MatSys::Renderer->RenderMesh(M);
            }

            // Render the center left (filled) part of the loading bar.
            {
                M.Vertices[0].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth                               , SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[1].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth+2*BarHalfWidth*ProgressPercent, SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[2].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth+2*BarHalfWidth*ProgressPercent, SingleOpenGLWindow->GetHeight()*9/10-20+32);
                M.Vertices[3].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth                               , SingleOpenGLWindow->GetHeight()*9/10-20+32);

                MatSys::Renderer->SetCurrentMaterial(LoadingBar1Mat);
                MatSys::Renderer->RenderMesh(M);
            }

            // Render the center right (not yet filled) part of the loading bar.
            {
                M.Vertices[0].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth+2*BarHalfWidth*ProgressPercent, SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[1].SetOrigin(SingleOpenGLWindow->GetWidth()/2+BarHalfWidth                               , SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[2].SetOrigin(SingleOpenGLWindow->GetWidth()/2+BarHalfWidth                               , SingleOpenGLWindow->GetHeight()*9/10-20+32);
                M.Vertices[3].SetOrigin(SingleOpenGLWindow->GetWidth()/2-BarHalfWidth+2*BarHalfWidth*ProgressPercent, SingleOpenGLWindow->GetHeight()*9/10-20+32);

                MatSys::Renderer->SetCurrentMaterial(LoadingBar0Mat);
                MatSys::Renderer->RenderMesh(M);
            }

            // Render the right end of the loading bar.
            {
                M.Vertices[0].SetOrigin(SingleOpenGLWindow->GetWidth()/2+BarHalfWidth   , SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[1].SetOrigin(SingleOpenGLWindow->GetWidth()/2+BarHalfWidth+16, SingleOpenGLWindow->GetHeight()*9/10-20   );
                M.Vertices[2].SetOrigin(SingleOpenGLWindow->GetWidth()/2+BarHalfWidth+16, SingleOpenGLWindow->GetHeight()*9/10-20+32);
                M.Vertices[3].SetOrigin(SingleOpenGLWindow->GetWidth()/2+BarHalfWidth   , SingleOpenGLWindow->GetHeight()*9/10-20+32);

                MatSys::Renderer->SetCurrentMaterial(LoadingBarRMat);
                MatSys::Renderer->RenderMesh(M);
            }
        #endif

            MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION);
            // Don't bother to restore the model-to-world or world-to-view matrices: Whatever code follows should write its own setup into them.


            const unsigned long CharWidth=10;

            #ifdef DEBUG
                LoadingFont->Print(SingleOpenGLWindow->GetWidth()/2-34*CharWidth/2, SingleOpenGLWindow->GetHeight()*9/10+12, 0x00800000, "Version: %s [Debug build], %s", __DATE__, LoadingProgressText.c_str());
            #else
                LoadingFont->Print(SingleOpenGLWindow->GetWidth()/2-20*CharWidth/2, SingleOpenGLWindow->GetHeight()*9/10+12, 0x00800000, "Version: %s", __DATE__);
            #endif

            if (LoadingProgressPercent>0)
                LoadingFont->Print(SingleOpenGLWindow->GetWidth()/2-10*CharWidth/2, SingleOpenGLWindow->GetHeight()*9/10+30, 0x00800000, "Loading... %.0f%%", LoadingProgressPercent*100.0f);
            else
                LoadingFont->Print(SingleOpenGLWindow->GetWidth()/2-10*CharWidth/2, SingleOpenGLWindow->GetHeight()*9/10+30, 0x00800000, "Loading...");
        }
        else
        {
            // We have no world yet, and are currently not loading one,
            // which means that we're still waiting for a SC1_WorldInfo message.
            Font_f.Print(5, 200, 0x00DDFFBB, "Waiting for SC1_WorldInfo message...");
        }
    }

    // Zeichne die restlichen Dinge, die unabhängig von der World sind.
    static ConVarT ShowGraphs("showGraphs", false, ConVarT::FLAG_MAIN_EXE, "Toggles whether some FPS graphs are shown.");

    if (ShowGraphs.GetValueBool()) Graphs.Draw(ClientFrameNr);

    for (const char* s=DequeueString(); s!=NULL; s=DequeueString())
    {
        Console->Print(std::string(s)+"\n");
        SystemScrollInfo.Print(s);
    }

    ChatScrollInfo  .Draw(Font_v, 5, SingleOpenGLWindow->GetHeight()-10-140);
    ChatScrollInfo  .AdvanceTime(FrameTime);
    SystemScrollInfo.Draw(Font_v, 5, 15);
    SystemScrollInfo.AdvanceTime(FrameTime);


    ClientFrameNr++;
}


void ClientStateInGameT::ProcessConnectionLessPacket(NetDataT& InData, const NetAddressT& SenderAddress)
{
    InData.ReadLong();    // const unsigned long PacketID=InData.ReadLong();

    /* if (PacketID!=ExpectedPacketID)
    {
        Console->Warning(...);
        return;
    } */

    switch (InData.ReadByte())
    {
        case SC0_RccReply:
            // This is the reply to a CS0_RemoteConsoleCommand message.
            // Setting a non-default color would be nice...
            Console->Print(InData.ReadString());
            break;

        case SC0_ACK:
        case SC0_NACK:
        default:
            // Unexpected message type...
            break;
    }
}


static void WorldLoadingProgressFunction(float ProgressPercent, const char* ProgressText)
{
    LoadingProgressPercent=ProgressPercent;
    LoadingProgressText   =ProgressText ? ProgressText : "";

    // TODO 1: Should pass the total, global system time to BeginFrame().
    // TODO 2: This should be in a "Yield()" method. (That additionally makes sure that it is not called recursively.)
    //
    // Also note that we (==Yield()) need our *own* Renderer->BeginFrame() / EndFrame() pair,
    // because   cf::GuiSys::GuiMan->RenderAll();   doesn't clear the screen and thus cannot be called twice inside this pair.
    // The pair however cannot be nested either! Thus calling Yield() from within   cf::GuiSys::GuiMan->RenderAll();   must not be allowed at all!
    MatSys::Renderer->BeginFrame(0.0);

    cf::GuiSys::GuiMan->RenderAll();

    MatSys::Renderer->EndFrame();
    SingleOpenGLWindow->SwapBuffers();

    SoundSystem->Update();
}


void ClientStateInGameT::ParseServerPacket(NetDataT& InData)
{
    cf::LogDebug(net, "SC1_*: BEGIN parsing InData of size %lu", InData.Data.Size());

    while (!InData.ReadOfl && !(InData.ReadPos>=InData.Data.Size()))
    {
        char MessageType=InData.ReadByte();

        switch (MessageType)
        {
            case SC1_WorldInfo:
            {
                const char*   GameName   =InData.ReadString();
                const char*   WorldName  =InData.ReadString();
                unsigned long OurEntityID=InData.ReadLong();

                cf::LogDebug(net, "SC1_WorldInfo: %s %s %lu", GameName, WorldName, OurEntityID);
                // printf("    Client: Got MapInfo");

                // This must be here, because if we really wanted to re-register materials here after a game change
                // (which servers can't do anyway), we should first get rid of the old world before re-initing the MaterialManager.
                delete World;
                World=NULL;

                // If we really wanted to change the game here, the following would suffice.
                // See my svn commit message to revision #9 for a detailed discussion about why this doesn't conflict with
                // a simultaneously running server!
                //
                // MaterialManager->ClearAllMaterials();
                // MaterialManager->RegisterMaterialScriptsInDir(std::string("Games/")+GameName+"/Materials", std::string("Games/")+GameName+"/");
                //
                // // Re-assign fonts now that we have read the material scripts (which are all game-specific).
                // Font_v=FontT("Fonts/Arial");
                // Font_f=FontT("Fonts/FixedWidth");

                // Welcome-Screen anzeigen.
                LogoRenderMat =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("MainSplashLogo"    ));
                LoadingBarLMat=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("LoadingBar_Left"   ));
                LoadingBarRMat=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("LoadingBar_Right"  ));
                LoadingBar0Mat=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("LoadingBar_Center0"));
                LoadingBar1Mat=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("LoadingBar_Center1"));
                LoadingFont   =&Font_v;

                // BEGIN Load Map
                cf::GuiSys::GuiMan->Find(std::string("Games/")+/*GameName*/"DeathMatch"+"/GUIs/Console.cgui", true)->Activate(false);    // Close console on map change.
                Console->Print(std::string("Load World \"")+WorldName+"\".\n");

                char PathName[512];
                sprintf(PathName, "Games/%.200s/Worlds/%.200s.cw", GameName, WorldName);

                try
                {
                    // World is deleted above.
                    IsLoadingWorld=true;
                    World=new CaClientWorldT(PathName, WorldLoadingProgressFunction, OurEntityID);
                    IsLoadingWorld=false;

                    ConsoleInterpreter->RunCommand("StartLevelIntroMusic()");   // This function must be provided in "config.lua".

                    WasLMBOnceUp=false;
                }
                catch (const WorldT::LoadErrorT& E)
                {
                    Console->Warning(E.Msg);
                    Client.NextState=ClientT::IDLE;
                }
                // END Load Map

                MatSys::Renderer->FreeMaterial(LogoRenderMat ); LogoRenderMat =NULL;
                MatSys::Renderer->FreeMaterial(LoadingBarLMat); LoadingBarLMat=NULL;
                MatSys::Renderer->FreeMaterial(LoadingBarRMat); LoadingBarRMat=NULL;
                MatSys::Renderer->FreeMaterial(LoadingBar0Mat); LoadingBar0Mat=NULL;
                MatSys::Renderer->FreeMaterial(LoadingBar1Mat); LoadingBar1Mat=NULL;
                LoadingFont=NULL;


                // Wir bestätigen explizit, daß wir in der neuen Map sind, damit der Server uns von
                // MapTransition nach Dead setzt und uns wieder unreliable Data schickt.
                // Dies ist notwendig, falls der MapChange gerade erfolgt, während der Server noch auf ein
                // ACK von uns wartet. Erst das danach folgende ACK wäre das ACK zur MapInfo, also bestätigen
                // wir es einfach explizit, und alles ist OK!
                NetDataT NewReliableMsg;

                NewReliableMsg.WriteByte(CS1_WorldInfoACK);
                // if (Worldchange hat geklappt, kein Fehler) ReliableBuffer.WriteString(WorldName);
                //                                       else ReliableBuffer.WriteString("");     // Hat nicht geklappt
                NewReliableMsg.WriteString(WorldName);

                ReliableDatas.PushBack(NewReliableMsg.Data);
                break;
            }

            case SC1_ChatMsg:
            {
                const char* ChatMessage=InData.ReadString();

                cf::LogDebug(net, "SC1_ChatMsg: %s", ChatMessage);
                Console->Print(std::string(ChatMessage)+"\n");
                ChatScrollInfo.Print(ChatMessage);
                break;
            }

            case SC1_EntityBaseLine:
                cf::LogDebug(net, "SC1_EntityBaseLine: World==%p", World);
                if (!World) return;
                World->ReadEntityBaseLineMessage(InData);
                // Console->Print("Received BaseLine\n");
                break;

            case SC1_FrameInfo:
                cf::LogDebug(net, "SC1_FrameInfo: World==%p", World);
                if (!World) return;
                // Sende hier direkt eine CS1_FrameInfoACK-Message zurück.
                // Beachte, daß dies nicht in unserer Hauptschleife geschehen muß, selbst bei Packet-Loss nicht!
                UnreliableData.WriteByte(CS1_FrameInfoACK);
                UnreliableData.WriteLong(World->ReadServerFrameMessage(InData));
                break;

            case SC1_DropClient:
            {
                unsigned long EntityID=InData.ReadLong();
                const char*   Reason  =InData.ReadString();

                cf::LogDebug(net, "SC1_DropClient: EntityID==%lu%s, Reason==%s, World==%p", EntityID,
                    World ? (EntityID==World->GetOurEntityID() ? " (our)" : " (not our)") : "", Reason, World);

                if (World)
                    if (EntityID==World->GetOurEntityID())
                        Client.NextState=ClientT::IDLE;

                Console->Print(cf::va("Client with EntityID %u has left the game. Reason: %s\n", EntityID, Reason));
                SystemScrollInfo.Print("Client with EntityID %u has left the game. Reason: %s", EntityID, Reason);
                break;
            }

            default:
                // Alle SC1_EntityUpdate-Messages sollten nach SC1_FrameInfo schon gelesen worden sein!
                cf::LogDebug(net, "SC1_???: WARNING: Unknown SC1_* in-game message type '%u' received!\n", MessageType);
                Console->Print(cf::va("WARNING: Unknown in-game message type '%3u' received!\n", MessageType));
                SystemScrollInfo.Print("WARNING: Unknown in-game message type '%3u' received!", MessageType);
                // assert(false);
        }
    }

    cf::LogDebug(net, "SC1_*: END parsing InData\n");
}


void ClientStateInGameT::ParseServerPacketHelper(NetDataT& InData, unsigned long /*LastIncomingSequenceNr*/)
{
    ClientIGSPtr->ParseServerPacket(InData);
}


void ClientStateInGameT::MainLoop(float FrameTime)
{
    // Prüfe auf Server-Antwort(en) und verarbeite diese. Wir holen in einer Schleife die Packets ab, bis keine mehr da sind.
    // Dies ist insbesondere wichtig, wenn wir auf einem langsamen Computer schneller Server-Packets erhalten als wir Frames generieren können!
    // (Würde pro Frame nur ein Packet bearbeitet werden, gäbe es in einem solchen Fall Buffer-Overflows im OS und folglich packet-loss!)
    unsigned long MaxPacketsCount=20;

    while (MaxPacketsCount--)
    {
        try
        {
            NetDataT    InData;
            NetAddressT SenderAddress=InData.Receive(Client.Socket);

            if (GameProtocol1T::IsIncomingMessageOutOfBand(InData))
            {
                // It's well possible that we receive connection-less messages,
                // e.g. in reply to the connection-less messages that we have sent.
                ProcessConnectionLessPacket(InData, SenderAddress);
            }
            else
            {
                assert(ClientIGSPtr==this);
                const unsigned long RemoteLastIncomingSequenceNr=GameProtocol.ProcessIncomingMessage(InData, ParseServerPacketHelper);

                if (World && UsePrediction.GetValueBool() && RemoteLastIncomingSequenceNr!=0)
                {
                    // Führe für unseren Entity die Reprediction durch.
                    // Der zweite Parameter entspricht der 'LastOutgoingSequenceNr'.
                    World->OurEntity_Repredict(RemoteLastIncomingSequenceNr, GameProtocol.GetNextOutgoingSequenceNr()-1);
                }
            }
        }
        catch (const NetDataT::WinSockAPIError& E)
        {
            if (E.Error==WSAEWOULDBLOCK) break;

            const char* ErrorString="not yet looked-up";

            switch (E.Error)
            {
             // case WSAEWOULDBLOCK: ErrorString="WSAEWOULDBLOCK"; break;   // WSAEWOULDBLOCK is silently handled above
                case WSAEMSGSIZE   : ErrorString="WSAEMSGSIZE"   ; break;
                case WSAECONNRESET : ErrorString="WSAECONNRESET" ; break;
            }

            Console->Warning(cf::va("InData.Receive() returned WSA fail code %u (%s). Packet ignored.\n", E.Error, ErrorString));
        }
    }


    // Frage Keyboard- und Mouse-Input ab und leite diese in die ChatLine, Console oder das Game weiter.
    // Danach sende die Game-Keys (und Heading usw.) an den Server.
    // The contents of this if-block used to be in the (now obsolete) EventManager(float FrameTime) method.
    if (World)
    {
        cf::GuiSys::GuiI* ActiveGui=cf::GuiSys::GuiMan->GetTopmostActiveAndInteractive();
        assert(ActiveGui!=NULL);    // The GUI of the client for the world output must always be there.

        // This is unfortunately needed, because the users last click (in the GUI) that brought us here may still have the "LMB up" event pending.
        WasLMBOnceUp|=(SingleOpenGLWindow->GetMouseButtonState() & 0x1)==0;

        // FIXME: Should test for ActiveGui==ClientGui instead of ActiveGui->GetScriptName()=="".
        if (ActiveGui->GetScriptName()=="" && WasLMBOnceUp)
        {
            if (SingleOpenGLWindow->GetMouseButtonState() & 0x1) PlayerCommand.Keys|=PCK_Fire1;  // Button 0
            if (SingleOpenGLWindow->GetMouseButtonState() & 0x6) PlayerCommand.Keys|=PCK_Fire2;  // Button 1 oder 2

            // Alle anderen Keys via KeyboardState bestimmen und über die volle Frametime anwenden.
            // Später evtl. mal die echte Zeit vom Buffer einsetzen!
            // Mit anderen Worten: Diesen Kram mit in die obige Buffer-Schleife nehmen!
            // Player movement / state
         // if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RCONTROL   ]) ;                                       // R_Strg   Run
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RSHIFT     ] ||                                       // R_Shift  Stealth
                SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_LSHIFT     ]) PlayerCommand.Keys|=PCK_Walk;           // L_Shift  Stealth
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_UP         ] ||                                       // Up       Walk forward
                SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_W          ]) PlayerCommand.Keys|=PCK_MoveForward;    // W        Walk forward
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_DOWN       ] ||                                       // Down     Walk backward
                SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_S          ]) PlayerCommand.Keys|=PCK_MoveBackward;   // S        Walk backward
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_A          ] ||                                       // A        Strafe left
                SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_COMMA      ]) PlayerCommand.Keys|=PCK_StrafeLeft;     // ,        Strafe left
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_D          ] ||                                       // D        Strafe right
                SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PERIOD     ]) PlayerCommand.Keys|=PCK_StrafeRight;    // .        Strafe right
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_R          ]) PlayerCommand.Keys|=PCK_Fire1;          // R        Fire/Respawn
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RETURN     ] ||                                       // RETURN   Use
                SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPADENTER]) PlayerCommand.Keys|=PCK_Use;            // ENTER    Use
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_LEFT       ]) PlayerCommand.Keys|=PCK_TurnLeft;       // Left     Turn left
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RIGHT      ]) PlayerCommand.Keys|=PCK_TurnRight;      // Right    Turn right
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGDN       ]) PlayerCommand.Keys|=PCK_LookUp;         //          Look up
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGUP       ]) PlayerCommand.Keys|=PCK_LookDown;       //          Look down
         // if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_HOME       ]) PlayerCommand.Keys|=PCK_BankCW          //          Bank CW
         // if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_INSERT     ]) PlayerCommand.Keys|=PCK_BankCCW;        //          Bank CCW
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_END        ]) PlayerCommand.Keys|=PCK_CenterView;
        }


        PlayerCommand.FrameTime=FrameTime;
        PlayerCommand.Nr       =GameProtocol.GetNextOutgoingSequenceNr();

        // PlayerCommand an Server senden
        UnreliableData.WriteByte (CS1_PlayerCommand);
        UnreliableData.WriteFloat(FrameTime);
        UnreliableData.WriteLong (PlayerCommand.Keys);
        UnreliableData.WriteWord (PlayerCommand.DeltaHeading);
        UnreliableData.WriteWord (PlayerCommand.DeltaPitch);
     // UnreliableData.WriteWord (PlayerCommand.DeltaBank);
     // UnreliableData.WriteLong (PlayerCommand.Nr);

        // Führe für unseren Entity die Prediction durch
        World->OurEntity_Predict(PlayerCommand, PlayerCommand.Nr /* next outgoing sequence number */);

        if (m_PathRecorder)
            m_PathRecorder->WritePath(World->OurEntity_GetState(UsePrediction.GetValueBool()), FrameTime);

        PlayerCommand=PlayerCommandT();     // Clear the PlayerCommand.
    }


    try
    {
        GameProtocol.GetTransmitData(ReliableDatas, UnreliableData.Data).Send(Client.Socket, Client.ServerAddress);
    }
    catch (const GameProtocol1T::MaxMsgSizeExceeded& /*E*/) { EnqueueString("FATAL WARNING: caught a GameProtocol1T::MaxMsgSizeExceeded exception!\n"); }
    catch (const NetDataT::WinSockAPIError&            E  ) { EnqueueString("FATAL WARNING: caught a NetDataT::WinSockAPIError exception (error %u)!\n", E.Error); }
    catch (const NetDataT::MessageLength&              E  ) { EnqueueString("FATAL WARNING: caught a NetDataT::MessageLength exception (wanted %u, actual %u)!\n", E.Wanted, E.Actual); }

    ReliableDatas.Clear();
    UnreliableData=NetDataT();
}
