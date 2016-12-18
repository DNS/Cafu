/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cstring>
#include <dlfcn.h>
#define FreeLibrary dlclose
#endif

#include "GLFW/glfw3.h"

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "Fonts/Font.hpp"
#include "MainWindow/glfwWindow.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "Math3D/Plane3.hpp"
#include "Math3D/Matrix.hpp"
#include "Templates/Array.hpp"
#include "Util/Util.hpp"
#include "PlatformAux.hpp"


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

ConsoleInterpreterI* ConsoleInterpreter=NULL;
MaterialManagerI*    MaterialManager   =NULL;


struct LightSourceInfoT
{
    bool  IsOn;
    float Pos[3];
    float Radius;
    float DiffColor[3];
    float SpecColor[3];

    LightSourceInfoT()
    {
        IsOn=false;
        Pos[0]=0.0; Pos[1]=0.0; Pos[2]=0.0;
        Radius=1000.0;
        DiffColor[0]=1.0; DiffColor[1]=1.0; DiffColor[2]=1.0;
        SpecColor[0]=1.0; SpecColor[1]=1.0; SpecColor[2]=1.0;
    }
};


const char*              BaseDirectoryName="Games/DeathMatch";
const char*              MaterialName=NULL;
ArrayT<const char*>      MaterialScriptNames;
MaterialT*               MyMaterial = NULL;
MatSys::RenderMaterialT* RenderMaterial = NULL;
ArrayT<LightSourceInfoT> LightSources;
float                    ViewerHeading = 0.0;


class ViewerWindowT : public cf::glfwWindowT
{
    public:

    ViewerWindowT(int width, int height, const char* title, GLFWmonitor* monitor=0)
        : glfwWindowT(width, height, title, monitor),
          old_xpos(-10000.0f)
    {
    }

    void FramebufferSizeEvent(int width, int height) override
    {
        // The framebuffer's width and height are given in pixels (not in screen coordinates).
        glViewport(0, 0, width, height);

        // Avoid division by zero below.
        if (height == 0)
            return;

        // Set the projection matrix.
        // Note that the field of view is in y-direction, a value of 67.5° may correspond to 90° in x-direction.
        const double FieldOfView = 67.5;
        const double AspectRatio = double(width) / double(height);
        const double Near        = 100.0;
     // const double Far         = 100000.0;
        const double cotanFOV    = 1.0 / tan(FieldOfView / 2.0 / 180.0 * 3.14159265359);

        // This is the OpenGL projection matrix with the "far" clip plane moved to infinity,
        // according to the NVidia paper about robust stencil buffered shadows.
        // Note that this matrix is actually transposed, as this is what 'glLoadMatrix()' expects.
        const double ProjMatInf[4][4] = {
            { cotanFOV/AspectRatio,      0.0,       0.0,  0.0 },
            {                  0.0, cotanFOV,       0.0,  0.0 },
            {                  0.0,      0.0,      -1.0, -1.0 },
            {                  0.0,      0.0, -2.0*Near,  0.0 }
        };

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixd(&ProjMatInf[0][0]);

        // A common (but not equivalent) alternative to the above:
        // glLoadIdentity();
        // gluPerspective(FieldOfView, AspectRatio, Near, Far);
    }

    void KeyEvent(int key, int scancode, int action, int mods) override
    {
        if (action != GLFW_PRESS)
            return;

        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9)
        {
            const int Index = key - GLFW_KEY_1;

            if (Index < int(LightSources.Size()))
                LightSources[Index].IsOn = !LightSources[Index].IsOn;
        }

