/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

/*************************/
/*** Cafu Model Viewer ***/
/*************************/

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "OpenGL/OpenGLWindow.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "Math3D/Matrix.hpp"
#include "Models/Model_proxy.hpp"
#include "Util/Util.hpp"
#include "PlatformAux.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#else
#include <dlfcn.h>
#define FreeLibrary dlclose
#endif

#include <time.h>
#include <stdio.h>
#include <string.h>


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

ConsoleInterpreterI* ConsoleInterpreter=NULL;
MaterialManagerI*    MaterialManager   =NULL;


std::string              BaseDirectoryName="Games/DeathMatch";
MatSys::RenderMaterialT* SolidColorRenderMat=NULL;
MatSys::RenderMaterialT* GroundPlaneRenderMat=NULL;

ModelT* Model          =NULL;
ModelT* SubModel       =NULL;
int     ModelSequenceNr=0;
float   ModelFrameNr   =0.0f;


// AmbientLightingInfo - Elements are LightPosition (unused), LightColor, LightRadius (unused), and EyePosition (unused).
// DynamicLightingInfo - Elements are LightPosition (world space), LightColor, LightRadius (world space), and EyePosition (world space).
// RotX, RotY, RotZ    - Model rotation angles.
void DrawPolygons(const ArrayT<VectorT>& AmbientLightingInfo, const ArrayT<VectorT>& DynamicLightingInfo,
                  const float RotX, const float RotY, const float RotZ,
                  const bool DrawLightSource, const bool DrawGroundPlane)
{
    // 1. Draw world-space stuff.

    // World-space and model-space are identical here, so we can directly set the world-space light source parameters as model-space parameters.
    MatSys::Renderer->SetCurrentLightSourcePosition((float)DynamicLightingInfo[0].x, (float)DynamicLightingInfo[0].y, (float)DynamicLightingInfo[0].z);
    MatSys::Renderer->SetCurrentLightSourceRadius((float)DynamicLightingInfo[2].x);
    MatSys::Renderer->SetCurrentLightSourceDiffuseColor ((float)DynamicLightingInfo[1].x, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].z);
    MatSys::Renderer->SetCurrentLightSourceSpecularColor((float)DynamicLightingInfo[1].z, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].x);
    MatSys::Renderer->SetCurrentEyePosition((float)DynamicLightingInfo[3].x, (float)DynamicLightingInfo[3].y, (float)DynamicLightingInfo[3].z);

    if (DrawLightSource && MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        // Draw light source in world- (actually: eye-) space.
        MatSys::Renderer->SetCurrentMaterial(SolidColorRenderMat);

        MatSys::MeshT LightSourceMesh(MatSys::MeshT::Lines);
        LightSourceMesh.Vertices.PushBackEmpty(6);

        LightSourceMesh.Vertices[0].SetOrigin((float)DynamicLightingInfo[0].x+50.0f, (float)DynamicLightingInfo[0].y, (float)DynamicLightingInfo[0].z); LightSourceMesh.Vertices[0].SetColor((float)DynamicLightingInfo[1].x, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].z);
        LightSourceMesh.Vertices[1].SetOrigin((float)DynamicLightingInfo[0].x-50.0f, (float)DynamicLightingInfo[0].y, (float)DynamicLightingInfo[0].z); LightSourceMesh.Vertices[1].SetColor((float)DynamicLightingInfo[1].x, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].z);

        LightSourceMesh.Vertices[2].SetOrigin((float)DynamicLightingInfo[0].x, (float)DynamicLightingInfo[0].y+50.0f, (float)DynamicLightingInfo[0].z); LightSourceMesh.Vertices[2].SetColor((float)DynamicLightingInfo[1].x, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].z);
        LightSourceMesh.Vertices[3].SetOrigin((float)DynamicLightingInfo[0].x, (float)DynamicLightingInfo[0].y-50.0f, (float)DynamicLightingInfo[0].z); LightSourceMesh.Vertices[3].SetColor((float)DynamicLightingInfo[1].x, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].z);

        LightSourceMesh.Vertices[4].SetOrigin((float)DynamicLightingInfo[0].x, (float)DynamicLightingInfo[0].y, (float)DynamicLightingInfo[0].z+50.0f); LightSourceMesh.Vertices[4].SetColor((float)DynamicLightingInfo[1].x, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].z);
        LightSourceMesh.Vertices[5].SetOrigin((float)DynamicLightingInfo[0].x, (float)DynamicLightingInfo[0].y, (float)DynamicLightingInfo[0].z-50.0f); LightSourceMesh.Vertices[5].SetColor((float)DynamicLightingInfo[1].x, (float)DynamicLightingInfo[1].y, (float)DynamicLightingInfo[1].z);

        MatSys::Renderer->RenderMesh(LightSourceMesh);
    }

    if (DrawGroundPlane && GroundPlaneRenderMat!=NULL && (MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT || MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING))
    {
        // Draw ground plane in world- (actually: eye-) space.
        MatSys::Renderer->SetCurrentMaterial(GroundPlaneRenderMat);

        MatSys::MeshT GroundPlaneMesh(MatSys::MeshT::TriangleFan);
        GroundPlaneMesh.Vertices.PushBackEmpty(4);

        GroundPlaneMesh.Vertices[0].SetOrigin(-400.0, -400.0, -300.0); GroundPlaneMesh.Vertices[0].SetTextureCoord(0.0, 1.0); GroundPlaneMesh.Vertices[0].SetNormal(0.0, 0.0, 1.0); GroundPlaneMesh.Vertices[0].SetTangent(1.0, 0.0, 0.0); GroundPlaneMesh.Vertices[0].SetBiNormal(0.0, -1.0, 0.0);
        GroundPlaneMesh.Vertices[1].SetOrigin(-400.0,  400.0, -300.0); GroundPlaneMesh.Vertices[1].SetTextureCoord(0.0, 0.0); GroundPlaneMesh.Vertices[1].SetNormal(0.0, 0.0, 1.0); GroundPlaneMesh.Vertices[1].SetTangent(1.0, 0.0, 0.0); GroundPlaneMesh.Vertices[1].SetBiNormal(0.0, -1.0, 0.0);
        GroundPlaneMesh.Vertices[2].SetOrigin( 400.0,  400.0, -300.0); GroundPlaneMesh.Vertices[2].SetTextureCoord(1.0, 0.0); GroundPlaneMesh.Vertices[2].SetNormal(0.0, 0.0, 1.0); GroundPlaneMesh.Vertices[2].SetTangent(1.0, 0.0, 0.0); GroundPlaneMesh.Vertices[2].SetBiNormal(0.0, -1.0, 0.0);
        GroundPlaneMesh.Vertices[3].SetOrigin( 400.0, -400.0, -300.0); GroundPlaneMesh.Vertices[3].SetTextureCoord(1.0, 1.0); GroundPlaneMesh.Vertices[3].SetNormal(0.0, 0.0, 1.0); GroundPlaneMesh.Vertices[3].SetTangent(1.0, 0.0, 0.0); GroundPlaneMesh.Vertices[3].SetBiNormal(0.0, -1.0, 0.0);

        MatSys::Renderer->RenderMesh(GroundPlaneMesh);
    }


    // 2. Draw model-space stuff.
    {
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        const Vector3dT ModelPos=-Model->GetBB(ModelSequenceNr, 0.0f /*ModelFrameNr*/).GetCenter().AsVectorOfDouble();

        // Set up model space, both for the model, the light and the eye.
        ArrayT<VectorT> ModelSpace_DLI=DynamicLightingInfo;

        MatSys::Renderer->RotateX  (MatSys::RendererI::MODEL_TO_WORLD, RotX); ModelSpace_DLI[0]=ModelSpace_DLI[0].GetRotX(-RotX); ModelSpace_DLI[3]=ModelSpace_DLI[3].GetRotX(-RotX);
        MatSys::Renderer->RotateY  (MatSys::RendererI::MODEL_TO_WORLD, RotY); ModelSpace_DLI[0]=ModelSpace_DLI[0].GetRotY(-RotY); ModelSpace_DLI[3]=ModelSpace_DLI[3].GetRotY(-RotY);
        MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD, RotZ); ModelSpace_DLI[0]=ModelSpace_DLI[0].GetRotZ(-RotZ); ModelSpace_DLI[3]=ModelSpace_DLI[3].GetRotZ(-RotZ);
        MatSys::Renderer->Scale    (MatSys::RendererI::MODEL_TO_WORLD, 5.0f); ModelSpace_DLI[0]=scale(ModelSpace_DLI[0], 1.0/5.0); ModelSpace_DLI[2].x/=5.0; ModelSpace_DLI[3]=scale(ModelSpace_DLI[3], 1.0/5.0);
        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, (float)ModelPos.x, (float)ModelPos.y, (float)ModelPos.z); ModelSpace_DLI[0]=ModelSpace_DLI[0]-ModelPos; ModelSpace_DLI[3]=ModelSpace_DLI[3]-ModelPos;

        MatSys::Renderer->SetCurrentAmbientLightColor((float)AmbientLightingInfo[1].x, (float)AmbientLightingInfo[1].y, (float)AmbientLightingInfo[1].z);

        MatSys::Renderer->SetCurrentLightSourcePosition((float)ModelSpace_DLI[0].x, (float)ModelSpace_DLI[0].y, (float)ModelSpace_DLI[0].z);
        MatSys::Renderer->SetCurrentLightSourceRadius((float)ModelSpace_DLI[2].x);
        MatSys::Renderer->SetCurrentLightSourceDiffuseColor ((float)ModelSpace_DLI[1].x, (float)ModelSpace_DLI[1].y, (float)ModelSpace_DLI[1].z);
        MatSys::Renderer->SetCurrentLightSourceSpecularColor((float)ModelSpace_DLI[1].z, (float)ModelSpace_DLI[1].y, (float)ModelSpace_DLI[1].x);
        MatSys::Renderer->SetCurrentEyePosition((float)ModelSpace_DLI[3].x, (float)ModelSpace_DLI[3].y, (float)ModelSpace_DLI[3].z);

        Model->Draw(ModelSequenceNr, ModelFrameNr, 0.0f, SubModel);

        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    }
}


