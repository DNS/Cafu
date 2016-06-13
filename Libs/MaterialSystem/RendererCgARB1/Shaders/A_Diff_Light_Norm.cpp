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


class Shader_A_Diff_Light_Norm : public ShaderT
{
    private:

    CGprogram     VertexShader;
    CGparameter   VS_EyePos;
    CGprogram     FragmentShader;

    unsigned long InitCounter;


    void Initialize()
    {
        VertexShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_ARBVP1,
            " void main(in float4 InPos            : POSITION,      \n"
            "           in float3 InNormal         : NORMAL,        \n"
            "           in float3 InTangent        : TANGENT,       \n"
            "           in float3 InBiNormal       : BINORMAL,      \n"
            "           in float4 InColor          : COLOR,         \n"
            "           in float2 InTexCoord_Diff  : TEXCOORD0,     \n"
            "           in float2 InTexCoord_LiMa  : TEXCOORD1,     \n"
            "          out float4 OutPos           : POSITION,      \n"
            "          out float4 OutColor         : COLOR,         \n"
            "          out float2 OutTexCoord_Diff : TEXCOORD0,     \n"
            "          out float2 OutTexCoord_LiMa : TEXCOORD1,     \n"
            "          out float3 OutEyeDir        : TEXCOORD2,     \n"
            "      uniform float3 EyePos)                           \n"
            " {                                                     \n"
            "     const float3x3 RotMat=float3x3(InTangent,         \n"
            "                                    InBiNormal,        \n"
            "                                    InNormal);         \n"
            "                                                       \n"
            "     OutPos          =mul(glstate.matrix.mvp, InPos);  \n"
            "     OutColor        =InColor;                         \n"
            "     OutTexCoord_Diff=InTexCoord_Diff;                 \n"
            "     OutTexCoord_LiMa=InTexCoord_LiMa;                 \n"
            "     OutEyeDir       =mul(RotMat, EyePos-InPos.xyz);   \n"
            " }                                                     \n");

        VS_EyePos=cgGetNamedParameter(VertexShader, "EyePos");


        FragmentShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_ARBFP1,
            " void main(in half4    InColor           : COLOR,                                          \n"
            "           in half2    InTexCoord_Diff   : TEXCOORD0,                                      \n"
            "           in half2    InTexCoord_LiMa   : TEXCOORD1,                                      \n"
            "           in half3    EyeDir            : TEXCOORD2,                                      \n"
            "          out half4    OutColor          : COLOR,                                          \n"
            "      uniform sampler2D DiffuseMapSampler  : TEXUNIT0,                                     \n"
            "      uniform sampler2D LightMapSampler    : TEXUNIT1,                                     \n"
            "      uniform sampler2D LightDirMapSampler : TEXUNIT2,                                     \n"
            "      uniform sampler2D NormalMapSampler   : TEXUNIT3,                                     \n"
            "      uniform sampler2D SpecMapSampler     : TEXUNIT4)                                     \n"
            " {                                                                                         \n"
            "     const half4 DiffuseC    =     tex2D(DiffuseMapSampler,  InTexCoord_Diff);             \n"
            "     const half4 SpecularC   =     tex2D(SpecMapSampler,     InTexCoord_Diff);             \n"
            "     const half4 LightMapC   =     tex2D(LightMapSampler,    InTexCoord_LiMa);             \n"
            "     const half4 LightMapDirC=     tex2D(LightDirMapSampler, InTexCoord_LiMa);             \n"
            "     const half3 LightMapDir =2.0*(LightMapDirC.xyz-0.5);                                  \n"
            "     const half3 Normal      =2.0*(tex2D(NormalMapSampler,   InTexCoord_Diff).xyz-0.5);    \n"
            "                                                                                           \n"
            "     const half3 HalfwayDir =normalize(normalize(EyeDir)+LightMapDir);                     \n"
            "                                                                                           \n"
            "     const half4 diff       =half4(saturate(dot(LightMapDir, Normal)).xxx, 1.0);           \n"
            "     const half4 spec       =half4(pow(saturate(dot(HalfwayDir, Normal)), 32.0).xxx, 0.0); \n"
            "                                                                                           \n"
            "     const half4 LightMapCorrectedC=half4(LightMapC.xyz/LightMapDirC.w, 1.0);              \n"
            "                                                                                           \n"
            "     OutColor=LightMapCorrectedC*InColor*(diff*DiffuseC + spec*SpecularC);                 \n"
            " }                                                                                         \n");
    }


    public:

    Shader_A_Diff_Light_Norm()
    {
        VertexShader  =NULL;
        VS_EyePos     =NULL;
        FragmentShader=NULL;

        InitCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="A_Diff_Light_Norm";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& Material) const
    {
        if (Material.NoDraw) return 0;

        if ( Material.DiffMapComp .IsEmpty()) return 0;
        if ( Material.LightMapComp.IsEmpty()) return 0;
        if ( Material.NormMapComp .IsEmpty()) return 0;
        if (!Material.LumaMapComp .IsEmpty()) return 0;

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
        const float*                 EyePos     =Renderer.GetCurrentEyePosition();

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


        // Render the diffuse map.
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
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->UseDefaultLightMap && Renderer.GetCurrentLightMap()!=NULL ? Renderer.GetCurrentLightMap()->GetOpenGLObject() : RM->LightTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE2_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->UseDefaultLightMap && Renderer.GetCurrentLightDirMap()!=NULL ? Renderer.GetCurrentLightDirMap()->GetOpenGLObject() : Renderer.GetHelperLightBlue001Map()->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE3_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->NormTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE4_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, Material.SpecMapComp.IsEmpty() ? Renderer.GetHelperBlackMap()->GetOpenGLObject() : RM->SpecTexMap->GetOpenGLObject());


        // The cgGLSetParameter*() functions are very slow, so cache their parameters in order to call them as little as possible.
        static float EyePosCache[3]={ 0.0, 0.0, 0.0 };
        if (EyePos[0]!=EyePosCache[0] || EyePos[1]!=EyePosCache[1] || EyePos[2]!=EyePosCache[2])
        {
            cgGLSetParameter3fv(VS_EyePos, EyePos);
            EyePosCache[0]=EyePos[0];
            EyePosCache[1]=EyePos[1];
            EyePosCache[2]=EyePos[2];
        }


        glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                cf::glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, Mesh.Vertices[VertexNr].TextureCoord);
                cf::glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, Mesh.Vertices[VertexNr].LightMapCoord);

                glNormal3fv(Mesh.Vertices[VertexNr].Normal);                                    // Normal
                cf::glMultiTexCoord3fvARB(GL_TEXTURE6_ARB, Mesh.Vertices[VertexNr].Tangent);    // Tangent
                cf::glMultiTexCoord3fvARB(GL_TEXTURE7_ARB, Mesh.Vertices[VertexNr].BiNormal);   // BiNormal

                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_A_Diff_Light_Norm MyShader;
