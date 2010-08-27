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

/***************************/
/*** Cafu Terrain Viewer ***/
/***************************/

#include <stdio.h>
#include <float.h>
#include <string.h>

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "MaterialSystem/MapComposition.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "Math3D/Plane3.hpp"
#include "Math3D/Matrix.hpp"
#include "OpenGL/OpenGLWindow.hpp"
#include "Templates/Array.hpp"
#include "Terrain/Terrain.hpp"
#include "Util/Util.hpp"
#include "PlatformAux.hpp"

#include "zlib.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
    #if defined(_MSC_VER)
        #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
    #endif
#else
#include <dlfcn.h>
#define FreeLibrary dlclose
#endif


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

MaterialManagerI* MaterialManager=NULL;


#define DEG2RAD(x) ((3.1415927f / 180.0f) * (x))
#define SQR(x)     ((x) * (x))


std::string BaseDirectoryName="Games/DeathMatch";


void Usage()
{
    printf("\n");
    printf("\nUSAGE: TerrainViewer -t=HeightMapName -m=MaterialName [OPTIONS]\n");
    printf("\n");
    printf("\nMANDATORY PARAMETERS:\n");
    printf("\n");
    printf("-t=MyHeightMap specifies the name of the desired terrain (heightmap image).\n");
    printf("\n");
    printf("-m=MyMaterial specifies the name of the desired material, which must be\n");
    printf("   defined in one of the material script files (see below).\n");
    printf("\n");
    printf("\nOPTIONS:\n");
    printf("\n");
    printf("-bd=base/dir specifies the base directory from which I look both for material\n");
    printf("   scripts and the materials associated textures.\n");
    printf("   The default (if you don't use -bd) is %s\n", BaseDirectoryName.c_str());
    printf("\n");
    printf("-ms=MyMatScript.cmat specifies the material script that contains a definition\n");
    printf("   of \"MyMaterial\" (see above). You may use -ms several times, making me look\n");
    printf("   into each specified script for a definition of the material.\n");
    printf("   If you do not use -ms at all, I'll look into ALL material scripts that I can\n");
    printf("   find in %s/Materials, so you probably don't need it as well.\n", BaseDirectoryName.c_str());
    printf("\n");
    printf("-bm runs a simple benchmark. Mostly intended for development purposes.\n");
    printf("\n");

    exit(0);
}


