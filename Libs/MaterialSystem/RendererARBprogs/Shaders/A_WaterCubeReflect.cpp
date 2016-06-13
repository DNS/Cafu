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

#include <GL/gl.h>

#include "../../Common/OpenGLState.hpp"
#include "../RendererImpl.hpp"
#include "../RenderMaterial.hpp"
#include "../Shader.hpp"
#include "../TextureMapImpl.hpp"
#include "../../Mesh.hpp"
#include "_CommonHelpers.hpp"
#include "../../Common/OpenGLEx.hpp"


using namespace MatSys;


class Shader_A_WaterWithCubeReflection : public ShaderT
{
    private:

    GLuint        VertexProgram;
    GLuint        FragmentProgram;

    unsigned long InitCounter;


    void Initialize()
    {
        VertexProgram=UploadProgram(GL_VERTEX_PROGRAM_ARB,
            "!!ARBvp1.0                                                     \n"
            "PARAM mvp[4]  ={ state.matrix.mvp };                           \n"
            "PARAM mw [4]  ={ state.matrix.program[0] };                    \n" // The model-to-world matrix.
            "PARAM EyePos_w=program.local[0];                               \n"
            "PARAM Time    =program.local[1];                               \n"
            "PARAM Dirs    ={ 0.6, 0.8, 0.9, -0.3 };                        \n"
            "PARAM Milli   ={ 0.001 };                                      \n"

            "DP4 result.position.x, vertex.position, mvp[0];                \n" // result.position = mul(state.matrix.mvp, vertex.position);
            "DP4 result.position.y, vertex.position, mvp[1];                \n"
            "DP4 result.position.z, vertex.position, mvp[2];                \n"
            "DP4 result.position.w, vertex.position, mvp[3];                \n"

            "MOV result.color,       vertex.color;                          \n" // result.color       = vertex.color;
            "MOV result.texcoord[0], vertex.texcoord[0];                    \n" // result.texcoord[0] = vertex.texcoord[0];
            "MAD result.texcoord[1], Time.x, Dirs.xyzw, vertex.texcoord[0]; \n" // result.texcoord[1] = vertex.texcoord[0] + (0.6,  0.8)*Time;
            "MAD result.texcoord[2], Time.x, Dirs.zwzw, vertex.texcoord[0]; \n" // result.texcoord[2] = vertex.texcoord[0] + (0.9, -0.3)*Time;

            "TEMP EyeToVertex_w;                                            \n" // EyeToVertex_w = mul(mv, vertex.position) - EyePos_w;
            "DP4 EyeToVertex_w.x, vertex.position, mw[0];                   \n"
            "DP4 EyeToVertex_w.y, vertex.position, mw[1];                   \n"
            "DP4 EyeToVertex_w.z, vertex.position, mw[2];                   \n"
            "SUB EyeToVertex_w, EyeToVertex_w, EyePos_w;                    \n"
            "MUL result.texcoord[3], EyeToVertex_w, Milli.x;                \n" // result.texcoord[3] = EyeToVertex_w / 1000.0;
            "END                                                            \n");


        FragmentProgram=UploadProgram(GL_FRAGMENT_PROGRAM_ARB,
            "!!ARBfp1.0                                                                     \n"
            "OPTION ARB_precision_hint_fastest;                                             \n"

            "PARAM  _1000_div_MaxAlphaDist=program.local[0];                                \n"
            "ATTRIB InEyeToPos_w=fragment.texcoord[3];                                      \n"


            "# Compute the normal vector first.                                             \n"
            "TEMP Normal1;                                                                  \n"
            "TEMP Normal2;                                                                  \n"
            "TEX Normal1, fragment.texcoord[1], texture[1], 2D;                             \n"
            "TEX Normal2, fragment.texcoord[2], texture[1], 2D;                             \n"

            "# This is equivalent to   Normal = 2*(Normal1-0.5) + 2*(Normal2-0.5);          \n"
            "# with the factor 2 omitted, because renormalization follows!                  \n"
            "TEMP Normal;                                                                   \n"
            "ADD Normal, Normal1, Normal2;                                                  \n"
            "ADD Normal, Normal, -1;                                                        \n"

            "# Renormalize the normal.                                                      \n"
            "DP3 Normal1.x, Normal, Normal;                                                 \n"
            "RSQ Normal1.y, Normal1.x;                                                      \n"
            "MUL Normal, Normal, Normal1.y;                                                 \n"


            "# Compute the reflection vector, Reflect_w = I - 2*N*dot(N, I);                \n"
            "TEMP Reflect_w;                                                                \n"
            "DP3 Reflect_w.w, Normal, InEyeToPos_w;                                         \n"
            "MUL Reflect_w, Normal, Reflect_w.w;                                            \n"
            "MAD Reflect_w, -Reflect_w, 2.0, InEyeToPos_w;                                  \n"


            "# Fetch the sky and the water colors.                                          \n"
            "TEMP SkyColor;                                                                 \n"
            "TEMP WaterColor;                                                               \n"
            "TEX SkyColor, Reflect_w.xzyw, texture[2], CUBE;                                \n"
            "ADD WaterColor, fragment.texcoord[0], Normal;                                  \n"
            "TEX WaterColor, WaterColor, texture[0], 2D;                                    \n"


            "# Compute the normalized incident vector and its original length,              \n"
            "# EyeToPosN_w = float4( normalize(InEyeToPos_w), length(InEyeToPos_w) );       \n"
            "TEMP EyeToPosN_w;                                                              \n"
            "DP3 EyeToPosN_w.w, InEyeToPos_w, InEyeToPos_w;                                 \n"
            "RSQ EyeToPosN_w.w, EyeToPosN_w.w;                                              \n"
            "MUL EyeToPosN_w.xyz, InEyeToPos_w, EyeToPosN_w.w;                              \n"
            "RCP EyeToPosN_w.w, EyeToPosN_w.w;                                              \n"


            "# Compute the fresnel coefficient.                                             \n"
            "TEMP Fresnel;                                                                  \n"
            "DP3 Fresnel.w, Normal, EyeToPosN_w;                                            \n"
            "ADD_SAT Fresnel.w, Fresnel.w, 1.0;                                             \n"
            "POW Fresnel.w, Fresnel.w, { 5.0, 5.0, 5.0, 5.0 }.x;                            \n" // The ATI driver yields a warning when I write only { 5.0 }.x here.

            "SUB Fresnel.z, 1.0, fragment.color.primary.a;                                  \n"

            "MAD Fresnel.x, Fresnel.w, 0.9796, 0.0204;                                      \n"
            "MAD Fresnel.y, Fresnel.w, Fresnel.z, fragment.color.primary.a;                 \n"


            "# Compute alpha of result.color.                                               \n"
            "MAD_SAT result.color.a, EyeToPosN_w.w, _1000_div_MaxAlphaDist.x, Fresnel.y;    \n"


            "# Compute rgb of result.color.                                                 \n"
            "MUL SkyColor, SkyColor, fragment.color.primary;                                \n"
            "MUL SkyColor, SkyColor, Fresnel.x;                                             \n"
            "SUB Fresnel.z, 1.0, Fresnel.x;                                                 \n"
            "MAD result.color.xyz, WaterColor, Fresnel.z, SkyColor;                         \n"
            "END                                                                            \n");
    }


