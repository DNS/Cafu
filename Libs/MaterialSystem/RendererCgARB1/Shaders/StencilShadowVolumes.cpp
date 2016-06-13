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


class Shader_StencilShadowVolumes : public ShaderT
{
    private:

    CGprogram     VertexShader;
    CGprogram     FragmentShader;

    unsigned long InitCounter;


    void Initialize()
    {
        VertexShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_ARBVP1,
            " void main(in float4   InPos  : POSITION,      \n"
            "          out float4   OutPos : POSITION)      \n"
            " {                                             \n"
            "     OutPos=mul(glstate.matrix.mvp, InPos);    \n"
            " }                                             \n");


        FragmentShader=UploadCgProgram(RendererImplT::GetInstance().GetCgContext(), CG_PROFILE_ARBFP1,
            " void main(out half4 OutColor : COLOR)     \n"
            " {                                         \n"
            "     OutColor=0;                           \n" // This line cannot be omitted. Cg programs may not be empty.
            " }                                         \n");
    }


    public:

    Shader_StencilShadowVolumes()
    {
        VertexShader  =NULL;
        FragmentShader=NULL;

        InitCounter=0;
    }

    const std::string& GetName() const
    {
        static const std::string Name="StencilShadowVolumes";

        return Name;
    }

    char CanHandleAmbient(const MaterialT& /*Material*/) const
    {
        return 0;
    }

    char CanHandleLighting(const MaterialT& /*Material*/) const
    {
        return 0;
    }

    bool CanHandleStencilShadowVolumes() const
    {
        return true;
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
        if (!cf::GL_EXT_stencil_wrap_AVAIL) return;

        const RendererImplT& Renderer   =RendererImplT::GetInstance();
        OpenGLStateT*        OpenGLState=OpenGLStateT::GetInstance();

        if (InitCounter<Renderer.GetInitCounter())
        {
            Initialize();
            InitCounter=Renderer.GetInitCounter();
        }


        OpenGLState->PolygonMode(GL_FILL);
        OpenGLState->Disable(GL_POLYGON_OFFSET_FILL);
        OpenGLState->Disable(GL_ALPHA_TEST);
        OpenGLState->Disable(GL_BLEND);
        // Keep the depth test func at GL_LESS (rather than at GL_LEQUAL), or else it is impossible to use the front-facing polygons
        // of a shadow-caster as front-caps of the shadow volume. With GL_EQUAL, such front-caps would pass the z-test where the lit
        // polygons of the shadow-caster are, and thus put them wrongly into shadows!
        OpenGLState->DepthFunc(GL_LESS);
        OpenGLState->ActiveTextureUnit(GL_TEXTURE0_ARB);
        OpenGLState->Disable(GL_TEXTURE_2D);
        OpenGLState->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        OpenGLState->DepthMask(GL_FALSE);
        OpenGLState->Enable(GL_STENCIL_TEST);
        OpenGLState->FrontFace(OpenGLStateT::WindingToOpenGL[Mesh.Winding]);


        if (cf::GL_EXT_stencil_two_side_AVAIL)
        {
            OpenGLState->Disable(GL_CULL_FACE);
            OpenGLState->Enable(GL_STENCIL_TEST_TWO_SIDE_EXT);

            OpenGLState->ActiveStencilFace(GL_FRONT);
            OpenGLState->StencilOp(GL_KEEP, GL_INCR_WRAP, GL_KEEP);
            OpenGLState->StencilFunc(GL_ALWAYS, 0, ~0);
         // OpenGLState->StencilMask(~0);       // This is the default setting anyway.

            OpenGLState->ActiveStencilFace(GL_BACK);
            OpenGLState->StencilOp(GL_KEEP, GL_DECR_WRAP, GL_KEEP);
            OpenGLState->StencilFunc(GL_ALWAYS, 0, ~0);
         // OpenGLState->StencilMask(~0);       // This is the default setting anyway.

            glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
                for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
                    glVertex4dv(Mesh.Vertices[VertexNr].Origin);
            glEnd();
        }
        else
        {
            OpenGLState->Enable(GL_CULL_FACE);
            OpenGLState->StencilFunc(GL_ALWAYS, 0, ~0);
         // OpenGLState->StencilMask(~0);       // This is the default setting anyway.

            // Here, we render a single piece of shadow silhouette in both passes.
            // We could also render ALL pieces first in the first pass, then all in the second,
            // taking the OpenGL state changes out of the inner loop.
            // The good news is that that discussion is obsoleted by the two-sided stencil test extension above.
            for (char PassNr=0; PassNr<2; PassNr++)
            {
                // Note that we have to cull the following polygons wrt. the *VIEWER* (not the light source)!
                OpenGLState->CullFace(PassNr==0 ? GL_FRONT : GL_BACK);

                // There are two methods to get away without the stencil wrapping extension:
                // 1. Move the loop over the two passes outward.
                //    That is, do first cull ALL front-facing polygons (only INCR operations), then do all DECR operations.
                // 2. Clear the stencil buffer to 128, and compare later to reference value 128.
                OpenGLState->StencilOp(GL_KEEP, PassNr==0 ? GL_INCR_WRAP : GL_DECR_WRAP, GL_KEEP);

                glBegin(OpenGLStateT::MeshToOpenGLType[Mesh.Type]);
                    for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
                        glVertex4dv(Mesh.Vertices[VertexNr].Origin);
                glEnd();
            }
        }
    }
};


// The constructor of the base class automatically registers this shader with the ShaderRepository.
static Shader_StencilShadowVolumes MyShader;
