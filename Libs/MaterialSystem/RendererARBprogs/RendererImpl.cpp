/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*******************************/
/*** Renderer Implementation ***/
/*******************************/

// Required for #include <GL/gl.h> with MS VC++.
#if defined(_WIN32) && defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "RendererImpl.hpp"
#include "../Common/OpenGLState.hpp"
#include "RenderMaterial.hpp"
#include "Shader.hpp"
#include "TextureMapImpl.hpp"

#include "../Common/OpenGLEx.hpp"
#include "ConsoleCommands/Console.hpp"
#include "Templates/Array.hpp"


using namespace MatSys;


RendererImplT& RendererImplT::GetInstance()
{
    static RendererImplT Renderer;

    return Renderer;
}


RendererImplT::RendererImplT()
    : IsInitialized        (false),
      CurrentRenderAction  (AMBIENT),
      CurrentRenderMaterial(NULL),
      LockCurrentRM        (false),
      CurrentShader        (NULL),
      CurrentLightMap      (NULL),
      CurrentLightDirMap   (NULL),
   // CurrentSHLMaps       (),
      CurrentSHLLookupMap  (NULL),
      NormalizationCubeMap (NULL),
      HelperGrayMap        (NULL),
      HelperBlackMap       (NULL),
      HelperLightBlue001Map(NULL),
      InitCounter          (1),
      ShaderChangeCounter  (0),
      MatrixModelView      (Matrix[WORLD_TO_VIEW], Matrix[MODEL_TO_WORLD])
{
    for (unsigned long SHLMapNr=0; SHLMapNr<16; SHLMapNr++)
        CurrentSHLMaps[SHLMapNr]=NULL;

    // All matrices are initialized with the identity matrix by their constructor.
    // Sync'ing with the OpenGL matrices is done in Initialize().
    for (unsigned long MN=0; MN<END_MARKER; MN++)
    {
        Matrix[MN].Age=1;       // Force an update of all dependent matrices when they are next being accessed.
        MatrixInv[MN].SetSourceMatrix(&Matrix[MN]);
    }

    ExpressionSymbols.GenFloat.PushBackEmpty(32);
    ExpressionSymbols.GenInt  .PushBackEmpty(32);
}


RendererImplT::~RendererImplT()
{
    // Is it proper to call Release() here?
    // It will be called even if Initialize() has not been called, e.g. because the user only wanted
    // to call IsSupported(). And it's the users responsibility to call Release() anyway...  FIXME! TODO!
    // Then the IsInitialized member could also go again!
    Release();
}


bool RendererImplT::IsSupported() const
{
    // First check if we have a valid rendering context.
    // This ASSUMES that IF we have a valid rendering context, it has been left with the error flag cleared.
    // glGetError();   // Clear the error flag manually (will set error GL_INVALID_OPERATION on invalid RC).
    GLenum LastError=glGetError();
    if (LastError!=GL_NO_ERROR)
    {
#ifdef DEBUG
        Console->Print(cf::va("\n%s (%u): glGetError() returned error %lu (0x%X).\n", __FILE__, __LINE__, (unsigned long)LastError, LastError));
#endif
        return false;
    }

    // Technically, we require OpenGL 1.2 (e.g. for GL_CLAMP_TO_EDGE), and additionally some extensions must be present.
    const char* Version=(char const*)glGetString(GL_VERSION);
    if (Version==NULL) return false;                    // This is another way to see if the RC is valid.
    if (atof(Version)<1.2) return false;                // Require 1.2 or higher.


    cf::Init_GL_ARB_multitexture();
    if (!cf::GL_ARB_multitexture_AVAIL) return false;       // Require the GL_ARB_multitexture extension.

    cf::Init_GL_ARB_texture_cube_map();
    if (!cf::GL_ARB_texture_cube_map_AVAIL) return false;   // Require the GL_ARB_texture_cube_map extension.

    // Non-availability of the GL_EXT_stencil_wrap extension will disable stencil shadows,
    // but do no harm otherwise, so we don't strictly require it.
    // The GL_EXT_stencil_two_side extension is optional anyway.

    cf::Init_GL_ARB_vertex_and_fragment_program();
    if (!cf::GL_ARB_vertex_and_fragment_program_AVAIL) return false;


    // Some of our shaders currently bind up to 6 textures simultaneously,
    // and even more texture-coordinate slots are used for submitting parameters.
    // So make sure that the underlying implementation supports this.
    GLint MaxTexImgUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &MaxTexImgUnits);
    if (MaxTexImgUnits<8) return false;

    GLint MaxTexCoords;
    glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &MaxTexCoords);
    if (MaxTexCoords<8) return false;

    return true;
}


