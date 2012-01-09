/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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


class Shader_A_Terrain : public ShaderT
{
    private:

    CGprogram     VertexShader;
    CGparameter   VS_ModelViewProjMatrix;
    CGparameter   VS_TexCoordGenPlaneS;
    CGparameter   VS_TexCoordGenPlaneT;
    CGparameter   VS_ScaleDetailMap;

    CGprogram     FragmentShader;

    unsigned long InitCounter;
    unsigned long MatrixChangeCounter;


    void Initialize()
    {
        VertexShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_VP20,
            " void main(in float4   InPos            : POSITION,    \n"
            "           in float4   InColor          : COLOR,       \n"
            "          out float4   OutPos           : POSITION,    \n"
            "          out float4   OutColor         : COLOR,       \n"
            "          out float2   OutTexCoord_Diff : TEXCOORD0,   \n"
            "          out float2   OutTexCoord_LiMa : TEXCOORD1,   \n"
            "          out float2   OutTexCoord_DeMa : TEXCOORD2,   \n"
            "      uniform float4x4 ModelViewProjMatrix,            \n"
            "      uniform float4   TexCoordGenPlaneS,              \n"
            "      uniform float4   TexCoordGenPlaneT,              \n"
            "      uniform float    ScaleDetailMap)                 \n"
            " {                                                     \n"
            "     const float s=dot(TexCoordGenPlaneS, InPos);      \n"
            "     const float t=dot(TexCoordGenPlaneT, InPos);      \n"
            "                                                       \n"
            "     OutPos          =mul(ModelViewProjMatrix, InPos); \n"
            "     OutColor        =InColor;                         \n"
            "     OutTexCoord_Diff=float2(s, t);                    \n"
            "     OutTexCoord_LiMa=float2(s, t);                    \n"
            "     OutTexCoord_DeMa=float2(s, t)*ScaleDetailMap;     \n"
            " }                                                     \n");

        VS_ModelViewProjMatrix=cgGetNamedParameter(VertexShader, "ModelViewProjMatrix");
        VS_TexCoordGenPlaneS  =cgGetNamedParameter(VertexShader, "TexCoordGenPlaneS");
        VS_TexCoordGenPlaneT  =cgGetNamedParameter(VertexShader, "TexCoordGenPlaneT");
        VS_ScaleDetailMap     =cgGetNamedParameter(VertexShader, "ScaleDetailMap");


        FragmentShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_FP20,
            " void main(in float4    InColor           : COLOR,                 \n"
            "           in float2    InTexCoord_Diff   : TEXCOORD0,             \n"
            "           in float2    InTexCoord_LiMa   : TEXCOORD1,             \n"
            "           in float2    InTexCoord_DeMa   : TEXCOORD2,             \n"
            "          out float4    OutColor          : COLOR,                 \n"
            "      uniform sampler2D DiffuseMapSampler : TEXUNIT0,              \n"
            "      uniform sampler2D LightMapSampler   : TEXUNIT1,              \n"
            "      uniform sampler2D DetailMapSampler  : TEXUNIT2)              \n"
            " {                                                                 \n"
            "     float4 DiffuseC  =tex2D(DiffuseMapSampler, InTexCoord_Diff);  \n"
            "     float4 LightMapC =tex2D(LightMapSampler,   InTexCoord_LiMa);  \n"
            "     float4 DetailMapC=tex2D(DetailMapSampler,  InTexCoord_DeMa);  \n"
            "                                                                   \n"
            "     OutColor=InColor*DiffuseC*LightMapC*DetailMapC*2.0;           \n"
            " }                                                                 \n");
    }


    public:

    Shader_A_Terrain()
    {
        VertexShader          =NULL;
        VS_ModelViewProjMatrix=NULL;

        FragmentShader        =NULL;

        InitCounter=0;
        MatrixChangeCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="A_Terrain";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& Material) const
    {
        if (Material.NoDraw) return 0;

        if ( Material.DiffMapComp .IsEmpty()) return 0;
        if ( Material.LightMapComp.IsEmpty()) return 0;     // Require a LightMap for terrains.
        if (!Material.NormMapComp .IsEmpty()) return 0;
     // if (!Material.LumaMapComp .IsEmpty()) return 0;     // LumaMap (used as DetailMap) is optional.

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
        OpenGLState->BindTexture(GL_TEXTURE_2D, Material.LumaMapComp.IsEmpty() ? Renderer.GetHelperGrayMap()->GetOpenGLObject() : RM->LumaTexMap->GetOpenGLObject());

        if (Material.ShaderParamExpr.Size()<8) return;
        float p1[4]={ Material.ShaderParamExpr[0].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[1].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[2].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[3].Evaluate(Sym).GetAsFloat() };
        float p2[4]={ Material.ShaderParamExpr[4].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[5].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[6].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[7].Evaluate(Sym).GetAsFloat() };

        const unsigned long NewAge=Renderer.GetDepRelMatrixModelView().Age+Renderer.GetDepRelMatrix(RendererI::PROJECTION).Age;
        if (MatrixChangeCounter!=NewAge)
        {
            cgGLSetStateMatrixParameter(VS_ModelViewProjMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
            MatrixChangeCounter=NewAge;
        }
        cgGLSetParameter4fv(VS_TexCoordGenPlaneS, p1);
        cgGLSetParameter4fv(VS_TexCoordGenPlaneT, p2);
        cgGLSetParameter1f (VS_ScaleDetailMap, Material.ShaderParamExpr.Size()>=9 ? Material.ShaderParamExpr[8].Evaluate(Sym).GetAsFloat() : 4.0f);


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
static Shader_A_Terrain MyShader;