        switch (key)
        {
            case GLFW_KEY_ESCAPE:
                setShouldClose(true);
                break;

            case GLFW_KEY_U:
            case GLFW_KEY_TAB:
                // Re-load the textures of the current material.
                // Because the RenderMaterial was the *only* material that we registered with the MatSys,
                // free'ing it is enough to get the reference count of its textures in the texture manager to 0.
                // This in turn deletes the textures from the tex-mans texture repository, which in turn is
                // forced to re-load the texture from disk when MyMaterial is re-registered.
                MatSys::Renderer->FreeMaterial(RenderMaterial);
                RenderMaterial = MatSys::Renderer->RegisterMaterial(MyMaterial);
                break;

            case GLFW_KEY_M:
                break;

            // case GLFW_KEY_CK_X: RotXOn = !RotXOn; break;
            // case GLFW_KEY_CK_Y: RotYOn = !RotYOn; break;
            // case GLFW_KEY_CK_Z: RotZOn = !RotZOn; break;
            // case GLFW_KEY_CK_K: DrawLightSource = !DrawLightSource; break;
            // case GLFW_KEY_CK_ADD:      Model->SetSequenceNr(++SequenceNr); break;
            // case GLFW_KEY_CK_SUBTRACT: Model->SetSequenceNr(--SequenceNr); break;
        }
    }

    void MouseMoveEvent(double xpos, double ypos) override
    {
        if (old_xpos == -10000.0f)
            old_xpos = float(xpos);

        const float dx = float(xpos) - old_xpos;
        old_xpos = float(xpos);

        if (isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
            ViewerHeading += dx / 2.0f;
    }


    private:

    float old_xpos;
};


static void error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error: %s\n", description);
}


