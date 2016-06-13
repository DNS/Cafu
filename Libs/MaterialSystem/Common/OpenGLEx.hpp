/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************************/
/*** OpenGL Extensions (Header) ***/
/**********************************/

#ifndef CAFU_OPENGL_EX_HPP_INCLUDED
#define CAFU_OPENGL_EX_HPP_INCLUDED

#if defined(_WIN32)
    // #Including gl.h requires windows.h on Win32.
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    // On Linux (as on all all other platforms), we don't want to auto-include the systems <GL/glext.h> file.
    #define GL_GLEXT_LEGACY
#endif

#include <GL/gl.h>
#include "glext.h"  // Use the local version of glext.h, not the system one. The latest version is found at http://oss.sgi.com/projects/ogl-sample/ABI/.


// This namespace is very important because it solves two difficult problems:
// 1) First, depending on the versions of the #included gl.h and glext.h files, some of the function pointer
//    declarations may already be provided by those headers (especially newer versions e.g. for OpenGL 1.3
//    come e.g. with multi-texturing functions predeclared), and thus clash with our private declarations.
// 2) Subtle linking problems that eventually are also results of clashes can occur under Linux,
//    see http://groups.google.de/group/gnu.g++.help/browse_thread/thread/94674a10902646a5 for details.
namespace cf
{
    // Availability of extensions.
    extern bool GL_ARB_multitexture_AVAIL;
    extern bool GL_ARB_texture_cube_map_AVAIL;
    extern bool GL_ARB_texture_compression_AVAIL;
    extern bool GL_ATI_Radeon8500Shaders_AVAIL;
    extern bool GL_EXT_stencil_wrap_AVAIL;
    extern bool GL_EXT_stencil_two_side_AVAIL;
    extern bool GL_ARB_vertex_and_fragment_program_AVAIL;

    // Initialize extensions.
    void Init_GL_ARB_multitexture();
    void Init_GL_ARB_texture_cube_map();
    void Init_GL_ARB_texture_compression();
    void Init_ATI_Radeon8500Shaders();
    void Init_GL_EXT_stencil_wrap();
    void Init_GL_EXT_stencil_two_side();
    void Init_GL_ARB_vertex_and_fragment_program();


    // Pointers to extension functions.
    extern PFNGLMULTITEXCOORD2DARBPROC           glMultiTexCoord2dARB;      // Extension "GL_ARB_multitexture".
    extern PFNGLMULTITEXCOORD2FARBPROC           glMultiTexCoord2fARB;
    extern PFNGLMULTITEXCOORD2FVARBPROC          glMultiTexCoord2fvARB;
    extern PFNGLMULTITEXCOORD3FARBPROC           glMultiTexCoord3fARB;
    extern PFNGLMULTITEXCOORD3FVARBPROC          glMultiTexCoord3fvARB;
    extern PFNGLMULTITEXCOORD4FARBPROC           glMultiTexCoord4fARB;
    extern PFNGLMULTITEXCOORD4FVARBPROC          glMultiTexCoord4fvARB;
    extern PFNGLACTIVETEXTUREARBPROC             glActiveTextureARB;
    extern PFNGLCLIENTACTIVETEXTUREARBPROC       glClientActiveTextureARB;

    extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC      glCompressedTexImage3DARB; // Extension "GL_ARB_texture_compression".
    extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC      glCompressedTexImage2DARB;
    extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC      glCompressedTexImage1DARB;
    extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC   glCompressedTexSubImage3DARB;
    extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC   glCompressedTexSubImage2DARB;
    extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC   glCompressedTexSubImage1DARB;
    extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC     glGetCompressedTexImageARB;