    public:

    Shader_A_WaterWithCubeReflection()
    {
        VertexProgram  =0;
        FragmentProgram=0;

        InitCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="A_WaterCubeReflect";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& Material) const
    {
        if (Material.NoDraw) return 0;
        if (Material.DiffMapComp .IsEmpty()) return 0;      // A diffuse map is really required.
        if (Material.NormMapComp .IsEmpty()) return 0;
        if (Material.CubeMap1Comp.IsEmpty()) return 0;

        // This shader does not work by automatic assignment, but only when explicitly assigned in the cmat file!
        if (Material.AmbientShaderName!=GetName()) return 0;

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

        cf::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VertexProgram);
        cf::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FragmentProgram);
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
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->DiffTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE1_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->NormTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE2_ARB);
        OpenGLState->Enable(GL_TEXTURE_CUBE_MAP_ARB);
        OpenGLState->BindTexture(GL_TEXTURE_CUBE_MAP_ARB, RM->Cube1TexMap->GetOpenGLObject());


        OpenGLState->LoadMatrix(OpenGLStateT::MATRIX0, Renderer.GetDepRelMatrix(RendererI::MODEL_TO_WORLD));

        // The glProgramLocalParameter*() functions are very slow, so cache their parameters in order to call them as little as possible.
        // TODO: Is this true???  I'd suggest profiling and comparing with e.g. the Material- or ModelViewer.
        const MatrixT& VW=Renderer.GetMatrixInv(RendererI::WORLD_TO_VIEW);

        static float EyePosCache_w[3]={ 0.0, 0.0, 0.0 };
        if (VW.m[0][3]!=EyePosCache_w[0] || VW.m[1][3]!=EyePosCache_w[1] || VW.m[2][3]!=EyePosCache_w[2])
        {
            cf::glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 0, VW.m[0][3], VW.m[1][3], VW.m[2][3], 1.0f);
            EyePosCache_w[0]=VW.m[0][3];
            EyePosCache_w[1]=VW.m[1][3];
            EyePosCache_w[2]=VW.m[2][3];
        }

        const float  Current_1000_div_MaxAlphaDist=(Material.ShaderParamExpr.Size()>0) ? 1000.0f/Material.ShaderParamExpr[0].Evaluate(Sym).GetAsFloat() : 1000.0f/30000.0f;
        static float Cache_1000_div_MaxAlphaDist=-123.456f;
        if (Current_1000_div_MaxAlphaDist!=Cache_1000_div_MaxAlphaDist)
        {
            cf::glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, Current_1000_div_MaxAlphaDist, 0.0f, 0.0f, 0.0f);
            Cache_1000_div_MaxAlphaDist=Current_1000_div_MaxAlphaDist;
        }

        const float  CurrentTime=(Material.ShaderParamExpr.Size()>1) ? Material.ShaderParamExpr[1].Evaluate(Sym).GetAsFloat() : 0.0f;
        static float TimeCache=-123.456f;
        if (CurrentTime!=TimeCache)
        {
            cf::glProgramLocalParameter4fARB(GL_VERTEX_PROGRAM_ARB, 1, CurrentTime, 0.0f, 0.0f, 0.0f);
            TimeCache=CurrentTime;
        }


        glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                cf::glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, Mesh.Vertices[VertexNr].TextureCoord);

                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_A_WaterWithCubeReflection MyShader;
