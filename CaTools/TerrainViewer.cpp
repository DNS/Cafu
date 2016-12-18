/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/
#include <stdio.h>
#include <float.h>
#include <string.h>

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "MainWindow/glfwWindow.hpp"
#include "MaterialSystem/MapComposition.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "Math3D/Plane3.hpp"
#include "Math3D/Matrix.hpp"
#include "Templates/Array.hpp"
#include "Terrain/Terrain.hpp"
#include "Util/Util.hpp"
#include "PlatformAux.hpp"

#include "zlib.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#define FreeLibrary dlclose
#endif

#include "GLFW/glfw3.h"
#include "tclap/CmdLine.h"
#include "tclap/StdOutput.h"


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

MaterialManagerI* MaterialManager=NULL;


#define DEG2RAD(x) ((3.1415927f / 180.0f) * (x))
#define SQR(x)     ((x) * (x))


class ViewerWindowT : public cf::glfwWindowT
{
    public:

    ViewerWindowT(int width, int height, const char* title, GLFWmonitor* monitor=0)
        : glfwWindowT(width, height, title, monitor)
    {
        VI.cull  = true;    // perform view culling when set
        VI_morph = true;    // perform geomorphing when set
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

        switch (key)
        {
            case GLFW_KEY_ESCAPE:
                setShouldClose(true);
                break;

            case GLFW_KEY_C:
                VI.cull = !VI.cull;
                printf("View frustum culling is %s.\n", VI.cull ? "ON" : "OFF");
                break;

            case GLFW_KEY_M:
                VI_morph = !VI_morph;
                printf("Geo-morphing is %s\n", VI_morph ? "ON" : "OFF");
                break;
        }
    }


    public:

    TerrainT::ViewInfoT VI;
    bool VI_morph;
};


static void error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error: %s\n", description);
}


