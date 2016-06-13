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


class Shader_L_Diff_Norm_oSpec : public ShaderT
{
    private:

    CGprogram     VertexShader_Diff;
    CGparameter   VS_Diff_LightPos;
    CGparameter   VS_Diff_LightRadius;
    CGparameter   VS_Diff_ModelViewProjMatrix;

    CGprogram     FragmentShader_Diff;
    CGparameter   FS_Diff_LightColor;

    unsigned long InitCounter;
    unsigned long MatrixChangeCounter;


    void Initialize()
    {
        VertexShader_Diff=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_VP20,
            // Assumptions:
            //   - The w-component of InPos is 1.0.
            //   - InNormal  has unit length.
            //   - InTangent has unit length.
            " void main(in float4   InPos           : POSITION,                     \n"
            "           in float3   InNormal        : NORMAL,                       \n"
            "           in float3   InTangent       : TANGENT,                      \n"
            "           in float3   InBiNormal      : BINORMAL,                     \n"
            "           in float2   InTexCoord      : TEXCOORD0,                    \n"
            "          out float4   OutPos          : POSITION,                     \n"
            "          out float2   OutTexCoord     : TEXCOORD0,                    \n" // For diffuse map.
            "          out float3   OutLightVectorA : TEXCOORD1,                    \n" // "Normalized" (wrt. LightRadius) light vector for attenuation calculations.
            "          out float2   OutTexCoord2    : TEXCOORD2,                    \n" // For normal map.
            "          out float3   OutLightVector  : TEXCOORD3,                    \n"
            "      uniform float3   LightPos,                                       \n"
            "      uniform float    LightRadius,                                    \n"
            "      uniform float4x4 ModelViewProjMatrix)                            \n"
            " {                                                                     \n"
            "     const float3x3 RotMat=float3x3(InTangent,                         \n"
            "                                    InBiNormal,                        \n"
            "                                    InNormal);                         \n"
            "                                                                       \n"
            "     OutPos      =mul(ModelViewProjMatrix, InPos);                     \n"
            "     OutTexCoord =InTexCoord;                                          \n"
            "     OutTexCoord2=InTexCoord;                                          \n"
            "                                                                       \n"
            "     const float3 LightVector=LightPos-InPos.xyz;                      \n"
            "                                                                       \n"
            "     // Light attenuation.                                             \n"
         // "     OutLightAtten=saturate(1.0-length(LightVector)/LightRadius);      \n" // linear, 1-d/r
         // "     OutLightAtten=saturate(2000.0/length(LightVector)-0.01);          \n" // 1/d
            "     // Light vector in model space, normalized wrt. 'LightRadius'.    \n"
            "     // Omitting the range compression (which is NOT really required)  \n"
            "     // seems to cause the Cg compiler to generate bad code in the     \n"
            "     // pixel shader. TODO: Examine the assembly!                      \n"
            "     OutLightVectorA=(LightVector/LightRadius+1.0)/2.0;                \n" // Depending on the fragment shader, this will become 1-d/r or 1-(d/r)^2
            "                                                                       \n"
            "     // Light vector, rotated from model space into tangent space.     \n"
            "     OutLightVector=mul(RotMat, LightVector);                          \n"
            " }                                                                     \n");

        VS_Diff_LightPos           =cgGetNamedParameter(VertexShader_Diff, "LightPos"           );
        VS_Diff_LightRadius        =cgGetNamedParameter(VertexShader_Diff, "LightRadius"        );
        VS_Diff_ModelViewProjMatrix=cgGetNamedParameter(VertexShader_Diff, "ModelViewProjMatrix");


