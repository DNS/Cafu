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
#include <math.h>
#include <GL/gl.h>

#include "../../Common/OpenGLState.hpp"
#include "../RendererImpl.hpp"
#include "../RenderMaterial.hpp"
#include "../Shader.hpp"
#include "../TextureMapImpl.hpp"
#include "../../Mesh.hpp"
#include "../../Common/OpenGLEx.hpp"


using namespace MatSys;


class Shader_L_Diff_oSpec : public ShaderT
{
    public:

    const std::string& GetName() const
    {
        static const std::string Name="L_Diff_oSpec";

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

        if (Material.DiffMapComp.IsEmpty()) return 0;       // A diffuse map is really required.
        // We cannot handle a NormalMap anyway.
        // SpecularMap is optional.

        return 255;
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
        OpenGLState->TexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                // LightVectorNorm=VectorUnit(LightPos-Mesh.Vertices[VertexNr].Origin);
                const float LightVector[3]    ={ LightPos[0]-float(Mesh.Vertices[VertexNr].Origin[0]), LightPos[1]-float(Mesh.Vertices[VertexNr].Origin[1]), LightPos[2]-float(Mesh.Vertices[VertexNr].Origin[2]) };
                const float LightVectorLen    =sqrt(LightVector[0]*LightVector[0] + LightVector[1]*LightVector[1] + LightVector[2]*LightVector[2]);
                const float LightVectorDiv    =LightVectorLen>0.0 ? 1.0f/LightVectorLen : 0.0f;
                const float LightVectorNorm[3]={ LightVector[0]*LightVectorDiv, LightVector[1]*LightVectorDiv, LightVector[2]*LightVectorDiv };

                const float LdotN             =LightVectorNorm[0]*Mesh.Vertices[VertexNr].Normal[0] + LightVectorNorm[1]*Mesh.Vertices[VertexNr].Normal[1] + LightVectorNorm[2]*Mesh.Vertices[VertexNr].Normal[2];
                const float DiffuseFactor     =LdotN<0.0f ? 0.0f : LdotN;

                const float Atten             =1.0f-LightVectorLen/LightRadius;
                const float Attenuation       =Atten<0.0f ? 0.0f : Atten>1.0f ? 1.0f : Atten;

                glColor3f(Attenuation*DiffuseFactor*LightDiffuseColor[0], Attenuation*DiffuseFactor*LightDiffuseColor[1], Attenuation*DiffuseFactor*LightDiffuseColor[2]);
                glTexCoord2fv(Mesh.Vertices[VertexNr].TextureCoord);
                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();


        // Optionally add the specular contribution.
        if (Material.SpecMapComp.IsEmpty()) return;

        const float* EyePos            =Renderer.GetCurrentEyePosition();
        const float* LightSpecularColor=Renderer.GetCurrentLightSourceSpecularColor();

        OpenGLState->BindTexture(GL_TEXTURE_2D, RM->SpecTexMap->GetOpenGLObject());

        glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
            for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
            {
                // LightVectorNorm=VectorUnit(LightPos-Mesh.Vertices[VertexNr].Origin);
                const float LightVector[3]      ={ LightPos[0]-float(Mesh.Vertices[VertexNr].Origin[0]), LightPos[1]-float(Mesh.Vertices[VertexNr].Origin[1]), LightPos[2]-float(Mesh.Vertices[VertexNr].Origin[2]) };
                const float LightVectorLen      =sqrt(LightVector[0]*LightVector[0] + LightVector[1]*LightVector[1] + LightVector[2]*LightVector[2]);
                const float LightVectorDiv      =LightVectorLen>0.0f ? 1.0f/LightVectorLen : 0.0f;
                const float LightVectorNorm[3]  ={ LightVector[0]*LightVectorDiv, LightVector[1]*LightVectorDiv, LightVector[2]*LightVectorDiv };

                // EyeVectorNorm=VectorUnit(EyePos-Mesh.Vertices[VertexNr].Origin);
                const float EyeVector[3]        ={ EyePos[0]-float(Mesh.Vertices[VertexNr].Origin[0]), EyePos[1]-float(Mesh.Vertices[VertexNr].Origin[1]), EyePos[2]-float(Mesh.Vertices[VertexNr].Origin[2]) };
                const float EyeVectorLen        =sqrt(EyeVector[0]*EyeVector[0] + EyeVector[1]*EyeVector[1] + EyeVector[2]*EyeVector[2]);
                const float EyeVectorDiv        =EyeVectorLen>0.0f ? 1.0f/EyeVectorLen : 0.0f;
                const float EyeVectorNorm[3]    ={ EyeVector[0]*EyeVectorDiv, EyeVector[1]*EyeVectorDiv, EyeVector[2]*EyeVectorDiv };

                // HalfwayVectorNorm=VectorUnit(LightVectorNorm+EyeVectorNorm);
                const float HalfwayVector[3]    ={ LightVectorNorm[0]+EyeVectorNorm[0], LightVectorNorm[1]+EyeVectorNorm[1], LightVectorNorm[2]+EyeVectorNorm[2] };
                const float HalfwayVectorLen    =sqrt(HalfwayVector[0]*HalfwayVector[0] + HalfwayVector[1]*HalfwayVector[1] + HalfwayVector[2]*HalfwayVector[2]);
                const float HalfwayVectorDiv    =HalfwayVectorLen>0.0f ? 1.0f/HalfwayVectorLen : 0.0f;
                const float HalfwayVectorNorm[3]={ HalfwayVector[0]*HalfwayVectorDiv, HalfwayVector[1]*HalfwayVectorDiv, HalfwayVector[2]*HalfwayVectorDiv };

                const float LdotN               =LightVectorNorm  [0]*Mesh.Vertices[VertexNr].Normal[0] + LightVectorNorm  [1]*Mesh.Vertices[VertexNr].Normal[1] + LightVectorNorm  [2]*Mesh.Vertices[VertexNr].Normal[2];
                const float HdotN               =HalfwayVectorNorm[0]*Mesh.Vertices[VertexNr].Normal[0] + HalfwayVectorNorm[1]*Mesh.Vertices[VertexNr].Normal[1] + HalfwayVectorNorm[2]*Mesh.Vertices[VertexNr].Normal[2];
                const float SpecularFactor      =LdotN>0.0f ? pow(HdotN<0.0f ? 0.0f : HdotN, 32.0f) : 0.0f;

                const float Atten               =1.0f-LightVectorLen/LightRadius;
                const float Attenuation         =Atten<0.0f ? 0.0f : Atten>1.0f ? 1.0f : Atten;

                glColor3f(Attenuation*SpecularFactor*LightSpecularColor[0], Attenuation*SpecularFactor*LightSpecularColor[1], Attenuation*SpecularFactor*LightSpecularColor[2]);
                glTexCoord2fv(Mesh.Vertices[VertexNr].TextureCoord);
                glVertex3dv(Mesh.Vertices[VertexNr].Origin);
            }
        glEnd();
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_L_Diff_oSpec MyShader;