bool RendererImplT::DoesSupportCompressedSHL() const
{
    return false;
}


bool RendererImplT::DoesSupportUncompressedSHL() const
{
    return false;
}


int RendererImplT::GetPreferenceNr() const
{
    return 2200;
}


static char PackFloatInByte(float in)
{
    return char((in+1.0f)/2.0f*255.0f);
}


static TextureMapCubeT* InitNormalizationCubeMap()
{
    const unsigned long CUBEMAP_TEX_SIZE=32;
    const GLfloat       halfMapSize     =CUBEMAP_TEX_SIZE/2;

    char cubemapPX[3*CUBEMAP_TEX_SIZE*CUBEMAP_TEX_SIZE];
    char cubemapPY[3*CUBEMAP_TEX_SIZE*CUBEMAP_TEX_SIZE];
    char cubemapPZ[3*CUBEMAP_TEX_SIZE*CUBEMAP_TEX_SIZE];
    char cubemapNX[3*CUBEMAP_TEX_SIZE*CUBEMAP_TEX_SIZE];
    char cubemapNY[3*CUBEMAP_TEX_SIZE*CUBEMAP_TEX_SIZE];
    char cubemapNZ[3*CUBEMAP_TEX_SIZE*CUBEMAP_TEX_SIZE];

    // Positive X face
    for (unsigned long y=0; y<CUBEMAP_TEX_SIZE; y++)
        for (unsigned long z=0; z<CUBEMAP_TEX_SIZE; z++)
        {
            GLfloat fX=halfMapSize;
            GLfloat fY=halfMapSize-y;
            GLfloat fZ=halfMapSize-z;
            GLfloat oolen=1.0f/sqrt(fX*fX + fY*fY + fZ*fZ);
            fX *= oolen;  fY *= oolen;  fZ *= oolen;
            cubemapPX[y*3*CUBEMAP_TEX_SIZE + z*3 + 0]=PackFloatInByte(fX);
            cubemapPX[y*3*CUBEMAP_TEX_SIZE + z*3 + 1]=PackFloatInByte(fY);
            cubemapPX[y*3*CUBEMAP_TEX_SIZE + z*3 + 2]=PackFloatInByte(fZ);
        }

    // Negative X face
    for (unsigned long y=0; y<CUBEMAP_TEX_SIZE; y++)
        for (unsigned long z=0; z<CUBEMAP_TEX_SIZE; z++)
        {
            GLfloat fX= -halfMapSize;
            GLfloat fY=  halfMapSize-y;
            GLfloat fZ=z-halfMapSize;
            GLfloat oolen=1.0f/sqrt(fX*fX + fY*fY + fZ*fZ);
            fX *= oolen;  fY *= oolen;  fZ *= oolen;
            cubemapNX[y*3*CUBEMAP_TEX_SIZE + z*3 + 0]=PackFloatInByte(fX);
            cubemapNX[y*3*CUBEMAP_TEX_SIZE + z*3 + 1]=PackFloatInByte(fY);
            cubemapNX[y*3*CUBEMAP_TEX_SIZE + z*3 + 2]=PackFloatInByte(fZ);
        }

    // Positive Y face
    for (unsigned long z=0; z<CUBEMAP_TEX_SIZE; z++)
        for (unsigned long x=0; x<CUBEMAP_TEX_SIZE; x++)
        {
            GLfloat fX=x-halfMapSize;
            GLfloat fY=  halfMapSize;
            GLfloat fZ=z-halfMapSize;
            GLfloat oolen=1.0f/sqrt(fX*fX + fY*fY + fZ*fZ);
            fX *= oolen;  fY *= oolen;  fZ *= oolen;
            cubemapPY[z*3*CUBEMAP_TEX_SIZE + x*3 + 0]=PackFloatInByte(fX);
            cubemapPY[z*3*CUBEMAP_TEX_SIZE + x*3 + 1]=PackFloatInByte(fY);
            cubemapPY[z*3*CUBEMAP_TEX_SIZE + x*3 + 2]=PackFloatInByte(fZ);
        }

    // Negative Y face
    for (unsigned long z=0; z<CUBEMAP_TEX_SIZE; z++)
        for (unsigned long x=0; x<CUBEMAP_TEX_SIZE; x++)
        {
            GLfloat fX=x-halfMapSize;
            GLfloat fY= -halfMapSize;
            GLfloat fZ=  halfMapSize-z;
            GLfloat oolen=1.0f/sqrt(fX*fX + fY*fY + fZ*fZ);
            fX *= oolen;  fY *= oolen;  fZ *= oolen;
            cubemapNY[z*3*CUBEMAP_TEX_SIZE + x*3 + 0]=PackFloatInByte(fX);
            cubemapNY[z*3*CUBEMAP_TEX_SIZE + x*3 + 1]=PackFloatInByte(fY);
            cubemapNY[z*3*CUBEMAP_TEX_SIZE + x*3 + 2]=PackFloatInByte(fZ);
        }

    // Positive Z face
    for (unsigned long y=0; y<CUBEMAP_TEX_SIZE; y++)
        for (unsigned long x=0; x<CUBEMAP_TEX_SIZE; x++)
        {
            GLfloat fX=x-halfMapSize;
            GLfloat fY=  halfMapSize-y;
            GLfloat fZ=  halfMapSize;
            GLfloat oolen=1.0f/sqrt(fX*fX + fY*fY + fZ*fZ);
            fX *= oolen;  fY *= oolen;  fZ *= oolen;
            cubemapPZ[y*3*CUBEMAP_TEX_SIZE + x*3 + 0]=PackFloatInByte(fX);
            cubemapPZ[y*3*CUBEMAP_TEX_SIZE + x*3 + 1]=PackFloatInByte(fY);
            cubemapPZ[y*3*CUBEMAP_TEX_SIZE + x*3 + 2]=PackFloatInByte(fZ);
        }

    // Negative Z face
    for (unsigned long y=0; y<CUBEMAP_TEX_SIZE; y++)
        for (unsigned long x=0; x<CUBEMAP_TEX_SIZE; x++)
        {
            GLfloat fX= halfMapSize-x;
            GLfloat fY= halfMapSize-y;
            GLfloat fZ=-halfMapSize;
            GLfloat oolen=1.0f/sqrt(fX*fX + fY*fY + fZ*fZ);
            fX *= oolen;  fY *= oolen;  fZ *= oolen;
            cubemapNZ[y*3*CUBEMAP_TEX_SIZE + x*3 + 0]=PackFloatInByte(fX);
            cubemapNZ[y*3*CUBEMAP_TEX_SIZE + x*3 + 1]=PackFloatInByte(fY);
            cubemapNZ[y*3*CUBEMAP_TEX_SIZE + x*3 + 2]=PackFloatInByte(fZ);
        }


    char* cubemap[6]={ cubemapPX, cubemapPY, cubemapPZ, cubemapNX, cubemapNY, cubemapNZ };
    return TextureMapManagerImplT::Get().GetTextureMapCubeInternal(cubemap, CUBEMAP_TEX_SIZE, CUBEMAP_TEX_SIZE, 3, true, MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear, MapCompositionT::ClampToEdge, MapCompositionT::ClampToEdge));
}


