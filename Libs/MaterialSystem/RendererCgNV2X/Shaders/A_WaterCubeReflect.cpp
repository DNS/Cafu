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


class Shader_A_WaterWithCubeReflection : public ShaderT
{
    private:

    CGprogram     VertexShader;
    CGparameter   VS_ModelViewProjMatrix;
    CGparameter   VS_ModelWorldMatrix;
    CGparameter   VS_EyePos_w;      // Eye position in world-space.
    CGparameter   VS_Time;

    CGprogram     FragmentShader;
 // CGparameter   FS_1000_div_MaxAlphaDist;

    unsigned long InitCounter;
    unsigned long Matrix_MVP_Age;
    unsigned long Matrix_MW_Age;


    void Initialize()
    {
        VertexShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_VP20,
            // Assumptions:
            //   - The w-component of InPos is 1.0.
            " void main(in float4   InPos             : POSITION,               \n"
            "           in float4   InColor           : COLOR,                  \n"
            "           in float2   InTexCoord        : TEXCOORD0,              \n"
            "          out float4   OutPos            : POSITION,               \n"
            "          out float4   OutColor          : COLOR,                  \n"
            "          out float2   OutTexCoord_Water : TEXCOORD0,              \n"
            "          out float3   OutTexCoord_Sky   : TEXCOORD1,              \n"
            "          out float3   OutEyeToPos_w1    : TEXCOORD2,              \n"
         // "          out float3   OutEyeToPos_w2    : TEXCOORD3,              \n"
            "      uniform float4x4 ModelViewProjMatrix,                        \n"
            "      uniform float4x4 ModelWorldMatrix,                           \n"
            "      uniform float    Time,                                       \n"
            "      uniform float3   EyePos_w)                                   \n"
            " {                                                                 \n"
            // The ModelWorldMatrix matrix is bound to the model-to-world matrix, properly transposed as required.
            "     const float3 InPos_w   =mul(ModelWorldMatrix, InPos).xyz;     \n"
            "     const float3 EyeToPos_w=InPos_w-EyePos_w;                     \n"
            "     const float3 Reflect_w =float3(EyeToPos_w.xy, -EyeToPos_w.z); \n" // Reflect_w = reflect(EyeToPos_w, float3(0, 0, 1));
            "                                                                   \n"
            "     OutPos           =mul(ModelViewProjMatrix, InPos);            \n"
            "     OutColor         =InColor;                                    \n"
            "     OutTexCoord_Water=InTexCoord+float2(0.6, 0.8)*Time;           \n"
            "     OutTexCoord_Sky  =Reflect_w.xzy;                              \n" // The .xzy is required due to the special coord-system axes of cube-maps (Cafu and OpenGL are right-handed systems, cube-maps are left-handed).
            "     OutEyeToPos_w1   =EyeToPos_w;                                 \n"
         // "     OutEyeToPos_w2   =EyeToPos_w;                                 \n"
            " }                                                                 \n");

        VS_ModelViewProjMatrix=cgGetNamedParameter(VertexShader, "ModelViewProjMatrix");
        VS_ModelWorldMatrix   =cgGetNamedParameter(VertexShader, "ModelWorldMatrix");
        VS_EyePos_w           =cgGetNamedParameter(VertexShader, "EyePos_w");
        VS_Time               =cgGetNamedParameter(VertexShader, "Time");


        FragmentShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_FP20,
            " void main(in float4      InColor          : COLOR,                                        \n"
            "           in float2      InTexCoord_Water : TEXCOORD0,                                    \n"
            "           in float3      InTexCoord_Sky   : TEXCOORD1,                                    \n"
            "           in float3      InEyeToPos_w1    : TEXCOORD2,                                    \n"
         // "           in float3      InEyeToPos_w2    : TEXCOORD3,                                    \n"
            "          out float4      OutColor         : COLOR,                                        \n"
         // "      uniform float       _1000_div_MaxAlphaDist,                                          \n" // 1000 div the max. distance from which you can see ground, i.e. where alpha is less than 1.
            "      uniform sampler2D   WaterSampler     : TEXUNIT0,                                     \n"
            "      uniform samplerCUBE SkyDomeSampler   : TEXUNIT1,                                     \n"
            "      uniform samplerCUBE NormCubeMap      : TEXUNIT2)                                     \n"
            " {                                                                                         \n"
            "     const float4 SkyColor   =texCUBE(SkyDomeSampler, InTexCoord_Sky);                     \n"
            "     const float4 WaterColor =tex2D(WaterSampler, InTexCoord_Water);                       \n"
            "     const float3 EyeToPosN_w=2.0*(texCUBE(NormCubeMap, InEyeToPos_w1).xyz-0.5);           \n"
         // "     const float  Len        =dot(InEyeToPos_w2, EyeToPosN_w);                             \n" // A nice idea that would correctly work if not the range of floats was -2 to +2 (??) in this profile! See pages 270 and 271 of "The Cg Tutorial".
            "                                                                                           \n"
            "     const float  f1         =saturate(1+EyeToPosN_w.z);                                   \n"
            "     const float  f2         =f1*f1;                                                       \n"
            "     const float  f5         =f2*f2*f1;                                                    \n"
            "     const float  Fresnel1   =0.0204    +          0.9796*f5;                              \n"
            "     const float  Fresnel2   =InColor.a + (1.0-InColor.a)*f5;                              \n"
         // "     const float  Alpha      =saturate(Len*_1000_div_MaxAlphaDist + Fresnel2);             \n" // The *1000.0 comes from the vertex-shader!
            "     const float  Alpha      =Fresnel2;                                                    \n"
            "                                                                                           \n"
            "     OutColor=float4(((1.0-Fresnel1)*WaterColor + Fresnel1*SkyColor*InColor).xyz, Alpha);  \n"
            " }                                                                                         \n");

     // FS_1000_div_MaxAlphaDist=cgGetNamedParameter(FragmentShader, "_1000_div_MaxAlphaDist");
    }


    public:

    Shader_A_WaterWithCubeReflection()
    {
        VertexShader          =NULL;
        VS_ModelViewProjMatrix=NULL;
        VS_ModelWorldMatrix   =NULL;
        VS_EyePos_w           =NULL;
        VS_Time               =NULL;

        FragmentShader        =NULL;
     // FS_1000_div_MaxAlphaDist=NULL;

        InitCounter=0;
        Matrix_MVP_Age=0;
        Matrix_MW_Age=0;
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
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->DiffTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE1_ARB);
        OpenGLState->Enable(GL_TEXTURE_CUBE_MAP_ARB);
        OpenGLState->BindTexture(GL_TEXTURE_CUBE_MAP_ARB, RM->Cube1TexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE2_ARB);
        OpenGLState->Enable(GL_TEXTURE_CUBE_MAP_ARB);
        OpenGLState->BindTexture(GL_TEXTURE_CUBE_MAP_ARB, Renderer.GetNormalizationCubeMap()->GetOpenGLObject());


        const unsigned long NewAgeMVP=Renderer.GetDepRelMatrixModelView().Age+Renderer.GetDepRelMatrix(RendererI::PROJECTION).Age;
        if (Matrix_MVP_Age!=NewAgeMVP)
        {
            cgGLSetStateMatrixParameter(VS_ModelViewProjMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
            Matrix_MVP_Age=NewAgeMVP;
        }


        const unsigned long NewAgeMW=Renderer.GetDepRelMatrix(RendererI::MODEL_TO_WORLD).Age;
        if (Matrix_MVP_Age!=NewAgeMW)
        {
            cgGLSetMatrixParameterfr(VS_ModelWorldMatrix, &Renderer.GetMatrix(RendererI::MODEL_TO_WORLD).m[0][0]);
            Matrix_MVP_Age=NewAgeMW;
        }


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

     // const float  Current_1000_div_MaxAlphaDist=(Material.ShaderParamExpr.Size()>0) ? 1000.0f/Material.ShaderParamExpr[0].Evaluate(Sym).GetAsFloat() : 1000.0f/30000.0f;
     // static float Cache_1000_div_MaxAlphaDist=-123.456f;
     // if (Current_1000_div_MaxAlphaDist!=Cache_1000_div_MaxAlphaDist)
     // {
     //     cgGLSetParameter1f(FS_1000_div_MaxAlphaDist, Current_1000_div_MaxAlphaDist);
     //     Cache_1000_div_MaxAlphaDist=Current_1000_div_MaxAlphaDist;
     // }

        const float  CurrentTime=(Material.ShaderParamExpr.Size()>1) ? Material.ShaderParamExpr[1].Evaluate(Sym).GetAsFloat() : 0.0f;
        static float TimeCache=-123.456f;
        if (CurrentTime!=TimeCache)
        {
            cgGLSetParameter1f(VS_Time, CurrentTime);
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
