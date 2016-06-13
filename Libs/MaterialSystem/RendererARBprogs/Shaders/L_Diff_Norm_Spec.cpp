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


class Shader_L_Diff_Norm_Spec : public ShaderT
{
    private:

    GLuint        VertexProgram;
    GLuint        FragmentProgram;

    unsigned long InitCounter;


    void Initialize()
    {
        VertexProgram=UploadProgram(GL_VERTEX_PROGRAM_ARB,
            "!!ARBvp1.0                                                 \n"
            "PARAM mvp[4]     ={ state.matrix.mvp };                    \n"
            "PARAM EyePos     =program.local[0];                        \n"
            "PARAM LightPos   =program.local[1];                        \n"
            "PARAM LightRadius=program.local[2];                        \n"

            "OUTPUT result_LightVec     =result.texcoord[1];            \n" // Light vector, rotated from model space into tangent space.
            "OUTPUT result_LightVecAtten=result.texcoord[2];            \n" // Light vector for attenuation computations. Kept in model space, but normalized wrt. LightRadius.
            "OUTPUT result_HalfwayVec   =result.texcoord[3];            \n" // Halfway vector, rotated from model space into tangent space.

            "DP4 result.position.x, vertex.position, mvp[0];            \n" // result.position = mul(state.matrix.mvp, vertex.position);
            "DP4 result.position.y, vertex.position, mvp[1];            \n"
            "DP4 result.position.z, vertex.position, mvp[2];            \n"
            "DP4 result.position.w, vertex.position, mvp[3];            \n"

            "MOV result.texcoord[0], vertex.texcoord[0];                \n" // result.texcoord[0] = vertex.texcoord[0];

            "TEMP LightVector;                                          \n"
            "SUB LightVector, LightPos, vertex.position;                \n" // LightVector = LightPos - vertex.position;
            "DP3 LightVector.w, LightVector, LightVector;               \n" // LightVector.w = |LightVector|^2  (= LightVector dot LightVector)

            "DP3 result_LightVec.x, vertex.texcoord[6], LightVector;    \n" // result_LightVec = mul(TangentSpaceMatrix, LightVector);
            "DP3 result_LightVec.y, vertex.texcoord[7], LightVector;    \n"
            "DP3 result_LightVec.z, vertex.normal,      LightVector;    \n"

            "TEMP OneDivLightRadius;                                    \n"
            "RCP OneDivLightRadius, LightRadius.x;                      \n"
            "MUL result_LightVecAtten, LightVector, OneDivLightRadius;  \n" // result_LightVecAtten = LightVector/LightRadius;


            "ALIAS EyeVector=OneDivLightRadius;                         \n"
            "SUB EyeVector, EyePos, vertex.position;                    \n" // EyeVector = EyePos - vertex.position;
            "DP3 EyeVector.w, EyeVector, EyeVector;                     \n" // EyeVector.w = |EyeVector|^2  (= EyeVector dot EyeVector)

            // Normally, we would write something like
            //    RSQ LightVector.w, LightVector.w;
            //    MUL LightVector.xyz, LightVector, LightVector.w;
            //    RSQ EyeVector.w, EyeVector.w;
            //    MAD EyeVector.xyz, EyeVector, EyeVector.w, LightVector;
            // but that makes the HalfwayVector, when interpolated across the polygon,
            // become so short that the normalization cube map look-up returns undefined results!
            // Therefore, multiply the entire vector with LightVecLen.
            "RCP LightVector.w, LightVector.w;                          \n"
            "MUL EyeVector.w, EyeVector.w, LightVector.w;               \n"
            "RSQ EyeVector.w, EyeVector.w;                              \n"
            "MAD EyeVector.xyz, EyeVector, EyeVector.w, LightVector;    \n"

            "DP3 result_HalfwayVec.x, vertex.texcoord[6], EyeVector;    \n" // result_HalfwayVec = mul(TangentSpaceMatrix, HalfwayVector);
            "DP3 result_HalfwayVec.y, vertex.texcoord[7], EyeVector;    \n"
            "DP3 result_HalfwayVec.z, vertex.normal,      EyeVector;    \n"
            "END                                                        \n");


        FragmentProgram=UploadProgram(GL_FRAGMENT_PROGRAM_ARB,
            "!!ARBfp1.0                                             \n"
            "OPTION ARB_precision_hint_fastest;                     \n"
            "PARAM  LightColorDiff=program.local[0];                \n"
            "PARAM  LightColorSpec=program.local[1];                \n"
            "ATTRIB LightVec      =fragment.texcoord[1];            \n"
            "ATTRIB LightVecAtten =fragment.texcoord[2];            \n"
            "ATTRIB HalfwayVec    =fragment.texcoord[3];            \n"

            "TEMP diff_col;                                         \n"
            "TEMP spec_col;                                         \n"
            "TEMP norm_col;                                         \n"

            "KIL LightVec.z;                                        \n" // If lighting from behind the triangle, we're done immediately.

            "TEX diff_col, fragment.texcoord[0], texture[0], 2D;    \n"
            "TEX spec_col, fragment.texcoord[0], texture[1], 2D;    \n"
            "TEX norm_col, fragment.texcoord[0], texture[2], 2D;    \n"

            "MAD norm_col, norm_col, 2.0, -1.0;                     \n" // norm_col = norm_col*2-1;     // range-uncompress

            "TEMP LightVecN;                                        \n"
            "DP3 LightVecN.a, LightVec, LightVec;                   \n"
            "RSQ LightVecN.a, LightVecN.a;                          \n"
            "MUL LightVecN.rgb, LightVec, LightVecN.a;              \n" // LightVecN = normalize(LightVec);

            "TEMP HalfwayVecN;                                      \n"
            "DP3 HalfwayVecN.a, HalfwayVec, HalfwayVec;             \n"
            "RSQ HalfwayVecN.a, HalfwayVecN.a;                      \n"
            "MUL HalfwayVecN.rgb, HalfwayVec, HalfwayVecN.a;        \n" // HalfwayVecN = normalize(HalfwayVec);

            "TEMP Coeffs;                                           \n"
            "DP3_SAT Coeffs.x, norm_col, LightVecN;                 \n" // Saturate here, because 1.01^32 == 1.37...
            "DP3_SAT Coeffs.y, norm_col, HalfwayVecN;               \n"
            "MOV Coeffs.w, 32.0;                                    \n"
            "LIT Coeffs, Coeffs;                                    \n"

            "MUL diff_col, diff_col, Coeffs.y;                      \n"
            "MUL diff_col, diff_col, LightColorDiff;                \n"

            "MUL spec_col, spec_col, Coeffs.z;                      \n"
            "MAD spec_col, spec_col, LightColorSpec, diff_col;      \n"

            "ALIAS Atten=Coeffs;                                    \n"
            "DP3 Atten, LightVecAtten, LightVecAtten;               \n"
            "SUB_SAT Atten, 1.0, Atten;                             \n"

            "MUL result.color, spec_col, Atten;                     \n"
            "END                                                    \n");
    }