void RendererImplT::Initialize()
{
    // Initialize OpenGL extensions.
    cf::Init_GL_ARB_multitexture();
    cf::Init_GL_ARB_texture_cube_map();
    cf::Init_GL_ARB_texture_compression();
    cf::Init_GL_EXT_stencil_wrap();
    cf::Init_GL_EXT_stencil_two_side();
    cf::Init_GL_ARB_vertex_and_fragment_program();

    if (cf::GL_ARB_texture_compression_AVAIL)
        glHint(GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);

    glEnable(GL_VERTEX_PROGRAM_ARB);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);

    NormalizationCubeMap=InitNormalizationCubeMap();

    char GrayData[]={ 128, 128, 128, 0 };
    HelperGrayMap=TextureMapManagerImplT::Get().GetTextureMap2DInternal(GrayData, 1, 1, 3, true, MapCompositionT());

    char BlackData[]={ 0, 0, 0, 0 };
    HelperBlackMap=TextureMapManagerImplT::Get().GetTextureMap2DInternal(BlackData, 1, 1, 3, true, MapCompositionT());

    char LightBlueData[]={ 0x80, 0x80, 0xFF, 0 };
    HelperLightBlue001Map=TextureMapManagerImplT::Get().GetTextureMap2DInternal(LightBlueData, 1, 1, 3, true, MapCompositionT());


    // Bring the local matrices in sync with the OpenGL matrices.
    MatrixT ProjMat;
    glGetFloatv(GL_PROJECTION_MATRIX, &ProjMat.m[0][0]);

    SetMatrix(MODEL_TO_WORLD, MatrixT());               // Start with the identity matrix.
    SetMatrix(WORLD_TO_VIEW,  MatrixT());               // Start with the identity matrix.
    SetMatrix(PROJECTION,     ProjMat.GetTranspose());  // The OpenGL window might have already set this, so we have to overtake it from OpenGL!

    IsInitialized=true;
}