void CreateMeshes(ArrayT<MatSys::MeshT>& Meshes, const MaterialT& MyMaterial)
{
    const float ar=float(MyMaterial.GetPixelSizeY())/float(MyMaterial.GetPixelSizeX());

    MatSys::MeshT* M=NULL;


    // The small inner cubic box.
    Meshes.PushBackEmpty();     // Front
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-100.0, 1000.0, -100.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal( 0.0, -1.0, 0.0); M->Vertices[0].SetTangent( 1.0,  0.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 100.0, 1000.0, -100.0*ar); M->Vertices[1].SetTextureCoord(1.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal( 0.0, -1.0, 0.0); M->Vertices[1].SetTangent( 1.0,  0.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 100.0, 1000.0,  100.0*ar); M->Vertices[2].SetTextureCoord(1.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal( 0.0, -1.0, 0.0); M->Vertices[2].SetTangent( 1.0,  0.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-100.0, 1000.0,  100.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal( 0.0, -1.0, 0.0); M->Vertices[3].SetTangent( 1.0,  0.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Right
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin( 100.0, 1000.0, -100.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal( 1.0,  0.0, 0.0); M->Vertices[0].SetTangent( 0.0,  1.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 100.0, 1200.0, -100.0*ar); M->Vertices[1].SetTextureCoord(1.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal( 1.0,  0.0, 0.0); M->Vertices[1].SetTangent( 0.0,  1.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 100.0, 1200.0,  100.0*ar); M->Vertices[2].SetTextureCoord(1.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal( 1.0,  0.0, 0.0); M->Vertices[2].SetTangent( 0.0,  1.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin( 100.0, 1000.0,  100.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal( 1.0,  0.0, 0.0); M->Vertices[3].SetTangent( 0.0,  1.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Back
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-100.0, 1200.0,  100.0*ar); M->Vertices[0].SetTextureCoord(1.0, 0.0); M->Vertices[0].SetLightMapCoord(2.0, 0.0); M->Vertices[0].SetNormal( 0.0,  1.0, 0.0); M->Vertices[0].SetTangent(-1.0,  0.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 100.0, 1200.0,  100.0*ar); M->Vertices[1].SetTextureCoord(0.0, 0.0); M->Vertices[1].SetLightMapCoord(0.0, 0.0); M->Vertices[1].SetNormal( 0.0,  1.0, 0.0); M->Vertices[1].SetTangent(-1.0,  0.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 100.0, 1200.0, -100.0*ar); M->Vertices[2].SetTextureCoord(0.0, 1.0); M->Vertices[2].SetLightMapCoord(0.0, 3.0); M->Vertices[2].SetNormal( 0.0,  1.0, 0.0); M->Vertices[2].SetTangent(-1.0,  0.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-100.0, 1200.0, -100.0*ar); M->Vertices[3].SetTextureCoord(1.0, 1.0); M->Vertices[3].SetLightMapCoord(2.0, 3.0); M->Vertices[3].SetNormal( 0.0,  1.0, 0.0); M->Vertices[3].SetTangent(-1.0,  0.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Left
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-100.0, 1200.0, -100.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal(-1.0,  0.0, 0.0); M->Vertices[0].SetTangent( 0.0, -1.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin(-100.0, 1000.0, -100.0*ar); M->Vertices[1].SetTextureCoord(1.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal(-1.0,  0.0, 0.0); M->Vertices[1].SetTangent( 0.0, -1.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin(-100.0, 1000.0,  100.0*ar); M->Vertices[2].SetTextureCoord(1.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal(-1.0,  0.0, 0.0); M->Vertices[2].SetTangent( 0.0, -1.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-100.0, 1200.0,  100.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal(-1.0,  0.0, 0.0); M->Vertices[3].SetTangent( 0.0, -1.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Top
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-100.0, 1000.0,  100.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal( 0.0,  0.0, 1.0); M->Vertices[0].SetTangent( 1.0,  0.0,  0.0); M->Vertices[0].SetBiNormal( 0.0, -1.0,  0.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 100.0, 1000.0,  100.0*ar); M->Vertices[1].SetTextureCoord(1.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal( 0.0,  0.0, 1.0); M->Vertices[1].SetTangent( 1.0,  0.0,  0.0); M->Vertices[1].SetBiNormal( 0.0, -1.0,  0.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 100.0, 1200.0,  100.0*ar); M->Vertices[2].SetTextureCoord(1.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal( 0.0,  0.0, 1.0); M->Vertices[2].SetTangent( 1.0,  0.0,  0.0); M->Vertices[2].SetBiNormal( 0.0, -1.0,  0.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-100.0, 1200.0,  100.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal( 0.0,  0.0, 1.0); M->Vertices[3].SetTangent( 1.0,  0.0,  0.0); M->Vertices[3].SetBiNormal( 0.0, -1.0,  0.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Bottom
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-100.0, 1200.0, -100.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetTangent( 1.0,  0.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  1.0,  0.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 100.0, 1200.0, -100.0*ar); M->Vertices[1].SetTextureCoord(1.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetTangent( 1.0,  0.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  1.0,  0.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 100.0, 1000.0, -100.0*ar); M->Vertices[2].SetTextureCoord(1.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetTangent( 1.0,  0.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  1.0,  0.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-100.0, 1000.0, -100.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetTangent( 1.0,  0.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  1.0,  0.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);


    // The outer walls.
    Meshes.PushBackEmpty();     // Back
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-1000.0,  5000.0, -1000.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal( 0.0, -1.0, 0.0); M->Vertices[0].SetTangent( 1.0,  0.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 1000.0,  5000.0, -1000.0*ar); M->Vertices[1].SetTextureCoord(1.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal( 0.0, -1.0, 0.0); M->Vertices[1].SetTangent( 1.0,  0.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 1000.0,  5000.0,  1000.0*ar); M->Vertices[2].SetTextureCoord(1.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal( 0.0, -1.0, 0.0); M->Vertices[2].SetTangent( 1.0,  0.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-1000.0,  5000.0,  1000.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal( 0.0, -1.0, 0.0); M->Vertices[3].SetTangent( 1.0,  0.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Left
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-1000.0, -5000.0, -1000.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal( 1.0,  0.0, 0.0); M->Vertices[0].SetTangent( 0.0,  1.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin(-1000.0,  5000.0, -1000.0*ar); M->Vertices[1].SetTextureCoord(5.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal( 1.0,  0.0, 0.0); M->Vertices[1].SetTangent( 0.0,  1.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin(-1000.0,  5000.0,  1000.0*ar); M->Vertices[2].SetTextureCoord(5.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal( 1.0,  0.0, 0.0); M->Vertices[2].SetTangent( 0.0,  1.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-1000.0, -5000.0,  1000.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal( 1.0,  0.0, 0.0); M->Vertices[3].SetTangent( 0.0,  1.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Front
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin(-1000.0, -5000.0,  1000.0*ar); M->Vertices[0].SetTextureCoord(1.0, 0.0); M->Vertices[0].SetLightMapCoord(2.0, 0.0); M->Vertices[0].SetNormal( 0.0,  1.0, 0.0); M->Vertices[0].SetTangent(-1.0,  0.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 1000.0, -5000.0,  1000.0*ar); M->Vertices[1].SetTextureCoord(0.0, 0.0); M->Vertices[1].SetLightMapCoord(0.0, 0.0); M->Vertices[1].SetNormal( 0.0,  1.0, 0.0); M->Vertices[1].SetTangent(-1.0,  0.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 1000.0, -5000.0, -1000.0*ar); M->Vertices[2].SetTextureCoord(0.0, 1.0); M->Vertices[2].SetLightMapCoord(0.0, 3.0); M->Vertices[2].SetNormal( 0.0,  1.0, 0.0); M->Vertices[2].SetTangent(-1.0,  0.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin(-1000.0, -5000.0, -1000.0*ar); M->Vertices[3].SetTextureCoord(1.0, 1.0); M->Vertices[3].SetLightMapCoord(2.0, 3.0); M->Vertices[3].SetNormal( 0.0,  1.0, 0.0); M->Vertices[3].SetTangent(-1.0,  0.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);

    Meshes.PushBackEmpty();     // Right
    M=&Meshes[Meshes.Size()-1];
    M->Type   =MatSys::MeshT::TriangleFan;
    M->Winding=MatSys::MeshT::CCW;
    M->Vertices.PushBackEmpty(4);
    M->Vertices[0].SetOrigin( 1000.0,  5000.0, -1000.0*ar); M->Vertices[0].SetTextureCoord(0.0, 1.0); M->Vertices[0].SetLightMapCoord(0.0, 3.0); M->Vertices[0].SetNormal(-1.0,  0.0, 0.0); M->Vertices[0].SetTangent( 0.0, -1.0,  0.0); M->Vertices[0].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[0].SetColor(1.0, 0.0, 0.0);
    M->Vertices[1].SetOrigin( 1000.0, -5000.0, -1000.0*ar); M->Vertices[1].SetTextureCoord(5.0, 1.0); M->Vertices[1].SetLightMapCoord(2.0, 3.0); M->Vertices[1].SetNormal(-1.0,  0.0, 0.0); M->Vertices[1].SetTangent( 0.0, -1.0,  0.0); M->Vertices[1].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[1].SetColor(0.0, 1.0, 0.0);
    M->Vertices[2].SetOrigin( 1000.0, -5000.0,  1000.0*ar); M->Vertices[2].SetTextureCoord(5.0, 0.0); M->Vertices[2].SetLightMapCoord(2.0, 0.0); M->Vertices[2].SetNormal(-1.0,  0.0, 0.0); M->Vertices[2].SetTangent( 0.0, -1.0,  0.0); M->Vertices[2].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[2].SetColor(0.0, 0.0, 1.0);
    M->Vertices[3].SetOrigin( 1000.0,  5000.0,  1000.0*ar); M->Vertices[3].SetTextureCoord(0.0, 0.0); M->Vertices[3].SetLightMapCoord(0.0, 0.0); M->Vertices[3].SetNormal(-1.0,  0.0, 0.0); M->Vertices[3].SetTangent( 0.0, -1.0,  0.0); M->Vertices[3].SetBiNormal( 0.0,  0.0, -1.0); M->Vertices[3].SetColor(1.0, 1.0, 0.0);
}


void DrawSilhouettes(MatSys::RendererI* Renderer, const ArrayT<MatSys::MeshT>& Meshes)
{
    // Assumptions:
    // 1. All meshes are TriangleFans.
    // 2. All TriangleFans are planar.
    VectorT LightPosition(Renderer->GetCurrentLightSourcePosition()[0], Renderer->GetCurrentLightSourcePosition()[1], Renderer->GetCurrentLightSourcePosition()[2]);

    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        const MatSys::MeshT& M=Meshes[MeshNr];

        if (M.Type!=MatSys::MeshT::TriangleFan) continue;
        if (M.Vertices.Size()<3) continue;

        // Is this mesh backfacing to the lightsource?
        const VectorT         Normal(M.Vertices[0].Normal[0], M.Vertices[0].Normal[1], M.Vertices[0].Normal[2]);
        const Plane3T<double> Plane(Normal, dot(Normal, VectorT(M.Vertices[0].Origin[0], M.Vertices[0].Origin[1], M.Vertices[0].Origin[2])));

        if (Plane.GetDistance(LightPosition)>=0.0) continue;

        // Yes, backfacing the lightsource.
        // Render the silhouette quads.
        MatSys::MeshT SilhouetteMesh(MatSys::MeshT::QuadStrip, MatSys::MeshT::CCW);

        for (unsigned long VertexNr=0; VertexNr<M.Vertices.Size(); VertexNr++)
        {
            // Yes, this could be further optimized...
            const VectorT  A(M.Vertices[VertexNr].Origin[0], M.Vertices[VertexNr].Origin[1], M.Vertices[VertexNr].Origin[2]);
            const VectorT LA=A-LightPosition;

            // Note that the order is reversed because we're dealing with a back-facing polygon as occluder!
            SilhouetteMesh.Vertices.PushBackEmpty(2);
            SilhouetteMesh.Vertices[VertexNr*2+0].SetOrigin(LA.x, LA.y, LA.z, 0.0);
            SilhouetteMesh.Vertices[VertexNr*2+1].SetOrigin( A.x,  A.y,  A.z, 1.0);
        }

        SilhouetteMesh.Vertices.PushBack(SilhouetteMesh.Vertices[0]);   // Close the silhouette for this occluder.
        SilhouetteMesh.Vertices.PushBack(SilhouetteMesh.Vertices[1]);

        Renderer->RenderMesh(SilhouetteMesh);


        // Render the occluders near cap (front-facing wrt. the light source).
        // (The MatSys expects vertices for front-facing polygons to be specified in CCW order.
        //  However, as we are considering a *back-facing* polygon as a occluder (oriented CW when looked at from behind),
        //  we have to reverse the order of its vertices in order to turn it into a CCW ordered, front-facing polygon.)
        MatSys::MeshT M_front(MatSys::MeshT::TriangleFan, MatSys::MeshT::CCW);

        M_front.Vertices.PushBackEmpty(M.Vertices.Size());

        for (unsigned long VertexNr=0; VertexNr<M.Vertices.Size(); VertexNr++)
            M_front.Vertices[M_front.Vertices.Size()-VertexNr-1].SetOrigin(M.Vertices[VertexNr].Origin[0],
                                                                           M.Vertices[VertexNr].Origin[1],
                                                                           M.Vertices[VertexNr].Origin[2], 1.0);

        Renderer->RenderMesh(M_front);


        // Render the occluders far cap (back-facing wrt. the light source).
        // As we are already dealing with a back-facing polygon, the vertex order is already as required,
        // we just have to project them to infinity as seen from the light source.
        MatSys::MeshT M_back(MatSys::MeshT::TriangleFan, MatSys::MeshT::CCW);

        M_back.Vertices.PushBackEmpty(M.Vertices.Size());

        for (unsigned long VertexNr=0; VertexNr<M.Vertices.Size(); VertexNr++)
            M_back.Vertices[VertexNr].SetOrigin(M.Vertices[VertexNr].Origin[0]-LightPosition.x,
                                                M.Vertices[VertexNr].Origin[1]-LightPosition.y,
                                                M.Vertices[VertexNr].Origin[2]-LightPosition.z, 0.0);

        Renderer->RenderMesh(M_back);
    }
}


void Usage()
{
    printf("\nOPTIONS:\n");
    printf("\n");
    printf("-m=MyMaterial specifies the name of the desired material, which must be\n");
    printf("   defined in one of the material script files (see below).\n");
    printf("\n");
    printf("-bd=base/dir specifies the base directory from which I look both for material\n");
    printf("   scripts and the materials associated textures.\n");
    printf("   The default (if you don't use -bd) is %s\n", BaseDirectoryName);
    printf("\n");
    printf("-ms=MyMatScript.cmat specifies the material script that contains a definition\n");
    printf("   of \"MyMaterial\" (see above). You may use -ms several times, making me look\n");
    printf("   into each specified script for a definition of the material.\n");
    printf("   If you do not use -ms at all, I'll look into ALL material scripts that I can\n");
    printf("   find in %s/Materials, so you probably don't need it as well.\n", BaseDirectoryName);
    printf("\n");
    printf("-r=RendererXY overrides the automatic selection of the \"best\" renderer,\n");
    printf("   and loads the renderer with base name RendererXY instead.\n");
    printf("   Only provide the base name (e.g. RendererOpenGL12), no path and no suffix.\n");
    printf("\n");

    exit(0);
}


int main(int ArgC, const char* ArgV[])
{
    printf("\nCafu Material Viewer (%s)\n\n", __DATE__);

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    std::string DesiredRendererName="";

    // Process the command line options.
    for (int ArgNr=1; ArgNr<ArgC; ArgNr++)
    {
             if (_strnicmp(ArgV[ArgNr], "-m=" , 3)==0) MaterialName=ArgV[ArgNr]+3;
        else if (_strnicmp(ArgV[ArgNr], "-bd=", 4)==0) BaseDirectoryName=ArgV[ArgNr]+4;
        else if (_strnicmp(ArgV[ArgNr], "-ms=", 4)==0) MaterialScriptNames.PushBack(ArgV[ArgNr]+4);
        else if (_strnicmp(ArgV[ArgNr], "-r=" , 3)==0) DesiredRendererName=ArgV[ArgNr]+3;
        else
        {
            printf("Option \"%s\" is unknown.\n", ArgV[ArgNr]);
            Usage();
        }
    }

    // Setup the global Material Manager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    // Make sure that a material has been specified.
    if (MaterialName==NULL)
    {
        printf("Please use the -m option in order to specify the desired material!\n");
        Usage();
    }

    // Register the material script files with the material manager.
    if (MaterialScriptNames.Size()==0)
    {
        // The -ms option has not been used, so register all material script files in BaseDirectoryName/Materials.
        MaterialManager->RegisterMaterialScriptsInDir(std::string(BaseDirectoryName)+"/Materials", std::string(BaseDirectoryName)+"/");
    }
    else
    {
        // Material script files have been specified - register them now.
        for (unsigned long MSNNr=0; MSNNr<MaterialScriptNames.Size(); MSNNr++)
            MaterialManager->RegisterMaterialScript(std::string(BaseDirectoryName)+"/"+MaterialScriptNames[MSNNr], std::string(BaseDirectoryName)+"/");
    }

    // Get the desired material.
    MyMaterial = MaterialManager->GetMaterial(MaterialName);

    if (MyMaterial==NULL)
    {
        printf("Sorry, material \"%s\" could not be retrieved\n", MaterialName);
        printf("from the registered material script files. Possible causes:\n");
        printf("  - the material is not defined in any of the script files (material name typo?)\n");
        printf("  - the material script file(s) could not be opened (script file name typo?)\n");
        printf("  - the material script file contains bugs, i.e. syntax errors.\n");
        return 0;
    }


    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        return -1;

    try
    {
        // The default values for the window creations hints look just right for our purposes,
        // see http://www.glfw.org/docs/latest/window_guide.html#window_hints_values for details.
        ViewerWindowT win(1280, 1024, "Cafu Material Viewer 1.1");

        // TODO: Set a taskbar icon?
        win.makeContextCurrent();
        win.triggerFramebufferSizeEvent();

        glfwSwapInterval(1);   // enable v-sync

        // Get the renderer with the highest preference number that is supported.
        HMODULE RendererDLL;
        MatSys::Renderer=(DesiredRendererName=="") ? PlatformAux::GetBestRenderer(RendererDLL) : PlatformAux::GetRenderer(DesiredRendererName, RendererDLL);

        if (MatSys::Renderer==NULL || RendererDLL==NULL)
            throw std::runtime_error("No renderer loaded.");

        MatSys::Renderer->Initialize();


        // Get the TextureMapManager from the RendererDLL.
        MatSys::TextureMapManagerI* TextureMapManager=PlatformAux::GetTextureMapManager(RendererDLL);

        if (TextureMapManager==NULL)
        {
            FreeLibrary(RendererDLL);
            throw std::runtime_error("No TextureMapManager obtained.");
        }


        // Register the material with the renderer.
        RenderMaterial = MatSys::Renderer->RegisterMaterial(MyMaterial);


        // Create a very simple lightmap for the materials that need one, and register it with the renderer.
     // char Data[]={   0,   0,   0,   0,   0,   0, 0, 0,
     //                 0,   0,   0,   0,   0,   0, 0, 0 };
        char Data[]={  50,  50,  50,  50,  50,  50, 0, 0,
                       50,  50,  50,  50,  50,  50, 0, 0 };
     // char Data[]={ 255,   0, 255,   255,   0,   0,    0, 0,
     //               255, 255,   0,     0,   0, 255,    0, 0 };

        MatSys::TextureMapI* LightMap=TextureMapManager->GetTextureMap2D(Data, 2, 2, 3, true, MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear));

        MatSys::Renderer->SetCurrentLightMap(LightMap);
        MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.


        // Create draw meshes.
        ArrayT<MatSys::MeshT> Meshes;

        // CreateMeshes needs to know the pixel dimensions of MyMaterial in order to create geometry in the proper aspect ratio.
        CreateMeshes(Meshes, *MyMaterial);


        // Also create a font (mostly for testing the FontT class).
        // It is created with new rather than on the stack so that we can delete it before the MatSys goes down.
        FontT* MyFont=new FontT("Fonts/Arial");


        // Master loop.
        TimerT Timer;
        float  ViewerOrigin[3] = { 0.0, 0.0, 0.0 };

        LightSources.PushBackEmpty(4);

        while (!win.shouldClose())
        {
            static double TotalTime=0.0;
            const  double DeltaTime=Timer.GetSecondsSinceLastCall();

            TotalTime+=DeltaTime;

            // Render the frame.
            MatSys::Renderer->BeginFrame(TotalTime);
            MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, MatrixT::GetRotateXMatrix(-90.0f));      // Rotate coordinate system axes to Cafu standard.
            MatSys::Renderer->RotateZ  (MatSys::RendererI::WORLD_TO_VIEW, ViewerHeading);
            MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -ViewerOrigin[0], -ViewerOrigin[1], -ViewerOrigin[2]);
            {
                MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
                MatSys::Renderer->SetCurrentMaterial(RenderMaterial);
                MatSys::Renderer->SetCurrentEyePosition(ViewerOrigin[0], ViewerOrigin[1], ViewerOrigin[2]);     // Also required in some ambient shaders, e.g. for the A_SkyDome.

                for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
                    MatSys::Renderer->RenderMesh(Meshes[MeshNr]);

                // Render something at the positions of the light sources.
                for (unsigned long LightNr=0; LightNr<LightSources.Size(); LightNr++)
                {
                    if (!LightSources[LightNr].IsOn) continue;

                    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

                    MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, LightSources[LightNr].Pos[0], LightSources[LightNr].Pos[1], LightSources[LightNr].Pos[2]);
                    MatSys::Renderer->Scale    (MatSys::RendererI::MODEL_TO_WORLD, 0.1f);

                    // Simply re-use the six sides of the inner central cube.
                    MatSys::Renderer->RenderMesh(Meshes[0]);
                    MatSys::Renderer->RenderMesh(Meshes[1]);
                    MatSys::Renderer->RenderMesh(Meshes[2]);
                    MatSys::Renderer->RenderMesh(Meshes[3]);
                    MatSys::Renderer->RenderMesh(Meshes[4]);
                    MatSys::Renderer->RenderMesh(Meshes[5]);

                    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
                }


                // Render the per-lightsource parts of the scene.
                for (unsigned long LightNr=0; LightNr<LightSources.Size(); LightNr++)
                {
                    if (!LightSources[LightNr].IsOn) continue;

                    MatSys::Renderer->SetCurrentLightSourcePosition(LightSources[LightNr].Pos[0], LightSources[LightNr].Pos[1], LightSources[LightNr].Pos[2]);
                    MatSys::Renderer->SetCurrentLightSourceRadius(LightSources[LightNr].Radius);
                    MatSys::Renderer->SetCurrentLightSourceDiffuseColor (LightSources[LightNr].DiffColor[0], LightSources[LightNr].DiffColor[1], LightSources[LightNr].DiffColor[2]);
                    MatSys::Renderer->SetCurrentLightSourceSpecularColor(LightSources[LightNr].SpecColor[0], LightSources[LightNr].SpecColor[1], LightSources[LightNr].SpecColor[2]);

                    if (!MyMaterial->NoShadows)
                    {
                        MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::STENCILSHADOW);
                        DrawSilhouettes(MatSys::Renderer, Meshes);
                    }

                    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::LIGHTING);

                    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
                        MatSys::Renderer->RenderMesh(Meshes[MeshNr]);
                }


                unsigned int width;
                unsigned int height;
                win.getFramebufferSize(width, height);

                MyFont->Print(width - 100, height - 18, float(width), float(height), 0x00FFFFFF, cf::va("%5.1f FPS", 1.0 / DeltaTime));
            }

            MatSys::Renderer->EndFrame();

            win.swapBuffers();
            glfwPollEvents();


            const float angle=ViewerHeading/180.0f*3.1415926f;

            if (win.isKeyPressed(GLFW_KEY_UP   ) || win.isKeyPressed(GLFW_KEY_W)) { ViewerOrigin[0]+=sin(angle)*500.0f*float(DeltaTime); ViewerOrigin[1]+=cos(angle)*500.0f*float(DeltaTime); }
            if (win.isKeyPressed(GLFW_KEY_DOWN ) || win.isKeyPressed(GLFW_KEY_S)) { ViewerOrigin[0]-=sin(angle)*500.0f*float(DeltaTime); ViewerOrigin[1]-=cos(angle)*500.0f*float(DeltaTime); }
            if (win.isKeyPressed(GLFW_KEY_LEFT ) || win.isKeyPressed(GLFW_KEY_A)) { ViewerOrigin[0]-=cos(angle)*500.0f*float(DeltaTime); ViewerOrigin[1]+=sin(angle)*500.0f*float(DeltaTime); }
            if (win.isKeyPressed(GLFW_KEY_RIGHT) || win.isKeyPressed(GLFW_KEY_D)) { ViewerOrigin[0]+=cos(angle)*500.0f*float(DeltaTime); ViewerOrigin[1]-=sin(angle)*500.0f*float(DeltaTime); }


            // Move the light sources.
            for (unsigned long LightNr=0; LightNr<LightSources.Size(); LightNr++)
            {
                if (!LightSources[LightNr].IsOn) continue;

                LightSources[LightNr].Pos[0]=sin(float(TotalTime)*0.5f/(LightNr+1))*800.0f;
                LightSources[LightNr].Pos[1]=cos(float(TotalTime)*0.5f/(LightNr+1))*800.0f;
                LightSources[LightNr].Pos[2]=-400.0f+LightNr*400.0f;
                LightSources[LightNr].Radius=5000.0;//XXXLightNr==2 ? 10000.0 : 1000.0;

                switch (LightNr % 4)
                {
                    case 0:
                        LightSources[LightNr].DiffColor[0]=1.0; LightSources[LightNr].DiffColor[1]=1.0; LightSources[LightNr].DiffColor[2]=1.0;
                        LightSources[LightNr].SpecColor[0]=1.0; LightSources[LightNr].SpecColor[1]=1.0; LightSources[LightNr].SpecColor[2]=1.0;
                        break;

                    case 1:
                        LightSources[LightNr].DiffColor[0]=1.0; LightSources[LightNr].DiffColor[1]=0.0; LightSources[LightNr].DiffColor[2]=0.0;
                        LightSources[LightNr].SpecColor[0]=1.0; LightSources[LightNr].SpecColor[1]=1.0; LightSources[LightNr].SpecColor[2]=1.0;
                        break;

                    case 2:
                        LightSources[LightNr].DiffColor[0]=0.0; LightSources[LightNr].DiffColor[1]=1.0; LightSources[LightNr].DiffColor[2]=0.0;
                        LightSources[LightNr].SpecColor[0]=1.0; LightSources[LightNr].SpecColor[1]=1.0; LightSources[LightNr].SpecColor[2]=1.0;
                        break;

                    case 3:
                        LightSources[LightNr].DiffColor[0]=0.0; LightSources[LightNr].DiffColor[1]=0.0; LightSources[LightNr].DiffColor[2]=1.0;
                        LightSources[LightNr].SpecColor[0]=1.0; LightSources[LightNr].SpecColor[1]=1.0; LightSources[LightNr].SpecColor[2]=1.0;
                        break;
                }
            }
        }


        // Clean-up.
        delete MyFont;
        MyFont=NULL;
        MatSys::Renderer->FreeMaterial(RenderMaterial);
        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
        FreeLibrary(RendererDLL);
    }
    catch (const std::runtime_error& re)
    {
        fprintf(stderr, "ERROR: %s\n", re.what());
        glfwTerminate();
        return -1;
    }

    glfwTerminate();
    return 0;
}