int main(int ArgC, char* ArgV[])
{
    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    // Process the command line options.
    // The "wireframe" material is defined in file "meta.cmat",
    // SOARX is the new SOAR implementation.
    TCLAP::StdOutput stdOutput(std::cout, std::cerr);
    TCLAP::CmdLine cmd("Cafu Engine Terrain Viewer", stdOutput, ' ', "1.2");

    // These may throw e.g. SpecificationException, but such exceptions are easily fixed permanently.
    const TCLAP::ValueArg<std::string> TerrainName("t", "terrain", "Name of the terrain (heightmap image) to view.", true, "", "string", cmd);
    const TCLAP::ValueArg<std::string> MaterialName("m", "material", "Name of the material to render the terrain with.", false, "wireframe", "string", cmd);
    const TCLAP::ValueArg<std::string> BaseDirName("d", "base-dir", "Name of the base directory to search for material scripts.", false, "Games/DeathMatch", "string", cmd);
    const TCLAP::SwitchArg BenchMarkMode("b", "benchmark", "Run program in benchmark mode.", cmd, false);
    const TCLAP::SwitchArg UseSOARX("s", "soarx", "Use the SOARX implementation of the SOAR algorithm.", cmd, false);

    TCLAP::VersionVisitor vv(&cmd, stdOutput);
    const TCLAP::SwitchArg argVersion("",  "version", "Displays version information and exits.", cmd, false, &vv);

    TCLAP::HelpVisitor hv(&cmd, stdOutput);
    const TCLAP::SwitchArg argHelp("h", "help", "Displays usage information and exits.", cmd, false, &hv);

    try
    {
        cmd.parse(ArgC, ArgV);
    }
    catch (const TCLAP::ExitException& ee)
    {
        //  ExitException is thrown after --help or --version was handled.
        return ee.getExitStatus();
    }
    catch (const TCLAP::ArgException& e)
    {
        cmd.getOutput().failure(cmd, e, true);
        return -1;
    }

    // Setup the global Material Manager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    // Register the material script files with the material manager.
    MaterialManager->RegisterMaterialScriptsInDir(BaseDirName.getValue() + "/Materials", BaseDirName.getValue() + "/");

    // Get the desired material.
    MaterialT* TerrainMaterial=MaterialManager->GetMaterial(MaterialName.getValue());

    if (TerrainMaterial==NULL)
    {
        printf("Sorry, material \"%s\" could not be retrieved\n", MaterialName.getValue().c_str());
        printf("from the registered material script files. Possible causes:\n");
        printf("  - the material is not defined in any of the script files (material name typo?)\n");
        printf("  - the material script file(s) could not be opened (script file name typo?)\n");
        printf("  - the material script file contains bugs, i.e. syntax errors.\n");

        return 0;
    }


    try
    {
        const unsigned long FRAMES_FOR_BENCHMARK=1000;
        const Vector3fT     TerrainResolution(160.0f, 160.0f, 50.0f*255.0f);
        TerrainT            TerrainNew(TerrainName.getValue().c_str(), TerrainResolution);
        const double        STEPDIST_FOR_BENCHMARK=TerrainNew.GetSize()*1.2*TerrainResolution.x/FRAMES_FOR_BENCHMARK;


        glfwSetErrorCallback(error_callback);

        if (!glfwInit())
            return -1;

        // The default values for the window creations hints look just right for our purposes,
        // see http://www.glfw.org/docs/latest/window_guide.html#window_hints_values for details.
        ViewerWindowT win(1280, 1024, "Cafu Terrain Viewer 1.3", BenchMarkMode.getValue() ? glfwGetPrimaryMonitor() : NULL);

        // TODO: Set a taskbar icon?
        win.makeContextCurrent();
        win.triggerFramebufferSizeEvent();

        glfwSwapInterval(1);   // enable v-sync

        // Get the renderer with the highest preference number that is supported.
        HMODULE RendererDLL;
        MatSys::Renderer=PlatformAux::GetBestRenderer(RendererDLL);

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



        TerrainT::ViewInfoT& VI = win.VI;

        // Master Game Loop
        TimerT        Timer;
        unsigned long FrameCounter=0;
        unsigned long GeomCRC=adler32(0, NULL, 0);  // We use Adler-32 instead of CRC-32, as Adler is faster but just as reliable.
        Vector3fT     ViewerPos=BenchMarkMode.getValue() ? Vector3fT(TerrainNew.GetVertices()[0])+Vector3fT(0, 0, 4000.0f) : Vector3fT(0, -500.0f, 1000.0f);
        float         Heading=BenchMarkMode.getValue() ? 55.0f : 0.0f;
        float         Pitch  =BenchMarkMode.getValue() ? 25.0f : 0.0f;

        while (!win.shouldClose())
        {
            MatSys::Renderer->BeginFrame(Timer.GetSecondsSinceCtor());

            MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, MatrixT::GetRotateXMatrix(-90.0f));
            MatSys::Renderer->RotateX  (MatSys::RendererI::WORLD_TO_VIEW, Pitch  );
            MatSys::Renderer->RotateZ  (MatSys::RendererI::WORLD_TO_VIEW, Heading);
            MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -ViewerPos.x, -ViewerPos.y, -ViewerPos.z);


            const float tau    =4.0;    // Error tolerance in pixel.
         // const float tau_min=tau;
         // const float tau_max=VI_morph ? (3.0/2.0)*tau_min : tau_min;
            // The basic formula for fov_x is from soar/main.c, reshape_callback function, rearranged for fov_x.
            // TODO: The 67.5 is a fixed value from framebuffer_size_callback() above.
            unsigned int width;
            unsigned int height;

            win.getFramebufferSize(width, height);      // This is the window size in pixels.

            // printf("%u x %u    %u x %u\n", width, height, w_, h_);
            const float fov_x = 2.0f*atan(float(width)/float(height) * tan(DEG2RAD(67.5f)/2.0f));
            const float kappa = tau / width * fov_x;

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
                const char  m = 0;
                const float d = (i < 4) ? 1.0f - 0.75f*m/8.0f : 1.0f;
                float       plane[4];

                for (unsigned long j=0; j<4; j++)
                    plane[j]=((i & 1) ? mpv.m[i/2][j] : -mpv.m[i/2][j]) - d*mpv.m[3][j];

                const float l=sqrt(SQR(plane[0])+SQR(plane[1])+SQR(plane[2]));

                VI.viewplanes[i]=Plane3fT(Vector3fT(plane[0]/l, plane[1]/l, plane[2]/l), -plane[3]/l);
            }


            static MatSys::MeshT TerrainMesh(MatSys::MeshT::TriangleStrip);
            TerrainMesh.Vertices.Overwrite();

            if (UseSOARX.getValue())
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
                if (win.VI_morph)
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


            double DeltaTime=Timer.GetSecondsSinceLastCall();

            float   MoveSpeed=1000.0f*float(DeltaTime);
            float   RotSpeed =  90.0f*float(DeltaTime);


            MatSys::Renderer->EndFrame();

            win.swapBuffers();
            glfwPollEvents();

            FrameCounter++;

            if (BenchMarkMode.getValue() && FrameCounter==FRAMES_FOR_BENCHMARK)
                break;

            if (BenchMarkMode.getValue())
            {
                const float vx = float(STEPDIST_FOR_BENCHMARK)*sin(Heading/180.0f*3.1415926f);
                const float vy = float(STEPDIST_FOR_BENCHMARK)*cos(Heading/180.0f*3.1415926f);

                ViewerPos=ViewerPos+Vector3fT(vx, vy, 0);
            }
            else
            {
                const float vx = MoveSpeed*sin(Heading/180.0f*3.1415926f);
                const float vy = MoveSpeed*cos(Heading/180.0f*3.1415926f);

                if (win.isKeyPressed(GLFW_KEY_UP       ) || win.isKeyPressed(GLFW_KEY_W)) ViewerPos=ViewerPos+Vector3fT( vx,  vy, 0);
                if (win.isKeyPressed(GLFW_KEY_DOWN     ) || win.isKeyPressed(GLFW_KEY_S)) ViewerPos=ViewerPos+Vector3fT(-vx, -vy, 0);
                if (                                        win.isKeyPressed(GLFW_KEY_A)) ViewerPos=ViewerPos+Vector3fT(-vy,  vx, 0);
                if (                                        win.isKeyPressed(GLFW_KEY_D)) ViewerPos=ViewerPos+Vector3fT( vy, -vx, 0);
                if (win.isKeyPressed(GLFW_KEY_INSERT   ) || win.isKeyPressed(GLFW_KEY_R)) ViewerPos.z+=MoveSpeed;
                if (win.isKeyPressed(GLFW_KEY_DELETE   ) || win.isKeyPressed(GLFW_KEY_F)) ViewerPos.z-=MoveSpeed;
                if (win.isKeyPressed(GLFW_KEY_LEFT     )                                ) Heading-=RotSpeed;
                if (win.isKeyPressed(GLFW_KEY_RIGHT    )                                ) Heading+=RotSpeed;
                if (win.isKeyPressed(GLFW_KEY_PAGE_UP  )                                ) Pitch-=RotSpeed;
                if (win.isKeyPressed(GLFW_KEY_PAGE_DOWN)                                ) Pitch+=RotSpeed;
                if (win.isKeyPressed(GLFW_KEY_END      )                                ) Pitch=0.0;
            }
        }

        const double TotalTime=Timer.GetSecondsSinceCtor();
        printf("Average frame-rate was: %.2f FPS   (%lu frames in %.2f seconds)\n", double(FrameCounter)/TotalTime, FrameCounter, TotalTime);
        printf("Geo-morphing    was: %s\n", win.VI_morph ? " ON" : "OFF");
        printf("Frustum culling was: %s\n", VI.cull  ? " ON" : "OFF");
        printf("Geometry CRC    was: 0x%lX\n", GeomCRC);

        // Clean-up.
        MatSys::Renderer->FreeMaterial(TerrainRenderMat);
        MatSys::Renderer->Release();
        MatSys::Renderer=NULL;
        FreeLibrary(RendererDLL);
    }
    catch (const TerrainT::InitError& /*E*/)
    {
        printf("\nEither \"%s\" could not be found, not be read,\n", TerrainName.getValue().c_str());
        printf("is not square, is smaller than 3 pixels, or not of size 2^n+1.  Sorry.\n");
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