void RendererImplT::Release()
{
    if (!IsInitialized) return;

    TextureMapManagerImplT::Get().FreeTextureMap(HelperGrayMap);         HelperGrayMap        =NULL;
    TextureMapManagerImplT::Get().FreeTextureMap(HelperBlackMap);        HelperBlackMap       =NULL;
    TextureMapManagerImplT::Get().FreeTextureMap(HelperLightBlue001Map); HelperLightBlue001Map=NULL;
    TextureMapManagerImplT::Get().FreeTextureMap(NormalizationCubeMap);  NormalizationCubeMap =NULL;

    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_VERTEX_PROGRAM_ARB);

    // TODO: Check for OpenGL Errors (but at most say 20, to avoid inf loop!).

    IsInitialized=false;
}


const char* RendererImplT::GetDescription() const
{
    return "Renderer for ARB vertex and fragment programs, version 1.0.";
}


// void RendererImplT::SetBaseDir(const std::string& BaseDir_)
// {
//     BaseDir=BaseDir_;
// }


// const std::string& RendererImplT::GetBaseDir()
// {
//     return BaseDir;
// }


RenderMaterialT* RendererImplT::RegisterMaterial(const MaterialT* Material) const
{
    // For each material map, get a texture
    ;

    // The RenderMaterial alone cannot set-up the OpenGL render state alone,
    // because it does not know which pass is currently to be drawn (ambient, stencil, light),
    // and it does not know where (i.e. which texture unit) the Cg shaders expect the textures to be bound.

    // Moreover, not every RenderMaterial must come with all textures - many Material may e.g. have no luma or specular map.
    // Some may not even have a lightmap...

    // This brings us back to the concept of SHADERS.
    // A material can know with which shader it is rendered best (may also be user defineable).
    // (In fact, there are at least two shaders per material: One for the ambient pass, and one for the light pass,
    //  and each shader may in turn have multiple rendering passes. This is like FX!!)
    // A shader in turn knows what of the material it needs where (tex units...).

    // For internal optimization (sorting for reduced state changes), maybe we should cache all meshes of the current pass (ambient, stencil, or light).
    // This is the middle ground between caching everything of a scene, and caching only a single mesh.
    // YannL does the same, and the he uses radix sort for sorting by shader ID and
    // "shader param pointer (so that shaders with the same parameter set are grouped)".
    // (In our case, the shader params are probably the Material (pointers) themselves...).


    // Keeping the RenderMaterials array is probably not very useful... (could directly " return new RenderMaterialT(...); ").
    // However, if we want the user to delete the returned pointer on clean-up time, we also have to provide a
    // FreeMaterial(...) function, so that the delete is done in the same module (exe/dll boundary!).
    //RenderMaterials.PushBack(new RenderMaterialT(Material));
    //return RenderMaterials[RenderMaterials.Size()-1];

    if (Material==NULL) return NULL;

    return new RenderMaterialT(Material);
}


const MaterialT* RendererImplT::GetMaterialFromRM(MatSys::RenderMaterialT* RenderMaterial) const
{
    if (RenderMaterial==NULL) return NULL;

    return RenderMaterial->Material;
}


