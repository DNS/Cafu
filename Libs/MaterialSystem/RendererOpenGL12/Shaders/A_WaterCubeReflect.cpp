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


class Shader_A_WaterWithCubeReflection : public ShaderT
{
    public:

    const std::string& GetName() const
    {
        static const std::string Name="A_WaterCubeReflect";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& Material) const
    {
        if (Material.NoDraw) return 0;
        if (Material.DiffMapComp .IsEmpty()) return 0;      // A diffuse map is really required.
     // if (Material.NormMapComp .IsEmpty()) return 0;      // We don't use it (but other Renderers do for their A_WaterCubeReflect implementation), so don't require it.
     // if (Material.CubeMap1Comp.IsEmpty()) return 0;      // We don't use it (but other Renderers do for their A_WaterCubeReflect implementation), so don't require it.

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

        OpenGLState->ActiveTextureUnit(GL_TEXTURE0_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->DiffTexMap->GetOpenGLObject());
        OpenGLState->TexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  // Modulate with the OpenGL color.

        const float CurrentTime=(Material.ShaderParamExpr.Size()>1) ? Material.ShaderParamExpr[1].Evaluate(Sym).GetAsFloat() : 0.0f;

        glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                glTexCoord2f(Mesh.Vertices[VertexNr].TextureCoord[0]+0.6f*CurrentTime, Mesh.Vertices[VertexNr].TextureCoord[1]+0.8f*CurrentTime);
                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_A_WaterWithCubeReflection MyShader;
