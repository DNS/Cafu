/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

/**************/
/*** Shader ***/
/**************/

// Required for #include <GL/gl.h> with MS VC++.
#if defined(_WIN32) && defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
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


class Shader_L_Diff : public ShaderT
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
            "PARAM LightPos   =program.local[0];                        \n"
            "PARAM LightRadius=program.local[1];                        \n"

            "OUTPUT result_LightVec     =result.texcoord[1];            \n" // Light vector.
            "OUTPUT result_LightVecAtten=result.texcoord[2];            \n" // Light vector for attenuation computations. Normalized wrt. LightRadius.
            "OUTPUT result_NormalVec    =result.texcoord[3];            \n" // Normal vector.

            "DP4 result.position.x, vertex.position, mvp[0];            \n" // result.position = mul(state.matrix.mvp, vertex.position);
            "DP4 result.position.y, vertex.position, mvp[1];            \n"
            "DP4 result.position.z, vertex.position, mvp[2];            \n"
            "DP4 result.position.w, vertex.position, mvp[3];            \n"

            "MOV result.texcoord[0], vertex.texcoord[0];                \n" // result.texcoord[0] = vertex.texcoord[0];
            "MOV result_NormalVec, vertex.normal;                       \n" // result_NormalVec   = vertex.normal;

            "TEMP LightVector;                                          \n"
            "SUB LightVector, LightPos, vertex.position;                \n" // LightVector = LightPos - vertex.position;

            "MOV result_LightVec, LightVector;                          \n" // result_LightVec = LightVector;

            "TEMP OneDivLightRadius;                                    \n"
            "RCP OneDivLightRadius, LightRadius.x;                      \n"
            "MUL result_LightVecAtten, LightVector, OneDivLightRadius;  \n" // result_LightVecAtten = LightVector/LightRadius;
            "END                                                        \n");


        FragmentProgram=UploadProgram(GL_FRAGMENT_PROGRAM_ARB,
            "!!ARBfp1.0                                             \n"
            "OPTION ARB_precision_hint_fastest;                     \n"
            "PARAM  LightColorDiff=program.local[0];                \n"
            "ATTRIB LightVec      =fragment.texcoord[1];            \n"
            "ATTRIB LightVecAtten =fragment.texcoord[2];            \n"
            "ATTRIB NormalVec     =fragment.texcoord[3];            \n"

            "TEMP diff_col;                                         \n"
            "TEX diff_col, fragment.texcoord[0], texture[0], 2D;    \n"

            "TEMP LightVecN;                                        \n"
            "DP3 LightVecN.a, LightVec, LightVec;                   \n"
            "RSQ LightVecN.a, LightVecN.a;                          \n"
            "MUL LightVecN.rgb, LightVec, LightVecN.a;              \n" // LightVecN = normalize(LightVec);

            "TEMP NormalVecN;                                       \n"
            "DP3 NormalVecN.a, NormalVec, NormalVec;                \n"
            "RSQ NormalVecN.a, NormalVecN.a;                        \n"
            "MUL NormalVecN.rgb, NormalVec, NormalVecN.a;           \n" // NormalVecN = normalize(NormalVec);

            "TEMP Coeffs;                                           \n"
            "DP3_SAT Coeffs.x, NormalVecN, LightVecN;               \n" // Coeffs.x = saturate(N dot L);
            "DP3     Coeffs.y, LightVecAtten, LightVecAtten;        \n"
            "SUB_SAT Coeffs.y, 1.0, Coeffs.y;                       \n" // Coeffs.y = saturate(1.0 - (La dot La));

            "MUL diff_col,     diff_col, Coeffs.x;                  \n"
            "MUL diff_col,     diff_col, Coeffs.y;                  \n"
            "MUL result.color, diff_col, LightColorDiff;            \n"
            "END                                                    \n");
    }


    public:

    Shader_L_Diff()
    {
        VertexProgram  =0;
        FragmentProgram=0;

        InitCounter=0;
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
        static float LightPosCache[4]={ 0.0, 0.0, 0.0, 1.0 };
        if (LightPos[0]!=LightPosCache[0] || LightPos[1]!=LightPosCache[1] || LightPos[2]!=LightPosCache[2])
        {
            LightPosCache[0]=LightPos[0];
            LightPosCache[1]=LightPos[1];
            LightPosCache[2]=LightPos[2];
            cf::glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 0, LightPosCache);
        }

        static float LightRadiusCache[4]={ 0.0, 0.0, 0.0, 0.0 };
        if (LightRadius!=LightRadiusCache[0])
        {
            LightRadiusCache[0]=LightRadius;
            cf::glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 1, LightRadiusCache);
        }

        static float LightDiffColorCache[4]={ -1.0, -1.0, -1.0, 0.0 };
        if (LightDiffuseColor[0]!=LightDiffColorCache[0] || LightDiffuseColor[1]!=LightDiffColorCache[1] || LightDiffuseColor[2]!=LightDiffColorCache[2])
        {
            LightDiffColorCache[0]=LightDiffuseColor[0];
            LightDiffColorCache[1]=LightDiffuseColor[1];
            LightDiffColorCache[2]=LightDiffuseColor[2];
            cf::glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, LightDiffColorCache);
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