unsigned long RendererImplT::GetAmbientShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const
{
    if (RenderMaterial==NULL) return 0;

    for (unsigned long ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
        if (RenderMaterial->AmbientShader==GetShaderRepository()[ShaderNr])
            return ShaderNr;

    return 0xFFFFFFFF;      // Should never get here!
}


unsigned long RendererImplT::GetLightShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const
{
    if (RenderMaterial==NULL) return 0;

    for (unsigned long ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
        if (RenderMaterial->LightShader==GetShaderRepository()[ShaderNr])
            return ShaderNr;

    return 0xFFFFFFFF;      // Should never get here!
}


void RendererImplT::FreeMaterial(RenderMaterialT* RenderMaterial)
{
    // Trying to delete the currently active material? Deactivate it first.
    if (RenderMaterial==CurrentRenderMaterial)
    {
        LockCurrentMaterial(false);
        SetCurrentMaterial(NULL);
    }

    // The user code cannot simply delete directly, because the pointer is probably from a different heap (the "exe/dll boundary").
    // (And of course it could not do easily the above check.)
    delete RenderMaterial;
}


void RendererImplT::BeginFrame(double Time)
{
    // Set the "time" symbol of the expressions at the current time.
    ExpressionSymbols.Time=float(Time);

    // These affect also glClear(), see OpenGL Red Book, p. 435.
    OpenGLStateT::GetInstance()->ColorMask(true, true, true, true);
    OpenGLStateT::GetInstance()->DepthMask(true);

    // If the underlying windowing system was restarted since the last frame (e.g. the user changed the screen resolution),
    // we got a notification from the user code. Then this is the right place to reinitialize the rendering resources.
    // (Not really... TextureMapImplTs handle that all by themselves.  It IS right however that this place might be
    //  reasonable for (re-)precaching after an RC change).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset the counter for counting the shader changes per frame.
    ShaderChangeCounter=0;

#ifdef DEBUG
    GLenum Error=glGetError();

    if (Error!=GL_NO_ERROR)
        Console->Print(cf::va("glGetError()==%i\n", Error));
#endif
}


// #include <stdio.h>

void RendererImplT::EndFrame()
{
    // Console->Print(cf::va("%4lu", ShaderChangeCounter));
    // static int LineWrap=0;
    // LineWrap++;
    // if (LineWrap==20) { Console->Print("\n"); LineWrap=0; }
}


void RendererImplT::PreCache()
{
    // An idea from NVidias GPU Programming Guide:
    // Allocate the vertex and pixels shaders before the textures in order to minimize video memory thrashing.
    // This also makes sure that the shaders are already compiled and loaded when they're first used.
    if (CurrentShader!=NULL) CurrentShader->Deactivate();
    for (unsigned long ShaderNr=0; ShaderNr<GetShaderRepository().Size(); ShaderNr++)
    {
        GetShaderRepository()[ShaderNr]->Activate();
        GetShaderRepository()[ShaderNr]->Deactivate();
    }
    if (CurrentShader!=NULL) CurrentShader->Activate();


    OpenGLStateT::GetInstance()->Disable(GL_ALPHA_TEST);
    OpenGLStateT::GetInstance()->Disable(GL_BLEND);
    OpenGLStateT::GetInstance()->Disable(GL_CULL_FACE);

    OpenGLStateT::GetInstance()->DepthFunc(GL_LEQUAL);
    OpenGLStateT::GetInstance()->ColorMask(true, true, true, true);
    OpenGLStateT::GetInstance()->DepthMask(false);      // Want/need at lot of overdraw below...
    OpenGLStateT::GetInstance()->Disable(GL_STENCIL_TEST);
    if (cf::GL_EXT_stencil_two_side_AVAIL)
    {
        OpenGLStateT::GetInstance()->Disable(GL_STENCIL_TEST_TWO_SIDE_EXT);
        OpenGLStateT::GetInstance()->ActiveStencilFace(GL_FRONT);
    }

    OpenGLStateT::GetInstance()->ActiveTextureUnit(GL_TEXTURE0_ARB);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Set the modelview matrix to identity.
    SetMatrix(MODEL_TO_WORLD, MatrixT());
    SetMatrix(WORLD_TO_VIEW,  MatrixT());
    OpenGLStateT::GetInstance()->LoadMatrix(OpenGLStateT::MODELVIEW, GetDepRelMatrixModelView());

    // These are all the textures that we want to pre-cache.
    const ArrayT<TextureMapImplT*>& TexMapRepository=TextureMapManagerImplT::Get().GetTexMapRepository();

    for (unsigned long TexNr=0; TexNr<TexMapRepository.Size(); TexNr++)
    {
        // Reverse the order in which we pre-cache.
        // Assuming that the most important textures occur in TexMapRepository first, maybe it helps to upload them last...
        unsigned long TexNr_=TexMapRepository.Size()-TexNr-1;

        // Bind the OpenGL texture object of this texture to texture unit 0.
        GLuint TexObject=TexMapRepository[TexNr_]->GetOpenGLObject();

        if (dynamic_cast<TextureMap2DT*>(TexMapRepository[TexNr_]))
        {
            OpenGLStateT::GetInstance()->Enable(GL_TEXTURE_2D);
            OpenGLStateT::GetInstance()->BindTexture(GL_TEXTURE_2D, TexObject);
        }
        else if (dynamic_cast<TextureMapCubeT*>(TexMapRepository[TexNr_]))
        {
            OpenGLStateT::GetInstance()->Enable(GL_TEXTURE_CUBE_MAP_ARB);
            OpenGLStateT::GetInstance()->BindTexture(GL_TEXTURE_CUBE_MAP_ARB, TexObject);
        }

        // Render a simple mesh with the bound texture.
        glBegin(GL_TRIANGLE_FAN);
        {
            cf::glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 0.0); glVertex3f(-200.0,  200.0, -1000.0);
            cf::glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 1.0, 0.0); glVertex3f( 200.0,  200.0, -1000.0);
            cf::glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 1.0, 1.0); glVertex3f( 200.0, -200.0, -1000.0);
            cf::glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 1.0); glVertex3f(-200.0, -200.0, -1000.0);
        }
        glEnd();
    }

    // Console->Print(cf::va("%lu textures pre-cached.\n", TexMapRepository.Size()));
}