    extern PFNGLGENFRAGMENTSHADERSATIPROC        glGenFragmentShadersATI;   // Extension "GL_ATI_fragment_shader".
    extern PFNGLBINDFRAGMENTSHADERATIPROC        glBindFragmentShaderATI;
    extern PFNGLDELETEFRAGMENTSHADERATIPROC      glDeleteFragmentShaderATI;
    extern PFNGLBEGINFRAGMENTSHADERATIPROC       glBeginFragmentShaderATI;
    extern PFNGLENDFRAGMENTSHADERATIPROC         glEndFragmentShaderATI;
    extern PFNGLPASSTEXCOORDATIPROC              glPassTexCoordATI;
    extern PFNGLSAMPLEMAPATIPROC                 glSampleMapATI;
    extern PFNGLCOLORFRAGMENTOP1ATIPROC          glColorFragmentOp1ATI;
    extern PFNGLCOLORFRAGMENTOP2ATIPROC          glColorFragmentOp2ATI;
    extern PFNGLCOLORFRAGMENTOP3ATIPROC          glColorFragmentOp3ATI;
    extern PFNGLALPHAFRAGMENTOP1ATIPROC          glAlphaFragmentOp1ATI;
    extern PFNGLALPHAFRAGMENTOP2ATIPROC          glAlphaFragmentOp2ATI;
    extern PFNGLALPHAFRAGMENTOP3ATIPROC          glAlphaFragmentOp3ATI;
    extern PFNGLSETFRAGMENTSHADERCONSTANTATIPROC glSetFragmentShaderConstantATI;

    extern PFNGLBEGINVERTEXSHADEREXTPROC         glBeginVertexShaderEXT;    // Extension "GL_EXT_vertex_shader".
    extern PFNGLENDVERTEXSHADEREXTPROC           glEndVertexShaderEXT;
    extern PFNGLBINDVERTEXSHADEREXTPROC          glBindVertexShaderEXT;
    extern PFNGLGENVERTEXSHADERSEXTPROC          glGenVertexShadersEXT;
    extern PFNGLDELETEVERTEXSHADEREXTPROC        glDeleteVertexShaderEXT;
    extern PFNGLSHADEROP1EXTPROC                 glShaderOp1EXT;
    extern PFNGLSHADEROP2EXTPROC                 glShaderOp2EXT;
    extern PFNGLSHADEROP3EXTPROC                 glShaderOp3EXT;
    extern PFNGLSWIZZLEEXTPROC                   glSwizzleEXT;
    extern PFNGLWRITEMASKEXTPROC                 glWriteMaskEXT;
    extern PFNGLINSERTCOMPONENTEXTPROC           glInsertComponentEXT;
    extern PFNGLEXTRACTCOMPONENTEXTPROC          glExtractComponentEXT;
    extern PFNGLGENSYMBOLSEXTPROC                glGenSymbolsEXT;
    extern PFNGLSETINVARIANTEXTPROC              glSetInvariantEXT;
    extern PFNGLSETLOCALCONSTANTEXTPROC          glSetLocalConstantEXT;
    extern PFNGLVARIANTBVEXTPROC                 glVariantbvEXT;
    extern PFNGLVARIANTSVEXTPROC                 glVariantsvEXT;
    extern PFNGLVARIANTIVEXTPROC                 glVariantivEXT;
    extern PFNGLVARIANTFVEXTPROC                 glVariantfvEXT;
    extern PFNGLVARIANTDVEXTPROC                 glVariantdvEXT;
    extern PFNGLVARIANTUBVEXTPROC                glVariantubvEXT;
    extern PFNGLVARIANTUSVEXTPROC                glVariantusvEXT;
    extern PFNGLVARIANTUIVEXTPROC                glVariantuivEXT;
    extern PFNGLVARIANTPOINTEREXTPROC            glVariantPointerEXT;
    extern PFNGLENABLEVARIANTCLIENTSTATEEXTPROC  glEnableVariantClientStateEXT;
    extern PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC glDisableVariantClientStateEXT;
    extern PFNGLBINDLIGHTPARAMETEREXTPROC        glBindLightParameterEXT;
    extern PFNGLBINDMATERIALPARAMETEREXTPROC     glBindMaterialParameterEXT;
    extern PFNGLBINDTEXGENPARAMETEREXTPROC       glBindTexGenParameterEXT;
    extern PFNGLBINDTEXTUREUNITPARAMETEREXTPROC  glBindTextureUnitParameterEXT;
    extern PFNGLBINDPARAMETEREXTPROC             glBindParameterEXT;
    extern PFNGLISVARIANTENABLEDEXTPROC          glIsVariantEnabledEXT;
    extern PFNGLGETVARIANTBOOLEANVEXTPROC        glGetVariantBooleanvEXT;
    extern PFNGLGETVARIANTINTEGERVEXTPROC        glGetVariantIntegervEXT;
    extern PFNGLGETVARIANTFLOATVEXTPROC          glGetVariantFloatvEXT;
    extern PFNGLGETVARIANTPOINTERVEXTPROC        glGetVariantPointervEXT;
    extern PFNGLGETINVARIANTBOOLEANVEXTPROC      glGetInvariantBooleanvEXT;
    extern PFNGLGETINVARIANTINTEGERVEXTPROC      glGetInvariantIntegervEXT;
    extern PFNGLGETINVARIANTFLOATVEXTPROC        glGetInvariantFloatvEXT;
    extern PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC  glGetLocalConstantBooleanvEXT;
    extern PFNGLGETLOCALCONSTANTINTEGERVEXTPROC  glGetLocalConstantIntegervEXT;
    extern PFNGLGETLOCALCONSTANTFLOATVEXTPROC    glGetLocalConstantFloatvEXT;

