/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************/
/*** Shader ***/
/**************/

// Required for #include <GL/gl.h> with MS VC++.
#if defined(_WIN32) && defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

// This is required for cg.h to get the function calling conventions (Win32 import/export/lib) right.
#ifdef _WIN32
#undef WIN32    // VC++ 8 seems to predefine something here that is not an integer, and thus doesn't work with the "#if WIN32" expression in cgGL.h.
#define WIN32 1
#endif

#include <GL/gl.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include "../../Common/OpenGLState.hpp"
#include "../RendererImpl.hpp"
#include "../RenderMaterial.hpp"
#include "../Shader.hpp"
#include "../TextureMapImpl.hpp"
#include "../../Mesh.hpp"
#include "_CommonCgHelpers.hpp"
#include "../../Common/OpenGLEx.hpp"


using namespace MatSys;


class Shader_A_SkyDome : public ShaderT
{
    private:

    CGprogram     VertexShader;
    CGparameter   VS_EyePos_w;      // Eye position in world-space.

    CGprogram     FragmentShader;

    unsigned long InitCounter;


    void Initialize()
    {
        VertexShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_ARBVP1,
            // Assumptions:
            //   - The w-component of InPos is 1.0.
            " void main(in float4 InPos       : POSITION,                           \n"
            "           in float4 InColor     : COLOR,                              \n"
            "          out float4 OutPos      : POSITION,                           \n"
            "          out float4 OutColor    : COLOR,                              \n"
            "          out float3 OutTexCoord : TEXCOORD0,                          \n" // Direction vector for cube-map look-up.
            "      uniform float3 EyePos_w)                                         \n"
            " {                                                                     \n"
            // The program[0] matrix is bound to the model-to-world matrix.
            // The transpose due to row-major vs. column-major issues (see the Red Book p. 106 and the example in the ARB_vertex_program spec)
            // is *not* required here, because OpenGLStateT::LoadMatrix() already uploaded the transpose.
            // The .xzy is required due to the special coord-system axes of cube-maps (Cafu and OpenGL are right-handed systems, cube-maps are left-handed).
            "     const float3 InPos_w=mul(glstate.matrix.program[0], InPos).xzy;   \n"
            "                                                                       \n"
            "     OutPos     =mul(glstate.matrix.mvp, InPos);                       \n"
            "     OutColor   =InColor;                                              \n"
            "     OutTexCoord=InPos_w-EyePos_w.xzy;                                 \n"
            " }                                                                     \n");

        VS_EyePos_w=cgGetNamedParameter(VertexShader, "EyePos_w");


        FragmentShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_ARBFP1,
            " void main(in float4      InColor         : COLOR,         \n"
            "           in float3      InTexCoord      : TEXCOORD0,     \n"
            "          out float4      OutColor        : COLOR,         \n"
            "      uniform samplerCUBE SkyDomeSampler  : TEXUNIT0)      \n"
            " {                                                         \n"
            "  // OutColor=float4(InTexCoord, 1.0);                     \n"
            "     OutColor=texCUBE(SkyDomeSampler, InTexCoord)*InColor; \n"
            " }                                                         \n");
    }


    public:

    Shader_A_SkyDome()
    {
        VertexShader  =NULL;
        VS_EyePos_w   =NULL;

        FragmentShader=NULL;

        InitCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="A_SkyDome";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& Material) const
    {
        if (Material.NoDraw) return 0;
        if (Material.CubeMap1Comp.IsEmpty()) return 0;
        return 255;
    }

    char CanHandleLighting(const MaterialT& /*Material*/) const
    {
        return 0;
    }

    bool CanHandleStencilShadowVolumes() const
    {
        return false;
    }

    void Activate()
    {
        if (InitCounter<RendererImplT::GetInstance().GetInitCounter())
        {
            Initialize();
            InitCounter=RendererImplT::GetInstance().GetInitCounter();
        }

        // These are very expensive calls!
        cgGLBindProgram(VertexShader);
        cgGLBindProgram(FragmentShader);
    }

    void Deactivate()
    {
    }

    bool NeedsNormals() const
    {
        return false;
    }

    bool NeedsTangentSpace() const
    {
        return false;
    }

    bool NeedsXYAttrib() const
    {
        return false;
    }

    void RenderMesh(const MeshT& Mesh)
    {
        const RendererImplT&         Renderer   =RendererImplT::GetInstance();
        RenderMaterialT*             RM         =Renderer.GetCurrentRenderMaterial();
        const MaterialT&             Material   =*(RM->Material);
        const ExpressionT::SymbolsT& Sym        =Renderer.GetExpressionSymbols();
        OpenGLStateT*                OpenGLState=OpenGLStateT::GetInstance();

        const float AlphaTestValue=Material.AlphaTestValue.Evaluate(Sym).GetAsFloat();
        const float RedValue      =Material.RedGen        .Evaluate(Sym).GetAsFloat();
        const float GreenValue    =Material.GreenGen      .Evaluate(Sym).GetAsFloat();
        const float BlueValue     =Material.BlueGen       .Evaluate(Sym).GetAsFloat();
        const float AlphaValue    =Material.AlphaGen      .Evaluate(Sym).GetAsFloat();

        if (InitCounter<Renderer.GetInitCounter())
        {
            Initialize();
            InitCounter=Renderer.GetInitCounter();
        }


        // Render the cube-map.
        if (AlphaTestValue>=0.0)
        {
            OpenGLState->Enable(GL_ALPHA_TEST);
            OpenGLState->AlphaFunc(GL_GREATER, AlphaTestValue);
        }
        else OpenGLState->Disable(GL_ALPHA_TEST);

        if (Material.BlendFactorSrc!=MaterialT::None /*&& Material.BlendFactorDst!=MaterialT::None*/)
        {
            OpenGLState->Enable(GL_BLEND);
            OpenGLState->BlendFunc(OpenGLStateT::BlendFactorToOpenGL[Material.BlendFactorSrc], OpenGLStateT::BlendFactorToOpenGL[Material.BlendFactorDst]);
        }
        else OpenGLState->Disable(GL_BLEND);

        if (!Material.TwoSided)
        {
            OpenGLState->Enable(GL_CULL_FACE);
            OpenGLState->FrontFace(OpenGLStateT::WindingToOpenGL[Mesh.Winding]);
            OpenGLState->CullFace(GL_BACK);
        }
        else OpenGLState->Disable(GL_CULL_FACE);

        if (Material.DepthOffset!=0.0f)
        {
            OpenGLState->Enable(OpenGLStateT::PolygonModeToOpenGL_Offset[Material.PolygonMode]);
            OpenGLState->PolygonOffset(Material.DepthOffset, Material.DepthOffset);
        }
        else OpenGLState->Disable(OpenGLStateT::PolygonModeToOpenGL_Offset[Material.PolygonMode]);

        OpenGLState->PolygonMode(OpenGLStateT::PolygonModeToOpenGL[Material.PolygonMode]);
        OpenGLState->DepthFunc(GL_LEQUAL);
        OpenGLState->ColorMask(Material.AmbientMask[0], Material.AmbientMask[1], Material.AmbientMask[2], Material.AmbientMask[3]);
        OpenGLState->DepthMask(Material.AmbientMask[4]);
        OpenGLState->Disable(GL_STENCIL_TEST);
        if (cf::GL_EXT_stencil_two_side_AVAIL)
        {
            OpenGLState->Disable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            OpenGLState->ActiveStencilFace(GL_FRONT);
        }

        OpenGLState->ActiveTextureUnit(GL_TEXTURE0_ARB);
        OpenGLState->Enable(GL_TEXTURE_CUBE_MAP_ARB);
        OpenGLState->BindTexture(GL_TEXTURE_CUBE_MAP_ARB, RM->Cube1TexMap->GetOpenGLObject());


        OpenGLState->LoadMatrix(OpenGLStateT::MATRIX0, Renderer.GetDepRelMatrix(RendererI::MODEL_TO_WORLD));

        // The cgGLSetParameter*() functions are very slow, so cache their parameters in order to call them as little as possible.
        const MatrixT& VW=Renderer.GetMatrixInv(RendererI::WORLD_TO_VIEW);

        static float EyePosCache_w[3]={ 0.0, 0.0, 0.0 };
        if (VW.m[0][3]!=EyePosCache_w[0] || VW.m[1][3]!=EyePosCache_w[1] || VW.m[2][3]!=EyePosCache_w[2])
        {
            cgGLSetParameter3f(VS_EyePos_w, VW.m[0][3], VW.m[1][3], VW.m[2][3]);
            EyePosCache_w[0]=VW.m[0][3];
            EyePosCache_w[1]=VW.m[1][3];
            EyePosCache_w[2]=VW.m[2][3];
        }


        glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_A_SkyDome MyShader;