void RendererImplT::SetCurrentRenderAction(RenderActionT RA)
{
    if (RA==STENCILSHADOW)
    {
        glClear(GL_STENCIL_BUFFER_BIT);
    }

    CurrentRenderAction=RA;

    // Based on the CurrentRenderAction and the CurrentRenderMaterial, set the CurrentShader.
    SetCurrentShader();
}


RendererImplT::RenderActionT RendererImplT::GetCurrentRenderAction() const
{
    return CurrentRenderAction;
}


void RendererImplT::SetGenPurposeRenderingParam (unsigned long Index, float Value)
{
    if (Index>=ExpressionSymbols.GenFloat.Size()) return;

    ExpressionSymbols.GenFloat[Index]=Value;
}


void RendererImplT::SetGenPurposeRenderingParam (unsigned long Index, int Value)
{
    if (Index>=ExpressionSymbols.GenInt.Size()) return;

    ExpressionSymbols.GenInt[Index]=Value;
}


float RendererImplT::GetGenPurposeRenderingParamF(unsigned long Index) const
{
    if (Index>=ExpressionSymbols.GenFloat.Size()) return 0.0f;

    return ExpressionSymbols.GenFloat[Index];
}


int RendererImplT::GetGenPurposeRenderingParamI(unsigned long Index) const
{
    if (Index>=ExpressionSymbols.GenInt.Size()) return 0;

    return ExpressionSymbols.GenInt[Index];
}


void RendererImplT::SetCurrentAmbientLightColor(float r, float g, float b)
{
    ExpressionSymbols.AmbientLightColor[0]=r;
    ExpressionSymbols.AmbientLightColor[1]=g;
    ExpressionSymbols.AmbientLightColor[2]=b;
}


float* RendererImplT::GetCurrentAmbientLightColor()
{
    return ExpressionSymbols.AmbientLightColor;
}


const float* RendererImplT::GetCurrentAmbientLightColor() const
{
    return ExpressionSymbols.AmbientLightColor;
}


void RendererImplT::SetCurrentLightSourcePosition(float x, float y, float z)
{
    CurrentLightingParams.LightSourcePosition[0]=x;
    CurrentLightingParams.LightSourcePosition[1]=y;
    CurrentLightingParams.LightSourcePosition[2]=z;
}


float* RendererImplT::GetCurrentLightSourcePosition()
{
    return CurrentLightingParams.LightSourcePosition;
}


const float* RendererImplT::GetCurrentLightSourcePosition() const
{
    return CurrentLightingParams.LightSourcePosition;
}


void RendererImplT::SetCurrentLightSourceRadius(float r)
{
    CurrentLightingParams.LightSourceRadius=r;
}


float& RendererImplT::GetCurrentLightSourceRadius()
{
    return CurrentLightingParams.LightSourceRadius;
}


const float& RendererImplT::GetCurrentLightSourceRadius() const
{
    return CurrentLightingParams.LightSourceRadius;
}


void RendererImplT::SetCurrentLightSourceDiffuseColor(float r, float g, float b)
{
    CurrentLightingParams.LightSourceDiffuseColor[0]=r;
    CurrentLightingParams.LightSourceDiffuseColor[1]=g;
    CurrentLightingParams.LightSourceDiffuseColor[2]=b;
}


float* RendererImplT::GetCurrentLightSourceDiffuseColor()
{
    return CurrentLightingParams.LightSourceDiffuseColor;
}


const float* RendererImplT::GetCurrentLightSourceDiffuseColor() const
{
    return CurrentLightingParams.LightSourceDiffuseColor;
}


void RendererImplT::SetCurrentLightSourceSpecularColor(float r, float g, float b)
{
    CurrentLightingParams.LightSourceSpecularColor[0]=r;
    CurrentLightingParams.LightSourceSpecularColor[1]=g;
    CurrentLightingParams.LightSourceSpecularColor[2]=b;
}


float* RendererImplT::GetCurrentLightSourceSpecularColor()
{
    return CurrentLightingParams.LightSourceSpecularColor;
}


const float* RendererImplT::GetCurrentLightSourceSpecularColor() const
{
    return CurrentLightingParams.LightSourceSpecularColor;
}


void RendererImplT::SetCurrentEyePosition(float x, float y, float z)
{
    CurrentLightingParams.EyePosition[0]=x;
    CurrentLightingParams.EyePosition[1]=y;
    CurrentLightingParams.EyePosition[2]=z;
}


float* RendererImplT::GetCurrentEyePosition()
{
    return CurrentLightingParams.EyePosition;
}


const float* RendererImplT::GetCurrentEyePosition() const
{
    return CurrentLightingParams.EyePosition;
}


void RendererImplT::PushLightingParameters()
{
    LightingParamsStack.PushBack(CurrentLightingParams);
}