int main(int ArgC, char* ArgV[])
{
    const char*         TerrainName  =NULL;
    const char*         MaterialName ="wireframe";  // The "wireframe" material is defined in file "meta.cmat".
    bool                BenchMarkMode=false;
    bool                UseSOARX     =false;        // SOARX is the new SOAR implementation.
    ArrayT<const char*> MaterialScriptNames;


    printf("\nCafu Engine Terrain Viewer (%s)\n\n", __DATE__);

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    // Process the command line options.
    for (int ArgNr=1; ArgNr<ArgC; ArgNr++)
    {
             if (_strnicmp(ArgV[ArgNr], "-t=" , 3)==0) TerrainName=ArgV[ArgNr]+3;
        else if (_strnicmp(ArgV[ArgNr], "-m=" , 3)==0) MaterialName=ArgV[ArgNr]+3;
        else if (_strnicmp(ArgV[ArgNr], "-bd=", 4)==0) BaseDirectoryName=ArgV[ArgNr]+4;
        else if (_strnicmp(ArgV[ArgNr], "-ms=", 4)==0) MaterialScriptNames.PushBack(ArgV[ArgNr]+4);
        else if (_stricmp (ArgV[ArgNr], "-bm"    )==0) BenchMarkMode=true;
        else if (_stricmp (ArgV[ArgNr], "-sx"    )==0) UseSOARX=true;
        else
        {
            printf("Sorry, I don't know what option \"%s\" means.\n", ArgV[ArgNr]);
            Usage();
        }
    }

    if (TerrainName==NULL)
    {
        printf("Hmm. Specifying a terrain name wouldn't hurt...\n");
        printf("Please use the -t option in order to specify the desired terrain!\n");
        Usage();
    }

    // Setup the global Material Manager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    // Register the material script files with the material manager.
    if (MaterialScriptNames.Size()==0)
    {
        // The -ms option has not been used, so register all material script files in BaseDirectoryName/Materials.
        MaterialManager->RegisterMaterialScriptsInDir(BaseDirectoryName+"/Materials", BaseDirectoryName+"/");
    }
    else
    {
        // Material script files have been specified - register them now.
        for (unsigned long MSNNr=0; MSNNr<MaterialScriptNames.Size(); MSNNr++)
            MaterialManager->RegisterMaterialScript(BaseDirectoryName+"/"+MaterialScriptNames[MSNNr], BaseDirectoryName+"/");
    }

    // Get the desired material.
    MaterialT* TerrainMaterial=MaterialManager->GetMaterial(MaterialName);

    if (TerrainMaterial==NULL)
    {
        printf("Sorry, I have not been able to get material \"%s\"\n", MaterialName);
        printf("from the registered material script files. Possible causes:\n");
        printf("- the material is not defined in any of the script files (material name typo?)\n");
        printf("- the material script file(s) could not be opened (script file name typo?)\n");
        printf("- the material script file contains bugs, i.e. syntax errors.\n");

        return 0;
    }


    try
    {
        const unsigned long FRAMES_FOR_BENCHMARK=1000;
        const Vector3fT     TerrainResolution(160.0f, 160.0f, 50.0f*255.0f);
        TerrainT            TerrainNew(TerrainName, TerrainResolution);
        const double        STEPDIST_FOR_BENCHMARK=TerrainNew.GetSize()*1.2*TerrainResolution.x/FRAMES_FOR_BENCHMARK;


        // Open OpenGL-Window.
        const char* ErrorMsg=SingleOpenGLWindow->Open("Cafu Terrain Viewer 1.2", BenchMarkMode ? 1280 : 800, BenchMarkMode ? 1024 : 600, 32, BenchMarkMode);

        if (ErrorMsg)
        {
            printf("\nUnable to open OpenGL window: %s\n", ErrorMsg);
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


        MatSys::RenderMaterialT* TerrainRenderMat=MatSys::Renderer->RegisterMaterial(TerrainMaterial);

        MatSys::Renderer->SetCurrentMaterial(TerrainRenderMat);


        // As the terrain shaders require (cmat materials to specify) a lightmap, provide one here.
        char Data[]={ 255, 255, 255, 255, 255, 255, 0, 0,
                      255, 255, 255, 255, 255, 255, 0, 0 };

        MatSys::Renderer->SetCurrentLightMap(TextureMapManager->GetTextureMap2D(Data, 2, 2, 3, true, MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear)));
        MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.


        MatSys::Renderer->ClearColor(0.0f, 0.0f, 0.4f, 0.0f);



        /* This is how it used to be.
           We now try it via the vertex indices directly... */

        const float res0=160.0;
        const float res1=160.0;

        // BBmin and BBmax are BBs for the xy-plane, z is not needed here.
        // Also note that the y-components have been multiplied by -1, or otherwise the texture gets flipped.
        const float BBmin[2]={ -res0*0.5f*(TerrainNew.GetSize()-1),  res1*0.5f*(TerrainNew.GetSize()-1) };
        const float BBmax[2]={  res0*0.5f*(TerrainNew.GetSize()-1), -res1*0.5f*(TerrainNew.GetSize()-1) };

        const float Plane1[4]={ 1.0f/(BBmax[0]-BBmin[0]), 0.0f, 0.0f, -BBmin[0]/(BBmax[0]-BBmin[0]) };
        const float Plane2[4]={ 0.0f, 1.0f/(BBmax[1]-BBmin[1]), 0.0f, -BBmin[1]/(BBmax[1]-BBmin[1]) };

        MatSys::Renderer->SetGenPurposeRenderingParam( 4, Plane1[0]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 5, Plane1[1]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 6, Plane1[2]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 7, Plane1[3]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 8, Plane2[0]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 9, Plane2[1]);
        MatSys::Renderer->SetGenPurposeRenderingParam(10, Plane2[2]);
        MatSys::Renderer->SetGenPurposeRenderingParam(11, Plane2[3]);



        TerrainT::ViewInfoT VI;

        VI.cull=true;               // perform view culling when set
        bool VI_morph=true;         // perform geomorphing when set

        // Master Game Loop
        TimerT        Timer;
        unsigned long FrameCounter=0;
        unsigned long GeomCRC=adler32(0, NULL, 0);  // We use Adler-32 instead of CRC-32, as Adler is faster but just as reliable.
        Vector3fT     ViewerPos=BenchMarkMode ? Vector3fT(TerrainNew.GetVertices()[0])+Vector3fT(0, 0, 4000.0f) : Vector3fT(0, -500.0f, 1000.0f);
        float         Heading=BenchMarkMode ? 55.0f : 0.0f;
        float         Pitch  =BenchMarkMode ? 25.0f : 0.0f;

        while (true)
        {
            // Rufe die Nachrichten der Windows-Nachrichtenschlange ab.
            if (SingleOpenGLWindow->HandleWindowMessages()) break;

            MatSys::Renderer->BeginFrame(Timer.GetSecondsSinceCtor());

         // MatSys::Renderer->GetModifyMatrix(MatSys::RendererI::MODEL_TO_WORLD)=MatrixT();

            MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, MatrixT::GetRotateXMatrix(-90.0f));
            MatSys::Renderer->RotateX  (MatSys::RendererI::WORLD_TO_VIEW, Pitch  );
            MatSys::Renderer->RotateZ  (MatSys::RendererI::WORLD_TO_VIEW, Heading);
            MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -ViewerPos.x, -ViewerPos.y, -ViewerPos.z);

            // if (TextureName==NULL) glColor3f(0.6, 1.0, 0.5);
            // if (TextureName==NULL) glDisable(GL_POLYGON_OFFSET_FILL);
            // if (TextureName==NULL) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


            const float tau    =4.0;    // Error tolerance in pixel.
         // const float tau_min=tau;
         // const float tau_max=VI_morph ? (3.0/2.0)*tau_min : tau_min;
            // The basic formula for fov_x is from soar/main.c, reshape_callback function, rearranged for fov_x.
            // The 67.5 is a fixed value from OpenGLWindow.cpp.
            const float fov_x  =2.0f*atan(float(SingleOpenGLWindow->GetWidth())/float(SingleOpenGLWindow->GetHeight())*tan(DEG2RAD(67.5f)/2.0f));
            const float kappa  =tau/SingleOpenGLWindow->GetWidth() * fov_x;

            VI.nu    =kappa>0.0 ? 1.0f/kappa : FLT_MAX; // inverse of error tolerance in radians
            VI.nu_min=2.0f/3.0f*VI.nu;                  // lower morph parameter
            VI.nu_max=          VI.nu;                  // upper morph parameter

            VI.viewpoint=ViewerPos;

            // Set up VI.viewplanes (clip planes) for view frustum culling.
            MatrixT mpv=MatSys::Renderer->GetMatrix(MatSys::Renderer->PROJECTION)*MatSys::Renderer->GetMatrixModelView();

            // Compute view frustum planes.
            for (unsigned long i=0; i<5; i++)
            {
                // m can be used to easily minify / shrink the view frustum.
                // The values should be between 0 and 8: 0 is the default (no minification), 8 is the reasonable maximum.
                const char  m=0;
                const float d=(i<4 && m>0) ? 1.0f-0.75f*m/8.0f : 1.0f;
                float       plane[4];

                for (unsigned long j=0; j<4; j++)
                    plane[j]=((i & 1) ? mpv.m[i/2][j] : -mpv.m[i/2][j]) - d*mpv.m[3][j];

                const float l=sqrt(SQR(plane[0])+SQR(plane[1])+SQR(plane[2]));

                VI.viewplanes[i]=Plane3fT(Vector3fT(plane[0]/l, plane[1]/l, plane[2]/l), -plane[3]/l);
            }


            static MatSys::MeshT TerrainMesh(MatSys::MeshT::TriangleStrip);
            TerrainMesh.Vertices.Overwrite();

            if (UseSOARX)
            {
                ArrayT<Vector3fT>& VectorStrip=TerrainNew.ComputeVectorStrip(VI);

                // With SOARX, the vector strip begins as usual and as expected with the first vector.
                for (unsigned long VNr=0; VNr<VectorStrip.Size(); VNr++)
                {
                    TerrainMesh.Vertices.PushBackEmpty();
                    TerrainMesh.Vertices[VNr].SetOrigin(VectorStrip[VNr]);

                    // Update the geometry-CRC. We use Adler-32 instead of CRC-32, as Adler is faster but just as reliable.
                    GeomCRC=adler32(GeomCRC, (Bytef*)&VectorStrip[VNr].z, sizeof(VectorStrip[VNr].z));
                }
            }
            else
            {
                if (VI_morph)
                {
                    ArrayT<Vector3fT>& VectorStripNew=TerrainNew.ComputeVectorStripByMorphing(VI);

                    // Note that the first VectorT at VectorStrip[0] must be skipped!
                    for (unsigned long VNr=1; VNr<VectorStripNew.Size(); VNr++)
                    {
                        TerrainMesh.Vertices.PushBackEmpty();
                        TerrainMesh.Vertices[TerrainMesh.Vertices.Size()-1].SetOrigin(VectorStripNew[VNr]);

                        // Update the geometry-CRC. We use Adler-32 instead of CRC-32, as Adler is faster but just as reliable.
                        GeomCRC=adler32(GeomCRC, (Bytef*)&VectorStripNew[VNr].z, sizeof(VectorStripNew[VNr].z));
                    }
                }
                else
                {
                    ArrayT<unsigned long>& IdxStripNew=TerrainNew.ComputeIndexStripByRefinement(VI);

                    // Note that the first index at IdxStrip[0] must be skipped!
                    const TerrainT::VertexT* Vertices=TerrainNew.GetVertices();

                    for (unsigned long IdxNr=1; IdxNr<IdxStripNew.Size(); IdxNr++)
                    {
                        TerrainMesh.Vertices.PushBackEmpty();
                        TerrainMesh.Vertices[TerrainMesh.Vertices.Size()-1].SetOrigin(Vertices[IdxStripNew[IdxNr]]);

                        // Update the geometry-CRC. We use Adler-32 instead of CRC-32, as Adler is faster but just as reliable.
                        GeomCRC=adler32(GeomCRC, (Bytef*)&IdxStripNew[IdxNr], sizeof(IdxStripNew[IdxNr]));
                    }
                }
            }

            MatSys::Renderer->RenderMesh(TerrainMesh);

            // if (TextureName==NULL) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            // if (TextureName==NULL) glEnable(GL_POLYGON_OFFSET_FILL);


            double DeltaTime=Timer.GetSecondsSinceLastCall();

            float   MoveSpeed=1000.0f*float(DeltaTime);
            float   RotSpeed =  90.0f*float(DeltaTime);


            MatSys::Renderer->EndFrame();
            SingleOpenGLWindow->SwapBuffers();

            FrameCounter++;

            CaKeyboardEventT KE;
            bool QuitProgram=false;

            while (SingleOpenGLWindow->GetNextKeyboardEvent(KE)>0)
            {
                if (KE.Type!=CaKeyboardEventT::CKE_KEYDOWN) continue;
                if (KE.Key==CaKeyboardEventT::CK_ESCAPE) QuitProgram=true;
                if (KE.Key==CaKeyboardEventT::CK_C     ) { VI.cull =!VI.cull;  printf("View frustum culling is %s.\n", VI.cull ? "ON" : "OFF"); }
                if (KE.Key==CaKeyboardEventT::CK_M     ) { VI_morph=!VI_morph; printf("Geo-morphing is %s\n", VI_morph ? "ON" : "OFF"); }
            }

            if (QuitProgram) break;
            if (BenchMarkMode && FrameCounter==FRAMES_FOR_BENCHMARK) break;


            if (BenchMarkMode)
            {
                const float vx=float(STEPDIST_FOR_BENCHMARK)*sin(Heading/180.0f*3.1415926f);
                const float vy=float(STEPDIST_FOR_BENCHMARK)*cos(Heading/180.0f*3.1415926f);

                ViewerPos=ViewerPos+Vector3fT(vx, vy, 0);
            }
            else
            {
                const float vx=MoveSpeed*sin(Heading/180.0f*3.1415926f);
                const float vy=MoveSpeed*cos(Heading/180.0f*3.1415926f);

                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_UP    ] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_W]) ViewerPos=ViewerPos+Vector3fT( vx,  vy, 0);
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_DOWN  ] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_S]) ViewerPos=ViewerPos+Vector3fT(-vx, -vy, 0);
                if (                                                                       SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_A]) ViewerPos=ViewerPos+Vector3fT(-vy,  vx, 0);
                if (                                                                       SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_D]) ViewerPos=ViewerPos+Vector3fT( vy, -vx, 0);
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_INSERT] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_R]) ViewerPos.z+=MoveSpeed;
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_DELETE] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_F]) ViewerPos.z-=MoveSpeed;
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_LEFT  ]                                                                  ) Heading-=RotSpeed;
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RIGHT ]                                                                  ) Heading+=RotSpeed;
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGUP  ]                                                                  ) Pitch-=RotSpeed;
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGDN  ]                                                                  ) Pitch+=RotSpeed;
                if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_END   ]                                                                  ) Pitch=0.0;
            }
        }

        const double TotalTime=Timer.GetSecondsSinceCtor();
        printf("Average frame-rate was: %.2f FPS   (%lu frames in %.2f seconds)\n", double(FrameCounter)/TotalTime, FrameCounter, TotalTime);
        printf("Geo-morphing    was: %s\n", VI_morph ? " ON" : "OFF");
        printf("Frustum culling was: %s\n", VI.cull  ? " ON" : "OFF");
        printf("Geometry CRC    was: 0x%lX\n", GeomCRC);

        // Clean-up.
        MatSys::Renderer->FreeMaterial(TerrainRenderMat);
        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
        FreeLibrary(RendererDLL);
        SingleOpenGLWindow->Close();
    }
    catch (const TerrainT::InitError& /*E*/)
    {
        printf("\nEither \"%s\" could not be found, not be read,\n", TerrainName);
        printf("is not square, is smaller than 3 pixels, or not of size 2^n+1.  Sorry.\n");
    }

    return 0;
}