// AmbientLightingInfo - Elements are LightPosition (unused), LightColor, LightRadius (unused), and EyePosition (unused).
// DynamicLightingInfo - Elements are LightPosition (world space), LightColor, LightRadius (world space), and EyePosition (world space).
// RotX, RotY, RotZ    - Model rotation angles.
void DrawScene(const ArrayT<VectorT>& AmbientLightingInfo, const ArrayT<VectorT>& DynamicLightingInfo,
               const float RotX, const float RotY, const float RotZ,
               const bool DrawLightSource, const bool DrawGroundPlane,
               const bool EnableShadows)
{
 // MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, MatrixT::GetRotateXMatrix(-90.0f));   // Rotate coordinate system axes to Cafu standard.
    MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -(float)DynamicLightingInfo[3].x, -(float)DynamicLightingInfo[3].y, -(float)DynamicLightingInfo[3].z); // World to Camera space transform.

    // 1. Draw ambient pass.
    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    DrawPolygons(AmbientLightingInfo, DynamicLightingInfo, RotX, RotY, RotZ, DrawLightSource, DrawGroundPlane);

    // for each light source...
    {
        // 2. Draw stencil shadow pass.
        if (EnableShadows)
        {
            MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::STENCILSHADOW);
            DrawPolygons(AmbientLightingInfo, DynamicLightingInfo, RotX, RotY, RotZ, DrawLightSource, DrawGroundPlane);
        }


        // 3. Draw dynamic lighting pass.
        MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::LIGHTING);
        DrawPolygons(AmbientLightingInfo, DynamicLightingInfo, RotX, RotY, RotZ, DrawLightSource, DrawGroundPlane);
    }
}