void RendererImplT::PopLightingParameters()
{
    if (LightingParamsStack.Size()>0)
    {
        CurrentLightingParams=LightingParamsStack[LightingParamsStack.Size()-1];
        LightingParamsStack.DeleteBack();
    }
}


void RendererImplT::ClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}


void RendererImplT::Flush()
{
    glFlush();
}


/**************************************************************************************************************/
/*** BEGIN of matrix-related code.  This code happens to be common across all OpenGL-based renderers.       ***/
/**************************************************************************************************************/

const MatrixT& RendererImplT::GetMatrix(MatrixNameT MN) const
{
 // Matrix[MN].Update();

    return Matrix[MN].Matrix;
}


const MatrixT& RendererImplT::GetMatrixInv(MatrixNameT MN) const
{
    MatrixInv[MN].Update();

    return MatrixInv[MN].Matrix;
}


const MatrixT& RendererImplT::GetMatrixModelView() const
{
    MatrixModelView.Update();

    return MatrixModelView.Matrix;
}


const DepRelMatrixT& RendererImplT::GetDepRelMatrix(MatrixNameT MN) const
{
 // Matrix[MN].Update();

    return Matrix[MN];
}


const DepRelMatrixT& RendererImplT::GetDepRelMatrixInv(MatrixNameT MN) const
{
    MatrixInv[MN].Update();

    return MatrixInv[MN];
}


const DepRelMatrixT& RendererImplT::GetDepRelMatrixModelView() const
{
    MatrixModelView.Update();

    return MatrixModelView;
}


void RendererImplT::SetMatrix(MatrixNameT MN, const MatrixT& Matrix_)
{
    Matrix[MN].Matrix=Matrix_;
    Matrix[MN].Age++;
}


void RendererImplT::Translate(MatrixNameT MN, float x, float y, float z)
{
    Matrix[MN].Matrix.Translate_MT(x, y, z);
    Matrix[MN].Age++;
}


void RendererImplT::Scale(MatrixNameT MN, float scale)
{
    Matrix[MN].Matrix.Scale_MS(scale, scale, scale);
    Matrix[MN].Age++;
}


void RendererImplT::RotateX(MatrixNameT MN, float angle)
{
    Matrix[MN].Matrix.RotateX_MR(angle);
    Matrix[MN].Age++;
}


void RendererImplT::RotateY(MatrixNameT MN, float angle)
{
    Matrix[MN].Matrix.RotateY_MR(angle);
    Matrix[MN].Age++;
}


void RendererImplT::RotateZ(MatrixNameT MN, float angle)
{
    Matrix[MN].Matrix.RotateZ_MR(angle);
    Matrix[MN].Age++;
}


void RendererImplT::PushMatrix(MatrixNameT MN)
{
    MatrixStack[MN].PushBack(Matrix[MN].Matrix);
}


void RendererImplT::PopMatrix(MatrixNameT MN)
{
    const unsigned long StackSize=MatrixStack[MN].Size();

    if (StackSize>0)
    {
        Matrix[MN].Matrix=MatrixStack[MN][StackSize-1];
        MatrixStack[MN].DeleteBack();
        Matrix[MN].Age++;
    }
}

/************************************************************************************************************/
/*** END of matrix-related code.  This code happens to be common across all OpenGL-based renderers.       ***/
/************************************************************************************************************/


void RendererImplT::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}


void RendererImplT::GetViewport(int viewport[4])
{
    glGetIntegerv(GL_VIEWPORT, viewport);
}


void RendererImplT::SetSelectionBuffer(unsigned long Size, unsigned int* Buffer)
{
    glSelectBuffer(Size, Buffer);
}


unsigned long RendererImplT::SetPickingRenderMode(PickingRenderModeT PRM)
{
    switch (PRM)
    {
        case RENDER: return glRenderMode(GL_RENDER);
        case SELECT: return glRenderMode(GL_SELECT);
        default:     return glRenderMode(GL_FEEDBACK);
    }
}


void RendererImplT::InitNameStack()
{
    glInitNames();
}


void RendererImplT::LoadName(unsigned long Name)
{
    glLoadName(Name);
}


void RendererImplT::PushName(unsigned long Name)
{
    glPushName(Name);
}


void RendererImplT::PopName()
{
    glPopName();
}


