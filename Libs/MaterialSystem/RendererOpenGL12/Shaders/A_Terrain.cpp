/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************/
/*** Shader ***/
/**************/

#ifdef _MSC_VER
    #include <windows.h>
#endif
#include <GL/gl.h>

#include "../../Common/OpenGLState.hpp"
#include "../RendererImpl.hpp"
#include "../RenderMaterial.hpp"
#include "../Shader.hpp"
#include "../TextureMapImpl.hpp"
#include "../../Mesh.hpp"
#include "../../Common/OpenGLEx.hpp"


using namespace MatSys;


class Shader_A_Terrain : public ShaderT
{
    public:

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


        if (Material.ShaderParamExpr.Size()<8) return;
        const float PlaneS[4]={ Material.ShaderParamExpr[0].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[1].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[2].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[3].Evaluate(Sym).GetAsFloat() };
        const float PlaneT[4]={ Material.ShaderParamExpr[4].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[5].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[6].Evaluate(Sym).GetAsFloat(), Material.ShaderParamExpr[7].Evaluate(Sym).GetAsFloat() };


        OpenGLState->ActiveTextureUnit(GL_TEXTURE0_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->DiffTexMap->GetOpenGLObject());
        OpenGLState->TexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  // Modulate with the OpenGL color.

        // Setup automatic tex-coord generation for tex-unit 0.
        OpenGLState->Enable(GL_TEXTURE_GEN_S); glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); glTexGenfv(GL_S, GL_OBJECT_PLANE, PlaneS);
        OpenGLState->Enable(GL_TEXTURE_GEN_T); glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); glTexGenfv(GL_T, GL_OBJECT_PLANE, PlaneT);


        OpenGLState->ActiveTextureUnit(GL_TEXTURE1_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->UseDefaultLightMap && Renderer.GetCurrentLightMap()!=NULL ? Renderer.GetCurrentLightMap()->GetOpenGLObject() : RM->LightTexMap->GetOpenGLObject());
        OpenGLState->TexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  // Wenn hier GL_REPLACE steht, sehen wir dann die LM alleine!

        // Setup automatic tex-coord generation for tex-unit 1.
        OpenGLState->Enable(GL_TEXTURE_GEN_S); glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); glTexGenfv(GL_S, GL_OBJECT_PLANE, PlaneS);
        OpenGLState->Enable(GL_TEXTURE_GEN_T); glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); glTexGenfv(GL_T, GL_OBJECT_PLANE, PlaneT);


        glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();


        // Turn automatic tex-coord generation off again (for tex-unit 1).
        OpenGLState->Disable(GL_TEXTURE_GEN_S);
        OpenGLState->Disable(GL_TEXTURE_GEN_T);

        OpenGLState->Disable(GL_TEXTURE_2D);
        OpenGLState->ActiveTextureUnit(GL_TEXTURE0_ARB);


        // Optionally add the detail map.
        if (!Material.LumaMapComp.IsEmpty() && Material.ShaderParamExpr.Size()>=9)
        {
            const float Scale=Material.ShaderParamExpr[8].Evaluate(Sym).GetAsFloat();
            const float DetailPlaneS[4]={ PlaneS[0]*Scale, PlaneS[1]*Scale, PlaneS[2]*Scale, PlaneS[3]*Scale };
            const float DetailPlaneT[4]={ PlaneT[0]*Scale, PlaneT[1]*Scale, PlaneT[2]*Scale, PlaneT[3]*Scale };

            glTexGenfv(GL_S, GL_OBJECT_PLANE, DetailPlaneS);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, DetailPlaneT);


            // Adding a detail-map cannot be achieved by multi-texturing, as there
            // is *no* GL_MODULATE_MUL_2 texture environment in unextended OpenGL 1.2.
            // So we are forced to render a second pass.
            OpenGLState->Disable(GL_ALPHA_TEST);
            OpenGLState->Enable(GL_BLEND);
            OpenGLState->BlendFunc(GL_DST_COLOR, GL_SRC_COLOR);     // See e.g. the Q3A Shader Manual for some notes about this blend function.
            OpenGLState->DepthFunc(GL_LEQUAL);
            OpenGLState->BindTexture(GL_TEXTURE_2D, RM->LumaTexMap->GetOpenGLObject());

            // glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
            glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
                for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
                {
                    glVertex3dv(Mesh.Vertices[VertexNr].Origin);
                }
            glEnd();
        }

        // Turn automatic tex-coord generation off again (for tex-unit 0).
        OpenGLState->Disable(GL_TEXTURE_GEN_S);
        OpenGLState->Disable(GL_TEXTURE_GEN_T);
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_A_Terrain MyShader;
