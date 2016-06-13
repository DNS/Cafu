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


class Shader_A_Terrain : public ShaderT
{
    private:

    GLuint        VertexProgram;
    GLuint        FragmentProgram;

    unsigned long InitCounter;


    void Initialize()
    {
        VertexProgram=UploadProgram(GL_VERTEX_PROGRAM_ARB,
            "!!ARBvp1.0                                                 \n"
            "PARAM mvp[4]={ state.matrix.mvp };                         \n"
            "TEMP MyTexCoord;                                           \n"

            "DP4 result.position.x, vertex.position, mvp[0];            \n"     // result.position = mul(state.matrix.mvp, vertex.position);
            "DP4 result.position.y, vertex.position, mvp[1];            \n"
            "DP4 result.position.z, vertex.position, mvp[2];            \n"
            "DP4 result.position.w, vertex.position, mvp[3];            \n"

            "DP4 MyTexCoord.x, vertex.position, program.local[0];       \n"
            "DP4 MyTexCoord.y, vertex.position, program.local[1];       \n"

            "MOV result.color,       vertex.color;                      \n"     // result.color       = vertex.color;
            "MOV result.texcoord[0], MyTexCoord;                        \n"     // result.texcoord[0] = MyTexCoord;         // diffuse-map
            "MOV result.texcoord[1], MyTexCoord;                        \n"     // result.texcoord[1] = MyTexCoord;         // light-map
            "MUL result.texcoord[2], MyTexCoord, program.local[2].x;    \n"     // result.texcoord[2] = MyTexCoord*scale;   // detail-map
            "END                                                        \n");


        FragmentProgram=UploadProgram(GL_FRAGMENT_PROGRAM_ARB,
            "!!ARBfp1.0                                             \n"
            "OPTION ARB_precision_hint_fastest;                     \n"

            "TEMP diff_col;                                         \n"
            "TEMP lima_col;                                         \n"
            "TEMP dema_col;                                         \n"

            "TEX diff_col, fragment.texcoord[0], texture[0], 2D;    \n"
            "TEX lima_col, fragment.texcoord[1], texture[1], 2D;    \n"
            "TEX dema_col, fragment.texcoord[2], texture[2], 2D;    \n"

            "MUL diff_col,     diff_col, lima_col;                  \n"     // diff_col *= lima_col;    // This assumes that the alpha-component of lima_col is 1.0.
            "MUL diff_col.xyz, diff_col, dema_col;                  \n"     // diff_col *= dema_col;    // Apply dema-col, but not for the alpha-channel!
            "MUL diff_col.xyz, diff_col, 2.0;                       \n"     // diff_col *= 2.0;         // Fix the scale, but leave the alpha-channel alone.
            "MUL result.color, diff_col, fragment.color;            \n"     // result.color = diff_col * fragment.color;
            "END                                                    \n");
    }


    public:

    Shader_A_Terrain()
    {
        VertexProgram  =0;
        FragmentProgram=0;

        InitCounter=0;
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

        cf::glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 0, p1);
        cf::glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, 1, p2);
        cf::glProgramLocalParameter4fARB (GL_VERTEX_PROGRAM_ARB, 2, Material.ShaderParamExpr.Size()>=9 ? Material.ShaderParamExpr[8].Evaluate(Sym).GetAsFloat() : 4.0f, 0.0f, 0.0f, 0.0f);


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
