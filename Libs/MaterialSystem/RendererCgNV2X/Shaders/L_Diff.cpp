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


class Shader_L_Diff : public ShaderT
{
    private:

    CGprogram     VertexShader;
    CGparameter   VS_LightPos;
    CGparameter   VS_LightColor;
    CGparameter   VS_LightRadius;
    CGparameter   VS_ModelViewProjMatrix;

    CGprogram     FragmentShader;

    unsigned long InitCounter;
    unsigned long MatrixChangeCounter;


    void Initialize()
    {
        // This VertexShader and FragmentShader were derived from the L_Diff_Norm shaders.
        // Basically, there were two possible courses of action for the conversion:
        // a) Assume the NormalMap is (0, 0, 1) everywhere, and appropriately simplify the code.
        //    This works well, but requires us to provide full tangent space even for this simple shader!
        // b) Get rid of the tangent space. This in turn requires us to keep the lighting per-vertex,
        //    as the NV2X profiles cannot handle such lighting per-pixel as on pp. 124 in "The Cg Tutorial".
        // I've decided for b), as the importance of this shader is minor anyway.
        // I also kept the attenuation computations per-pixel, which may or may not be a good thing.

        VertexShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_VP20,
            // Assumptions:
            //   - The w-component of InPos is 1.0.
            //   - InNormal  has unit length.
            " void main(in float4   InPos           : POSITION,                     \n"
            "           in float3   InNormal        : NORMAL,                       \n"
            "           in float2   InTexCoord      : TEXCOORD0,                    \n"
            "          out float4   OutPos          : POSITION,                     \n"
            "          out float4   OutColor        : COLOR,                        \n"
            "          out float2   OutTexCoord     : TEXCOORD0,                    \n" // For diffuse map.
            "          out float3   OutLightVectorA : TEXCOORD1,                    \n" // "Normalized" (wrt. LightRadius) light vector for attenuation calculations.
            "      uniform float3   LightPos,                                       \n"
            "      uniform float3   LightColor,                                     \n"
            "      uniform float    LightRadius,                                    \n"
            "      uniform float4x4 ModelViewProjMatrix)                            \n"
            " {                                                                     \n"
            "     const float3 LightVector=LightPos-InPos.xyz;                      \n"
            "     const float3 LightDir   =normalize(LightVector);                  \n"
            "     const float  diff       =saturate(dot(LightDir, InNormal));       \n"
            "                                                                       \n"
            "     OutPos     =mul(ModelViewProjMatrix, InPos);                      \n"
            "     OutColor   =float4(diff*LightColor, 0.0);                         \n"
            "     OutTexCoord=InTexCoord;                                           \n"
            "                                                                       \n"
            "     // Light attenuation.                                             \n"
         // "     OutLightAtten=saturate(1.0-length(LightVector)/LightRadius);      \n" // linear, 1-d/r
         // "     OutLightAtten=saturate(2000.0/length(LightVector)-0.01);          \n" // 1/d
            "     // Light vector in model space, normalized wrt. 'LightRadius'.    \n"
            "     // Omitting the range compression (which is NOT really required)  \n"
            "     // seems to cause the Cg compiler to generate bad code in the     \n"
            "     // pixel shader. TODO: Examine the assembly!                      \n"
            "     OutLightVectorA=(LightVector/LightRadius+1.0)/2.0;                \n" // Depending on the fragment shader, this will become 1-d/r or 1-(d/r)^2
            " }                                                                     \n");

        VS_LightPos           =cgGetNamedParameter(VertexShader, "LightPos"           );
        VS_LightColor         =cgGetNamedParameter(VertexShader, "LightColor"         );
        VS_LightRadius        =cgGetNamedParameter(VertexShader, "LightRadius"        );
        VS_ModelViewProjMatrix=cgGetNamedParameter(VertexShader, "ModelViewProjMatrix");


        FragmentShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_FP20,
            " void main(in float4    InColor           : COLOR,                                             \n"
            "           in float2    InTexCoord_diff   : TEXCOORD0,                                         \n"
            "           in float3    InLightVectorA    : TEXCOORD1,                                         \n"
            "          out float4    OutColor          : COLOR,                                             \n"
            "      uniform sampler2D DiffuseMapSampler : TEXUNIT0)                                          \n"
            " {                                                                                             \n"
            "     // Range-uncompress the 'InLightVectorA' (which is normalized wrt. 'LightRadius').        \n"
            "     // [ Omitting the range compression (which is NOT really required) seems to cause the Cg  \n"
            "     //   compiler to generate buggy code here. TODO: Examine the assembly! ]                  \n"
            "     // Then compute the attenuation as 1-(d/r)^2. See L_Diff_Norm.cpp for more details.       \n"
            "     const float3 LightTmp=2.0*(InLightVectorA-0.5);                                           \n"
            "     const float  Atten   =saturate(1.0-dot(LightTmp, LightTmp));                              \n"
            "                                                                                               \n"
            "     OutColor=Atten*InColor*tex2D(DiffuseMapSampler, InTexCoord_diff);                         \n"
            " }                                                                                             \n");
    }


    public:

    Shader_L_Diff()
    {
        VertexShader          =NULL;
        VS_LightPos           =NULL;
        VS_LightColor         =NULL;
        VS_LightRadius        =NULL;
        VS_ModelViewProjMatrix=NULL;

        FragmentShader        =NULL;

        InitCounter=0;
        MatrixChangeCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="L_Diff";

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
        if (!Material.NormMapComp.IsEmpty()) return 0;
        if (!Material.SpecMapComp.IsEmpty()) return 0;

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
        cgGLBindProgram(VertexShader);
        cgGLBindProgram(FragmentShader);
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
        return false;
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

        const float*     LightPos         =Renderer.GetCurrentLightSourcePosition();
        float            LightRadius      =Renderer.GetCurrentLightSourceRadius();
        const float*     LightDiffuseColor=Renderer.GetCurrentLightSourceDiffuseColor();

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


        // The cgGLSetParameter*() functions are very slow, so cache their parameters in order to call them as little as possible.
        static float LightPosCache[3]={ 0.0, 0.0, 0.0 };
        if (LightPos[0]!=LightPosCache[0] || LightPos[1]!=LightPosCache[1] || LightPos[2]!=LightPosCache[2])
        {
            cgGLSetParameter3fv(VS_LightPos, LightPos);
            LightPosCache[0]=LightPos[0];
            LightPosCache[1]=LightPos[1];
            LightPosCache[2]=LightPos[2];
        }

        static float LightRadiusCache=0.0;
        if (LightRadius!=LightRadiusCache)
        {
            cgGLSetParameter1f (VS_LightRadius, LightRadius);
            LightRadiusCache=LightRadius;
        }

        const unsigned long NewAge=Renderer.GetDepRelMatrixModelView().Age+Renderer.GetDepRelMatrix(RendererI::PROJECTION).Age;
        if (MatrixChangeCounter!=NewAge)
        {
            cgGLSetStateMatrixParameter(VS_ModelViewProjMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
            MatrixChangeCounter=NewAge;
        }

        static float LightDiffColorCache[3]={ -1.0, -1.0, -1.0 };
        if (LightDiffuseColor[0]!=LightDiffColorCache[0] || LightDiffuseColor[1]!=LightDiffColorCache[1] || LightDiffuseColor[2]!=LightDiffColorCache[2])
        {
            cgGLSetParameter3fv(VS_LightColor, LightDiffuseColor);
            LightDiffColorCache[0]=LightDiffuseColor[0];
            LightDiffColorCache[1]=LightDiffuseColor[1];
            LightDiffColorCache[2]=LightDiffuseColor[2];
        }


        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                glNormal3fv(Mesh.Vertices[VertexNr].Normal);    // Normal

                cf::glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, Mesh.Vertices[VertexNr].TextureCoord);
                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_L_Diff MyShader;