    public:

    Shader_L_Diff_Norm_Spec()
    {
        VertexProgram  =0;
        FragmentProgram=0;

        InitCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="L_Diff_Norm_Spec";

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

        if (Material.DiffMapComp.IsEmpty()) return 0;
        if (Material.NormMapComp.IsEmpty()) return 0;
        if (Material.SpecMapComp.IsEmpty()) return 0;

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

        cf::glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VertexProgram);
        cf::glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FragmentProgram);
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

        const float*         EyePos            =Renderer.GetCurrentEyePosition();
        const float*         LightPos          =Renderer.GetCurrentLightSourcePosition();
        float                LightRadius       =Renderer.GetCurrentLightSourceRadius();
        const float*         LightDiffuseColor =Renderer.GetCurrentLightSourceDiffuseColor();
        const float*         LightSpecularColor=Renderer.GetCurrentLightSourceSpecularColor();

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

        OpenGLState->ActiveTextureUnit(GL_TEXTURE1_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->SpecTexMap->GetOpenGLObject());

        OpenGLState->ActiveTextureUnit(GL_TEXTURE2_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->NormTexMap->GetOpenGLObject());


        // The cgGLSetParameter*() functions are very slow, so cache their parameters in order to call them as little as possible.
        static float EyePosCache[4]={ 0.0, 0.0, 0.0, 1.0 };
        if (EyePos[0]!=EyePosCache[0] || EyePos[1]!=EyePosCache[1] || EyePos[2]!=EyePosCache[2])
        {
            EyePosCache[0]=EyePos[0];
            EyePosCache[1]=EyePos[1];
            EyePosCache[2]=EyePos[2];
            cf::glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 0, EyePosCache);
        }

        static float LightPosCache[4]={ 0.0, 0.0, 0.0, 1.0 };
        if (LightPos[0]!=LightPosCache[0] || LightPos[1]!=LightPosCache[1] || LightPos[2]!=LightPosCache[2])
        {
            LightPosCache[0]=LightPos[0];
            LightPosCache[1]=LightPos[1];
            LightPosCache[2]=LightPos[2];
            cf::glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 1, LightPosCache);
        }

        static float LightRadiusCache[4]={ 0.0, 0.0, 0.0, 0.0 };
        if (LightRadius!=LightRadiusCache[0])
        {
            LightRadiusCache[0]=LightRadius;
            cf::glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 2, LightRadiusCache);
        }

        static float LightDiffColorCache[4]={ -1.0, -1.0, -1.0, 0.0 };
        if (LightDiffuseColor[0]!=LightDiffColorCache[0] || LightDiffuseColor[1]!=LightDiffColorCache[1] || LightDiffuseColor[2]!=LightDiffColorCache[2])
        {
            LightDiffColorCache[0]=LightDiffuseColor[0];
            LightDiffColorCache[1]=LightDiffuseColor[1];
            LightDiffColorCache[2]=LightDiffuseColor[2];
            cf::glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, LightDiffColorCache);
        }

        static float LightSpecColorCache[4]={ -1.0, -1.0, -1.0, 0.0 };
        if (LightSpecularColor[0]!=LightSpecColorCache[0] || LightSpecularColor[1]!=LightSpecColorCache[1] || LightSpecularColor[2]!=LightSpecColorCache[2])
        {
            LightSpecColorCache[0]=LightSpecularColor[0];
            LightSpecColorCache[1]=LightSpecularColor[1];
            LightSpecColorCache[2]=LightSpecularColor[2];
            cf::glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, LightSpecColorCache);
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
static Shader_L_Diff_Norm_Spec MyShader;