    extern PFNGLACTIVESTENCILFACEEXTPROC         glActiveStencilFaceEXT;    // Extension "GL_EXT_stencil_two_side".

    extern PFNGLVERTEXATTRIB1DARBPROC             glVertexAttrib1dARB;      // Extensions "GL_ARB_vertex_program" and "GL_ARB_fragment_program".
    extern PFNGLVERTEXATTRIB1DVARBPROC            glVertexAttrib1dvARB;
    extern PFNGLVERTEXATTRIB1FARBPROC             glVertexAttrib1fARB;
    extern PFNGLVERTEXATTRIB1FVARBPROC            glVertexAttrib1fvARB;
    extern PFNGLVERTEXATTRIB1SARBPROC             glVertexAttrib1sARB;
    extern PFNGLVERTEXATTRIB1SVARBPROC            glVertexAttrib1svARB;
    extern PFNGLVERTEXATTRIB2DARBPROC             glVertexAttrib2dARB;
    extern PFNGLVERTEXATTRIB2DVARBPROC            glVertexAttrib2dvARB;
    extern PFNGLVERTEXATTRIB2FARBPROC             glVertexAttrib2fARB;
    extern PFNGLVERTEXATTRIB2FVARBPROC            glVertexAttrib2fvARB;
    extern PFNGLVERTEXATTRIB2SARBPROC             glVertexAttrib2sARB;
    extern PFNGLVERTEXATTRIB2SVARBPROC            glVertexAttrib2svARB;
    extern PFNGLVERTEXATTRIB3DARBPROC             glVertexAttrib3dARB;
    extern PFNGLVERTEXATTRIB3DVARBPROC            glVertexAttrib3dvARB;
    extern PFNGLVERTEXATTRIB3FARBPROC             glVertexAttrib3fARB;
    extern PFNGLVERTEXATTRIB3FVARBPROC            glVertexAttrib3fvARB;
    extern PFNGLVERTEXATTRIB3SARBPROC             glVertexAttrib3sARB;
    extern PFNGLVERTEXATTRIB3SVARBPROC            glVertexAttrib3svARB;
    extern PFNGLVERTEXATTRIB4NBVARBPROC           glVertexAttrib4NbvARB;
    extern PFNGLVERTEXATTRIB4NIVARBPROC           glVertexAttrib4NivARB;
    extern PFNGLVERTEXATTRIB4NSVARBPROC           glVertexAttrib4NsvARB;
    extern PFNGLVERTEXATTRIB4NUBARBPROC           glVertexAttrib4NubARB;
    extern PFNGLVERTEXATTRIB4NUBVARBPROC          glVertexAttrib4NubvARB;
    extern PFNGLVERTEXATTRIB4NUIVARBPROC          glVertexAttrib4NuivARB;
    extern PFNGLVERTEXATTRIB4NUSVARBPROC          glVertexAttrib4NusvARB;
    extern PFNGLVERTEXATTRIB4BVARBPROC            glVertexAttrib4bvARB;
    extern PFNGLVERTEXATTRIB4DARBPROC             glVertexAttrib4dARB;
    extern PFNGLVERTEXATTRIB4DVARBPROC            glVertexAttrib4dvARB;
    extern PFNGLVERTEXATTRIB4FARBPROC             glVertexAttrib4fARB;
    extern PFNGLVERTEXATTRIB4FVARBPROC            glVertexAttrib4fvARB;
    extern PFNGLVERTEXATTRIB4IVARBPROC            glVertexAttrib4ivARB;
    extern PFNGLVERTEXATTRIB4SARBPROC             glVertexAttrib4sARB;
    extern PFNGLVERTEXATTRIB4SVARBPROC            glVertexAttrib4svARB;
    extern PFNGLVERTEXATTRIB4UBVARBPROC           glVertexAttrib4ubvARB;
    extern PFNGLVERTEXATTRIB4UIVARBPROC           glVertexAttrib4uivARB;
    extern PFNGLVERTEXATTRIB4USVARBPROC           glVertexAttrib4usvARB;
    extern PFNGLVERTEXATTRIBPOINTERARBPROC        glVertexAttribPointerARB;
    extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC    glEnableVertexAttribArrayARB;
    extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC   glDisableVertexAttribArrayARB;
    extern PFNGLPROGRAMSTRINGARBPROC              glProgramStringARB;
    extern PFNGLBINDPROGRAMARBPROC                glBindProgramARB;
    extern PFNGLDELETEPROGRAMSARBPROC             glDeleteProgramsARB;
    extern PFNGLGENPROGRAMSARBPROC                glGenProgramsARB;
    extern PFNGLPROGRAMENVPARAMETER4DARBPROC      glProgramEnvParameter4dARB;
    extern PFNGLPROGRAMENVPARAMETER4DVARBPROC     glProgramEnvParameter4dvARB;
    extern PFNGLPROGRAMENVPARAMETER4FARBPROC      glProgramEnvParameter4fARB;
    extern PFNGLPROGRAMENVPARAMETER4FVARBPROC     glProgramEnvParameter4fvARB;
    extern PFNGLPROGRAMLOCALPARAMETER4DARBPROC    glProgramLocalParameter4dARB;
    extern PFNGLPROGRAMLOCALPARAMETER4DVARBPROC   glProgramLocalParameter4dvARB;
    extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC    glProgramLocalParameter4fARB;
    extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC   glProgramLocalParameter4fvARB;
    extern PFNGLGETPROGRAMENVPARAMETERDVARBPROC   glGetProgramEnvParameterdvARB;
    extern PFNGLGETPROGRAMENVPARAMETERFVARBPROC   glGetProgramEnvParameterfvARB;
    extern PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB;
    extern PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glGetProgramLocalParameterfvARB;
    extern PFNGLGETPROGRAMIVARBPROC               glGetProgramivARB;
    extern PFNGLGETPROGRAMSTRINGARBPROC           glGetProgramStringARB;
    extern PFNGLGETVERTEXATTRIBDVARBPROC          glGetVertexAttribdvARB;
    extern PFNGLGETVERTEXATTRIBFVARBPROC          glGetVertexAttribfvARB;
    extern PFNGLGETVERTEXATTRIBIVARBPROC          glGetVertexAttribivARB;
    extern PFNGLGETVERTEXATTRIBPOINTERVARBPROC    glGetVertexAttribPointervARB;
    extern PFNGLISPROGRAMARBPROC                  glIsProgramARB;
}

#endif