        FragmentShader_Diff=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_FP20,
            " void main(in float2      InTexCoord_diff             : TEXCOORD0,                             \n"
            "           in float3      InLightVectorA              : TEXCOORD1,                             \n"
            "           in float2      InTexCoord_norm             : TEXCOORD2,                             \n"
            "           in float3      InLightVector               : TEXCOORD3,                             \n"
            "          out float4      OutColor                    : COLOR,                                 \n"
            "      uniform float3      LightColor,                                                          \n"
            "      uniform sampler2D   DiffuseMapSampler           : TEXUNIT0,                              \n"
            "      uniform sampler2D   NormalMapSampler            : TEXUNIT2,                              \n"
            "      uniform samplerCUBE NormalizeCubeForLightVector : TEXUNIT3)                              \n"
            " {                                                                                             \n"
            "     const float3 LightDir=2.0*(texCUBE(NormalizeCubeForLightVector, InLightVector).xyz-0.5);  \n"
            "     const float3 Normal  =2.0*(tex2D(NormalMapSampler, InTexCoord_norm).xyz-0.5);             \n"
            "                                                                                               \n"
            "     const float  diff    =LightDir.z>0.0 ? saturate(dot(Normal, LightDir)) : 0.0;             \n" // See "The Cg Tutorial", p. 230.
            "                                                                                               \n"
            "     const float4 DiffuseC=tex2D(DiffuseMapSampler, InTexCoord_diff);                          \n"
            "     const float4 LightC  =float4(LightColor, 0.0);                                            \n"
            "                                                                                               \n"
            "     // Range-uncompress the 'InLightVectorA' (which is normalized wrt. 'LightRadius').        \n"
            "     // [ Omitting the range compression (which is NOT really required) seems to cause the Cg  \n"
            "     //   compiler to generate buggy code here. TODO: Examine the assembly! ]                  \n"
            "     // Then compute the attenuation as 1-(d/r)^2.                                             \n"
            "     // Note that it would actually be nice to remove the ^2 by some 1D texture lookup that    \n"
            "     // returns the square root, but the strict binding of texture units to texture samplers   \n"
            "     // in this profile makes that impossible.                                                 \n"
            "     // IMPORTANT: Note that 'InLightVector' and 'InLightVectorA' are ENTIRELY DIFFERENT!      \n"
            "     // Only 'InLightVectorA' is good for attenuation computations (model space), while        \n"
            "     // 'InLightVector' (local tangent space) takes SmoothGroups into account!!!               \n"
            "     // So they must never be collapsed, even if the profile admitted that!                    \n"
            "     const float3 LightTmp=2.0*(InLightVectorA-0.5);                                           \n"
            "     const float  Atten   =saturate(1.0-dot(LightTmp, LightTmp));                              \n"
            "                                                                                               \n"
            "     OutColor=Atten*LightC*diff*DiffuseC;                                                      \n"
            " }                                                                                             \n");

        FS_Diff_LightColor=cgGetNamedParameter(FragmentShader_Diff, "LightColor");
    }


    public:

    Shader_L_Diff_Norm_oSpec()
    {
        VertexShader_Diff          =NULL;
        VS_Diff_LightPos           =NULL;
        VS_Diff_LightRadius        =NULL;
        VS_Diff_ModelViewProjMatrix=NULL;

        FragmentShader_Diff        =NULL;
        FS_Diff_LightColor         =NULL;

        InitCounter=0;
        MatrixChangeCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="L_Diff_Norm_oSpec";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& /*Material*/) const
    {
        return 0;
    }

    char CanHandleLighting(const MaterialT& Material) const
    {
        if (Material.NoDraw    ) return 0;
        if (Material.NoDynLight) return 0;

        if ( Material.DiffMapComp.IsEmpty()) return 0;
        if ( Material.NormMapComp.IsEmpty()) return 0;
        // SpecularMap is optional - we never render it anyway.
        // Support for the specular pass was removed from svn revision 19 on, because the very frequent calls to
        // cgGLBindProgram() that were required were by far to expensive - framerate was only near 1.8 FPS before,
        // and climbed to 9.2 FPS after the removal.

        return 255;
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
        cgGLBindProgram(VertexShader_Diff);
        cgGLBindProgram(FragmentShader_Diff);
    }

    void Deactivate()
    {
    }

    bool NeedsNormals() const
    {
        return true;
    }

    bool NeedsTangentSpace() const
    {
        return true;
    }

    bool NeedsXYAttrib() const
    {
        return false;
    }

    void RenderMesh(const MeshT& Mesh)
    {
        const RendererImplT& Renderer   =RendererImplT::GetInstance();
        RenderMaterialT*     RM         =Renderer.GetCurrentRenderMaterial();
        const MaterialT&     Material   =*(RM->Material);
        OpenGLStateT*        OpenGLState=OpenGLStateT::GetInstance();

        const float*         LightPos         =Renderer.GetCurrentLightSourcePosition();
        float                LightRadius      =Renderer.GetCurrentLightSourceRadius();
        const float*         LightDiffuseColor=Renderer.GetCurrentLightSourceDiffuseColor();

        if (InitCounter<Renderer.GetInitCounter())
        {
            Initialize();
            InitCounter=Renderer.GetInitCounter();
        }


        // Render the lit diffuse map.
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
        OpenGLState->Disable(GL_ALPHA_TEST);
        OpenGLState->Enable(GL_BLEND);
        OpenGLState->BlendFunc(GL_ONE, GL_ONE);
        OpenGLState->DepthFunc(GL_EQUAL);   // Using GL_LEQUAL here yields problems with alpha-tested materials...  :(
        OpenGLState->ColorMask(Material.LightMask[0], Material.LightMask[1], Material.LightMask[2], Material.LightMask[3]);
        OpenGLState->DepthMask(Material.LightMask[4]);
     // OpenGLState->EnableOrDisable(GL_STENCIL_TEST);      // This is INTENTIONALLY not decided here! We take whatever was set by previous shaders.
        if (cf::GL_EXT_stencil_two_side_AVAIL)
        {
            OpenGLState->Disable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            OpenGLState->ActiveStencilFace(GL_FRONT);
        }
        OpenGLState->StencilFunc(GL_EQUAL, 0, ~0);
     // OpenGLState->StencilOp(GL_KEEP, GL_KEEP, GL_INCR);  // Uh, this works only if only ONE rendering pass follows. We often have TWO passes (diff+spec), though...
        OpenGLState->StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        OpenGLState->ActiveTextureUnit(GL_TEXTURE0_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->DiffTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE2_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->NormTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE3_ARB);
        OpenGLState->Enable(GL_TEXTURE_CUBE_MAP_ARB);
        OpenGLState->BindTexture(GL_TEXTURE_CUBE_MAP_ARB, Renderer.GetNormalizationCubeMap()->GetOpenGLObject());


        // The cgGLSetParameter*() functions are very slow, so cache their parameters in order to call them as little as possible.
        static float LightPosCache[3]={ 0.0, 0.0, 0.0 };
        if (LightPos[0]!=LightPosCache[0] || LightPos[1]!=LightPosCache[1] || LightPos[2]!=LightPosCache[2])
        {
            cgGLSetParameter3fv(VS_Diff_LightPos, LightPos);
            LightPosCache[0]=LightPos[0];
            LightPosCache[1]=LightPos[1];
            LightPosCache[2]=LightPos[2];
        }

        static float LightRadiusCache=0.0;
        if (LightRadius!=LightRadiusCache)
        {
            cgGLSetParameter1f (VS_Diff_LightRadius, LightRadius);
            LightRadiusCache=LightRadius;
        }

        const unsigned long NewAge=Renderer.GetDepRelMatrixModelView().Age+Renderer.GetDepRelMatrix(RendererI::PROJECTION).Age;
        if (MatrixChangeCounter!=NewAge)
        {
            cgGLSetStateMatrixParameter(VS_Diff_ModelViewProjMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
            MatrixChangeCounter=NewAge;
        }

        static float LightDiffColorCache[3]={ -1.0, -1.0, -1.0 };
        if (LightDiffuseColor[0]!=LightDiffColorCache[0] || LightDiffuseColor[1]!=LightDiffColorCache[1] || LightDiffuseColor[2]!=LightDiffColorCache[2])
        {
            cgGLSetParameter3fv(FS_Diff_LightColor, LightDiffuseColor);
            LightDiffColorCache[0]=LightDiffuseColor[0];
            LightDiffColorCache[1]=LightDiffuseColor[1];
            LightDiffColorCache[2]=LightDiffuseColor[2];
        }


        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                glNormal3fv(Mesh.Vertices[VertexNr].Normal);                                // Normal
                cf::glMultiTexCoord3fvARB(GL_TEXTURE6_ARB, Mesh.Vertices[VertexNr].Tangent);    // Tangent
                cf::glMultiTexCoord3fvARB(GL_TEXTURE7_ARB, Mesh.Vertices[VertexNr].BiNormal);   // BiNormal

                cf::glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, Mesh.Vertices[VertexNr].TextureCoord);
                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_L_Diff_Norm_oSpec MyShader;