void BenchMark(const bool EnableShadows, const std::string& ModelName, const std::string& SubModelName)
{
    const unsigned long NR_OF_FRAMES=2500;
    double              Results1[2]={ 0.0, 0.0 };
    double              Results2[2]={ 0.0, 0.0 };

    const float LightSourceElevation=   0.0;      // Ranges from -90.0 (south pole) to +90.0 (north pole).
    const float LightSourceAzimut   = 130.0;      // Ranges from -180.0 to +180.0.
    const float LightSourceDistance = 800.0;
    const float LightSourceRadius   =3000.0;

    ArrayT<VectorT> AmbientLightingInfo;
    AmbientLightingInfo.PushBack(VectorT());
    AmbientLightingInfo.PushBack(VectorT(0.0, 0.1, 0.2));   // Light color.
    AmbientLightingInfo.PushBack(VectorT());
    AmbientLightingInfo.PushBack(VectorT());

    ArrayT<VectorT> DynamicLightingInfo;
    DynamicLightingInfo.PushBack(VectorT(0.0, LightSourceDistance, 0.0).GetRotX(LightSourceElevation).GetRotZ(LightSourceAzimut));  // Light position.
    DynamicLightingInfo.PushBack(VectorT(1.0, 0.9, 0.8));                                                                           // Light color.
    DynamicLightingInfo.PushBack(VectorT(LightSourceRadius, 0.0, 0.0));                                                             // Light radius.
    DynamicLightingInfo.PushBack(VectorT(0.0, -800.0, 0.0));                                                                        // Eye position.

    const VectorT ClearColor=VectorT(0.0, 0.0, 0.4);
    MatSys::Renderer->ClearColor((float)ClearColor.x, (float)ClearColor.y, (float)ClearColor.z, 0.0f);


    for (unsigned long PhaseNr=0; PhaseNr<3; PhaseNr++)
    {
        // Phase 0 is pre-caching (not timed).
        // Phase 1 is without animation.
        // Phase 2 is with animation.
        const unsigned long FRAME_STEP=(PhaseNr==0) ? 10 : 1;

        switch (PhaseNr)
        {
            case 0:
            case 1:
                ModelSequenceNr=0;
                ModelFrameNr   =0.0f;
                break;

            case 2:
                ModelSequenceNr=9;
                ModelFrameNr   =0.0f;

                if (ModelSequenceNr>=int(Model->GetNrOfSequences())) ModelSequenceNr=1;
                if (ModelSequenceNr>=int(Model->GetNrOfSequences())) ModelSequenceNr=0;
                break;
        }

        TimerT TrueTimer;
        TimerT Timer;
        Timer.GetSecondsSinceLastCall();

        for (unsigned long FrameNr=0; FrameNr<NR_OF_FRAMES; FrameNr+=FRAME_STEP)
        {
            // Rufe die Nachrichten der Windows-Nachrichtenschlange ab.
            // (Der BenchMark kann aber nicht unterbrochen werden.)
            if (SingleOpenGLWindow->HandleWindowMessages())
            {
                printf("The benchmark was user aborted.\n");
                return;
            }

            if (PhaseNr==2) ModelFrameNr=Model->AdvanceFrameNr(ModelSequenceNr, float(FrameNr)/float(NR_OF_FRAMES)*40.0f, 0.0, true);

            TrueTimer.GetSecondsSinceLastCall();
            MatSys::Renderer->BeginFrame(Timer.GetSecondsSinceCtor());
            DrawScene(AmbientLightingInfo, DynamicLightingInfo, 0.0, 0.0, float(FrameNr)/float(NR_OF_FRAMES)*360.0f, false /*DrawLightSource*/, true /*DrawGroundPlane*/, EnableShadows);
            if (PhaseNr>=1) Results2[PhaseNr-1]+=TrueTimer.GetSecondsSinceLastCall();

            MatSys::Renderer->EndFrame();
            SingleOpenGLWindow->SwapBuffers();
        }

        if (PhaseNr>=1)
        {
            Results1[PhaseNr-1]=Timer.GetSecondsSinceLastCall();
        }
    }


    // Print out results.
    char   DateTime[256]="unknown";
    char   HostName[256]="unknown";
    time_t Time         =time(NULL);

    strftime(DateTime, 256, "%d.%m.%Y %H:%M", localtime(&Time));
    DateTime[255]=0;

#ifdef _WIN32
    const char* OsName="Win32";
    unsigned long Dummy=256;
    if (!GetComputerName(HostName, &Dummy)) sprintf(HostName, "unknown (look-up failed).");
#else
    const char* OsName="Linux";
    // This function also works on Windows, but sadly requires calls to 'WSAStartup()' and 'WSACleanup()'.
    if (gethostname(HostName, 256)) sprintf(HostName, "unknown (look-up failed).");
#endif
    HostName[255]=0;

    printf("Date/Time   : %-16s\n", DateTime);
    printf("Renderer    : %s dynamic lighting %s\n", MatSys::Renderer->GetDescription(), EnableShadows ? "" : "(SHADOWS DISABLED!)");
    printf("FPS1 (still): %7.2f  (%lu frames took %7.3fs)  (true time: %7.2f FPS / %8.4fs)\n", double(NR_OF_FRAMES)/Results1[0], NR_OF_FRAMES, Results1[0], double(NR_OF_FRAMES)/Results2[0], Results2[0]);
    printf("FPS2 (anim ): %7.2f  (%lu frames took %7.3fs)  (true time: %7.2f FPS / %8.4fs)\n", double(NR_OF_FRAMES)/Results1[1], NR_OF_FRAMES, Results1[1], double(NR_OF_FRAMES)/Results2[1], Results2[1]);
    printf("Model       : %s\n", ModelName.c_str());
    printf("Submodel    : %s\n", SubModelName!="" ? SubModelName.c_str() : "[none]");
    printf("Host        : %s   (OS is %s)\n", HostName, OsName);
 // printf("GL_RENDERER : %s\n", glGetString(GL_RENDERER));
    printf("\n");
}


