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


class Shader_A_Diff_Light_oLuma : public ShaderT
{
    public:

    const std::string& GetName() const
    {
        static const std::string Name="A_Diff_Light_oLuma";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& Material) const
    {
        if (Material.NoDraw) return 0;

        if (Material.DiffMapComp .IsEmpty()) return 0;      // A diffuse map is really required.
        if (Material.LightMapComp.IsEmpty()) return 0;
        // LumaMap is optional.
        if (!Material.NormMapComp.IsEmpty()) return 201;    // Somebody else might be better.

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

        OpenGLState->ActiveTextureUnit(GL_TEXTURE1_ARB);
        OpenGLState->Enable(GL_TEXTURE_2D);
        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->UseDefaultLightMap && Renderer.GetCurrentLightMap()!=NULL ? Renderer.GetCurrentLightMap()->GetOpenGLObject() : RM->LightTexMap->GetOpenGLObject());
        OpenGLState->TexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  // Wenn hier GL_REPLACE steht, sehen wir dann die LM alleine!

        // Only color modulate here if there is no luma-map.
        if (Material.LumaMapComp.IsEmpty()) glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
                                           else glColor4ub(255, 255, 255, 255);

        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                cf::glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, Mesh.Vertices[VertexNr].TextureCoord);
                cf::glMultiTexCoord2fvARB(GL_TEXTURE1_ARB, Mesh.Vertices[VertexNr].LightMapCoord);

                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();


        OpenGLState->Disable(GL_TEXTURE_2D);
        OpenGLState->ActiveTextureUnit(GL_TEXTURE0_ARB);


        // Optionally add the luminance map.
        if (!Material.LumaMapComp.IsEmpty())
        {
            // Adding a luma-map cannot be achieved by multi-texturing, as there
            // is *no* GL_ADD texture environment in unextended OpenGL 1.2.
            // So we are forced to render a second pass.
            OpenGLState->Disable(GL_ALPHA_TEST);
            OpenGLState->Enable(GL_BLEND);
            OpenGLState->BlendFunc(GL_ONE, GL_ONE);
            OpenGLState->DepthFunc(GL_LEQUAL);
            OpenGLState->BindTexture(GL_TEXTURE_2D, RM->LumaTexMap->GetOpenGLObject());

            glColor4f(RedValue, GreenValue, BlueValue, AlphaValue);
            glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
                for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
                {
                    glTexCoord2fv(Mesh.Vertices[VertexNr].TextureCoord);
                    glVertex3dv(Mesh.Vertices[VertexNr].Origin);
                }
            glEnd();
        }
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_A_Diff_Light_oLuma MyShader;