void RendererImplT::SetCurrentShader()
{
    if (CurrentRenderMaterial==NULL) return;

    switch (CurrentRenderAction)
    {
        case AMBIENT:
            if (CurrentShader!=CurrentRenderMaterial->AmbientShader)
            {
                if (CurrentShader!=NULL) CurrentShader->Deactivate();
                CurrentShader=CurrentRenderMaterial->AmbientShader;
                if (CurrentShader!=NULL) CurrentShader->Activate();
                                 // else Warning("CurrentShader==NULL");

                ShaderChangeCounter++;
            }
            break;

        case STENCILSHADOW:
            if (CurrentShader!=GetStencilShadowVolumesShader())
            {
                if (CurrentShader!=NULL) CurrentShader->Deactivate();
                CurrentShader=GetStencilShadowVolumesShader();
                if (CurrentShader!=NULL) CurrentShader->Activate();
                                 // else Warning("CurrentShader==NULL");

                ShaderChangeCounter++;
            }
            break;

        case LIGHTING:
            if (CurrentShader!=CurrentRenderMaterial->LightShader)
            {
                if (CurrentShader!=NULL) CurrentShader->Deactivate();
                CurrentShader=CurrentRenderMaterial->LightShader;
                if (CurrentShader!=NULL) CurrentShader->Activate();
                                 // else Warning("CurrentShader==NULL");

                ShaderChangeCounter++;
            }
            break;
    }
}


void RendererImplT::SetCurrentMaterial(RenderMaterialT* RenderMaterial)
{
    // Only do this if the current render material has not been locked.
    if (LockCurrentRM) return;

    CurrentRenderMaterial=RenderMaterial;

    // Based on the CurrentRenderAction and the CurrentRenderMaterial, set the CurrentShader.
    SetCurrentShader();
}


MatSys::RenderMaterialT* RendererImplT::GetCurrentMaterial() const
{
    return GetCurrentRenderMaterial();
}


void RendererImplT::LockCurrentMaterial(bool LockCM)
{
    LockCurrentRM=LockCM;
}


void RendererImplT::SetCurrentLightMap(TextureMapI* LightMap)
{
    CurrentLightMap=(TextureMap2DT*)LightMap;
}


void RendererImplT::SetCurrentLightDirMap(TextureMapI* LightDirMap)
{
    CurrentLightDirMap=(TextureMap2DT*)LightDirMap;
}


void RendererImplT::SetCurrentSHLMaps(const ArrayT<TextureMapI*>& SHLMaps)
{
    for (unsigned long SHLMapNr=0; SHLMapNr<16; SHLMapNr++)
        CurrentSHLMaps[SHLMapNr]=SHLMapNr<SHLMaps.Size() ? (TextureMap2DT*)SHLMaps[SHLMapNr] : NULL;
}


void RendererImplT::SetCurrentSHLLookupMap(TextureMapI* SHLLookupMap)
{
    CurrentSHLLookupMap=(TextureMap2DT*)SHLLookupMap;
}


/*void RendererImplT::BeginGeom(type)
{
    ;
}


void RendererImplT::VertexAttrib1(...)    // e.g. TexCoord
void RendererImplT::VertexAttrib2(...)    // e.g. LMCoord
void RendererImplT::Vertex(...)
void RendererImplT::EndGeom() */


void RendererImplT::RenderMesh(const MeshT& Mesh)
{
    OpenGLStateT::GetInstance()->LoadMatrix(OpenGLStateT::PROJECTION, GetDepRelMatrix(PROJECTION));
    OpenGLStateT::GetInstance()->LoadMatrix(OpenGLStateT::MODELVIEW,  GetDepRelMatrixModelView());

    if (CurrentRenderMaterial==NULL) return;
    if (CurrentShader        ==NULL) return;

    CurrentShader->RenderMesh(Mesh);
}


RenderMaterialT* RendererImplT::GetCurrentRenderMaterial() const
{
    return CurrentRenderMaterial;
}


TextureMap2DT*   RendererImplT::GetCurrentLightMap     () const { return CurrentLightMap;      }
TextureMap2DT*   RendererImplT::GetCurrentLightDirMap  () const { return CurrentLightDirMap;   }
TextureMap2DT*   RendererImplT::GetCurrentSHLMap(unsigned long Index) const { return Index<16 ? CurrentSHLMaps[Index] : NULL; }
TextureMap2DT*   RendererImplT::GetCurrentSHLLookupMap () const { return CurrentSHLLookupMap;  }
TextureMapCubeT* RendererImplT::GetNormalizationCubeMap() const { return NormalizationCubeMap; }
TextureMap2DT*   RendererImplT::GetHelperGrayMap       () const { return HelperGrayMap;        }
TextureMap2DT*   RendererImplT::GetHelperBlackMap       () const { return HelperBlackMap;        }
TextureMap2DT*   RendererImplT::GetHelperLightBlue001Map() const { return HelperLightBlue001Map; }


unsigned long RendererImplT::GetInitCounter() const
{
    return InitCounter;
}