void InteractiveViewer(const bool EnableShadows, const bool EnableAnim)
{
    bool RotXOn=false; float RotX=  0.0;
    bool RotYOn=false; float RotY=  0.0;
    bool RotZOn=false; float RotZ=  0.0;

          float LightSourceElevation=   0.0;      // Ranges from -90.0 (south pole) to +90.0 (north pole).
          float LightSourceAzimut   =  90.0;      // Ranges from -180.0 to +180.0.
          float LightSourceDistance = 800.0;
    const float LightSourceRadius   =3000.0;
    bool        DrawLightSource     =true;

    ArrayT<VectorT> AmbientLightingInfo;
    AmbientLightingInfo.PushBack(VectorT());
    AmbientLightingInfo.PushBack(VectorT(0.0, 0.1, 0.2));   // Light color.
    AmbientLightingInfo.PushBack(VectorT());
    AmbientLightingInfo.PushBack(VectorT());

    ArrayT<VectorT> DynamicLightingInfo;
    DynamicLightingInfo.PushBack(VectorT(0.0, LightSourceDistance, 0.0).GetRotX(LightSourceElevation).GetRotZ(LightSourceAzimut));  // Light position.
    DynamicLightingInfo.PushBack(VectorT(1.0, 0.9, 0.8));                                                                           // Light color.
    DynamicLightingInfo.PushBack(VectorT(LightSourceRadius, 0.0, 0.0));                                                             // Light radius.
    DynamicLightingInfo.PushBack(VectorT(0.0, -1000.0, 0.0));                                                                       // Eye position.

    VectorT ClearColor=VectorT(0.0, 0.0, 0.4);
    MatSys::Renderer->ClearColor((float)ClearColor.x, (float)ClearColor.y, (float)ClearColor.z, 0.0f);


    // Main loop
    TimerT Timer;

    while (true)
    {
        // Rufe die Nachrichten der Windows-Nachrichtenschlange ab.
        if (SingleOpenGLWindow->HandleWindowMessages()) break;

        // Compute and set the lighting parameters.
        DynamicLightingInfo[0]=VectorT(0.0, LightSourceDistance, 0.0).GetRotX(LightSourceElevation).GetRotZ(LightSourceAzimut);

        // Draw the model.
        double DeltaTime=Timer.GetSecondsSinceLastCall();
        if (EnableAnim) ModelFrameNr=Model->AdvanceFrameNr(ModelSequenceNr, ModelFrameNr, float(DeltaTime), true);

        MatSys::Renderer->BeginFrame(Timer.GetSecondsSinceCtor());
        DrawScene(AmbientLightingInfo, DynamicLightingInfo, RotX, RotY, RotZ, DrawLightSource, true /*DrawGroundPlane*/, EnableShadows);
        MatSys::Renderer->EndFrame();
        SingleOpenGLWindow->SwapBuffers();


        CaKeyboardEventT KE;
        CaMouseEventT    ME;
        bool             QuitProgram=false;

        while (SingleOpenGLWindow->GetNextKeyboardEvent(KE)>0)
        {
            if (KE.Type!=CaKeyboardEventT::CKE_KEYDOWN) continue;

            if (KE.Key==CaKeyboardEventT::CK_ESCAPE) QuitProgram=true;

            switch (KE.Key)
            {
                case CaKeyboardEventT::CK_X: RotXOn=!RotXOn; break;
                case CaKeyboardEventT::CK_Y: RotYOn=!RotYOn; break;
                case CaKeyboardEventT::CK_Z: RotZOn=!RotZOn; break;
                case CaKeyboardEventT::CK_K: DrawLightSource=!DrawLightSource; break;
                case CaKeyboardEventT::CK_ADD     : ModelSequenceNr++; if (ModelSequenceNr>=int(Model->GetNrOfSequences())) ModelSequenceNr=0; break;
                case CaKeyboardEventT::CK_SUBTRACT: ModelSequenceNr--; if (ModelSequenceNr<0) ModelSequenceNr=Model->GetNrOfSequences()-1; break;
                default: break;
            }
        }

        while (SingleOpenGLWindow->GetNextMouseEvent(ME)>0)
        {
            switch (ME.Type)
            {
                case CaMouseEventT::CM_MOVE_X:   // x-axis
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_R]) RotZ+=int(ME.Amount);
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_L]) LightSourceAzimut+=0.5f*int(ME.Amount);
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_M]) DynamicLightingInfo[3].x+=int(ME.Amount);
                    break;

                case CaMouseEventT::CM_MOVE_Y:   // y-axis
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_R      ]) RotX+=int(ME.Amount);
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_L      ]) { float& e=LightSourceElevation; e-=0.5f*int(ME.Amount); if (e>90.0f) e=90.0f; if (e<-90.0f) e=-90.0f; }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_M      ]) DynamicLightingInfo[3].z-=int(ME.Amount);
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD1]) { double& r=DynamicLightingInfo[1].x; r-=int(ME.Amount)/255.0/2.0; if (r>1) r=1; if (r<0) r=0; }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD2]) { double& g=DynamicLightingInfo[1].y; g-=int(ME.Amount)/255.0/2.0; if (g>1) g=1; if (g<0) g=0; }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD3]) { double& b=DynamicLightingInfo[1].z; b-=int(ME.Amount)/255.0/2.0; if (b>1) b=1; if (b<0) b=0; }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD4]) { double& r=AmbientLightingInfo[1].x; r-=int(ME.Amount)/255.0/2.0; if (r>1) r=1; if (r<0) r=0; }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD5]) { double& g=AmbientLightingInfo[1].y; g-=int(ME.Amount)/255.0/2.0; if (g>1) g=1; if (g<0) g=0; }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD6]) { double& b=AmbientLightingInfo[1].z; b-=int(ME.Amount)/255.0/2.0; if (b>1) b=1; if (b<0) b=0; }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD7]) { double& r=ClearColor.x; r-=int(ME.Amount)/255.0/2.0; if (r>1) r=1; if (r<0) r=0; MatSys::Renderer->ClearColor((float)ClearColor.x, (float)ClearColor.y, (float)ClearColor.z, 0.0); }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD8]) { double& g=ClearColor.y; g-=int(ME.Amount)/255.0/2.0; if (g>1) g=1; if (g<0) g=0; MatSys::Renderer->ClearColor((float)ClearColor.x, (float)ClearColor.y, (float)ClearColor.z, 0.0); }
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_NUMPAD9]) { double& b=ClearColor.z; b-=int(ME.Amount)/255.0/2.0; if (b>1) b=1; if (b<0) b=0; MatSys::Renderer->ClearColor((float)ClearColor.x, (float)ClearColor.y, (float)ClearColor.z, 0.0); }
                    break;

                case CaMouseEventT::CM_MOVE_Z:   // z-axis (mouse wheel)
                    if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_L]) LightSourceDistance-=0.5f*int(ME.Amount);
                                                     else DynamicLightingInfo[3].y+=int(ME.Amount);
                    break;

                default:
                    // Ignore all other ME types.
                    break;
            }
        }

        if (QuitProgram) break;

        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_W]) DynamicLightingInfo[3].y+=100*DeltaTime;
        if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_S]) DynamicLightingInfo[3].y-=100*DeltaTime;

        if (RotXOn) RotX+=90.0f*float(DeltaTime);
        if (RotYOn) RotY+=90.0f*float(DeltaTime);
        if (RotZOn) RotZ+=90.0f*float(DeltaTime);
    }
}


void Usage()
{
    printf("\nUSAGE: ModelViewer ModelName [SubModelName] [-print] [-bm] [-noS] [-noAnim]\n");
    printf("\n");
    printf("-print    Prints model data to stdout.\n");
    printf("-bm       Runs a benchmark (and prints results to stdout).\n");
    printf("-noS      If dynamic lighting is enabled, this disables the shadows.\n");
    printf("-noAnim   In interactive mode, this turns model sequence animations OFF.\n");
    printf("          Intended for code testing only, and not very useful otherwise.\n");

    exit(1);
}


int main(int ArgC, char* ArgV[])
{
    // Parse command line.
    std::string ModelName    ="";
    std::string SubModelName ="";
    bool        DoBenchMark  =false;
    bool        PrintModel   =false;
    bool        EnableShadows=true;
    bool        EnableAnim   =true;

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    for (int ArgNr=1; ArgNr<ArgC; ArgNr++)
    {
             if (_stricmp(ArgV[ArgNr], "-print" )==0) PrintModel =true;
        else if (_stricmp(ArgV[ArgNr], "-bm"    )==0) DoBenchMark=true;
        else if (_stricmp(ArgV[ArgNr], "-noS"   )==0) EnableShadows=false;
        else if (_stricmp(ArgV[ArgNr], "-noAnim")==0) EnableAnim=false;
        else if (ArgV[ArgNr][0]=='-') Usage();
        else if (   ModelName=="")    ModelName=ArgV[ArgNr];
        else if (SubModelName=="") SubModelName=ArgV[ArgNr];
        else Usage();
    }

    if (ModelName=="") Usage();


    // Setup the global Material Manager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;


    // Register the material script files with the material manager.
#if 1
    // It's just too often that I wonder why a material has not been found... so now let's load them all, always!
    MaterialManager->RegisterMaterialScriptsInDir(BaseDirectoryName+"/Materials", BaseDirectoryName+"/");
#else
    MaterialManager->RegisterMaterialScriptsInDir(BaseDirectoryName+"/Materials/Models", BaseDirectoryName+"/");
    MaterialManager->RegisterMaterialScript(BaseDirectoryName+"/Materials/meta.cmat", BaseDirectoryName+"/");    // For meta materials.
    MaterialManager->RegisterMaterialScript(BaseDirectoryName+"/Materials/misc.cmat", BaseDirectoryName+"/");    // For "meta/model_replacement".
    MaterialManager->RegisterMaterialScript(BaseDirectoryName+"/Materials/WilliH.cmat", BaseDirectoryName+"/");  // For the ground plane.
    MaterialManager->RegisterMaterialScript(BaseDirectoryName+"/Materials/Test.cmat", BaseDirectoryName+"/");    // For, well, tests.
#endif


    // Open an OpenGL window.
    const char* ErrorMsg=SingleOpenGLWindow->Open("Cafu Model Viewer 1.2", DoBenchMark ? 1024 : 800, DoBenchMark ? 768 : 600, 32, DoBenchMark);

    if (ErrorMsg)
    {
        printf("Unable to open OpenGL window: %s\n", ErrorMsg);
        return 0;
    }


    // Get the renderer with the highest preference number that is supported.
    HMODULE RendererDLL;
    MatSys::Renderer=PlatformAux::GetBestRenderer(RendererDLL);

    if (MatSys::Renderer==NULL || RendererDLL==NULL) { printf("No renderer loaded.\n"); SingleOpenGLWindow->Close(); return 0; }
    MatSys::Renderer->Initialize();


    // Get the TextureMapManager from the RendererDLL.
    MatSys::TextureMapManagerI* TextureMapManager=PlatformAux::GetTextureMapManager(RendererDLL);

    if (TextureMapManager==NULL) { printf("No TextureMapManager obtained.\n"); FreeLibrary(RendererDLL); SingleOpenGLWindow->Close(); return 0; }


    MaterialT* GroundPlaneMaterial=MaterialManager->GetMaterial("Textures/WilliH/rock01b");
    if (GroundPlaneMaterial!=NULL)
    {
        MaterialT GPM=*GroundPlaneMaterial;

        GPM.LightMapComp=MapCompositionT();
        GroundPlaneRenderMat=MatSys::Renderer->RegisterMaterial(&GPM);
    }
    else printf("Sorry, material for ground plane not found!\n");


    MaterialT SolidColorMaterial;
    SolidColorMaterial.UseMeshColors=true;

    SolidColorRenderMat=MatSys::Renderer->RegisterMaterial(&SolidColorMaterial);


    // Create a very simple lightmap for the materials that need one, and register it with the renderer.
    // This is just in case they (users) apply a material to models that have lightmaps.
 // char Data[]={ 255, 255, 255, 255, 255, 255, 0, 0,
 //               255, 255, 255, 255, 255, 255, 0, 0 };
    char Data[]={   0,   0,   0,   0,   0,   0, 0, 0,
                    0,   0,   0,   0,   0,   0, 0, 0 };

    MatSys::Renderer->SetCurrentLightMap(TextureMapManager->GetTextureMap2D(Data, 2, 2, 3, true, MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear)));
    MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.


    // Load models.
    Model   =new ModelProxyT(ModelName);
    SubModel=SubModelName!="" ? new ModelProxyT(SubModelName) : NULL;


    // Main action.
         if (PrintModel ) Model->Print();
    else if (DoBenchMark) BenchMark(EnableShadows, ModelName, SubModelName);
    else                  InteractiveViewer(EnableShadows, EnableAnim);


    // Clean-up.
    delete SubModel;
    delete Model;
    MatSys::Renderer->FreeMaterial(GroundPlaneRenderMat);
    MatSys::Renderer->FreeMaterial(SolidColorRenderMat);
    MatSys::Renderer->Release();
    MatSys::Renderer=NULL;
    FreeLibrary(RendererDLL);
    SingleOpenGLWindow->Close();
    return 0;
}
