/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/********************************/
/*** OpenGL Extensions (Code) ***/
/********************************/

#include "OpenGLEx.hpp"
#include <string.h>

#ifdef _WIN32
    #define GetProcAddress wglGetProcAddress
    #define StringPtr      const char*
#else
    // Don't confuse glx with glext...
    #define GLX_GLXEXT_LEGACY
    #include <GL/glx.h>

    #define GetProcAddress glXGetProcAddressARB
    #define StringPtr      const GLubyte*
#endif


bool cf::GL_ARB_multitexture_AVAIL               =false;
bool cf::GL_ARB_texture_cube_map_AVAIL           =false;
bool cf::GL_ARB_texture_compression_AVAIL        =false;
bool cf::GL_ATI_Radeon8500Shaders_AVAIL          =false;
bool cf::GL_EXT_stencil_wrap_AVAIL               =false;
bool cf::GL_EXT_stencil_two_side_AVAIL           =false;
bool cf::GL_ARB_vertex_and_fragment_program_AVAIL=false;


static bool IsExtensionAvailable(const char* ExtensionName)
{
    // Dies ist im wesentlichen der Code aus dem "OpenGL Programming Guide", Seite 568.
    const size_t LengthOfExtName=strlen(ExtensionName);
    char*        Extensions     =(char*)glGetString(GL_EXTENSIONS);

    if (Extensions==NULL) return false;

    const char* End=Extensions+strlen(Extensions);

    while (Extensions<End)
    {
        const size_t n=strcspn(Extensions, " ");

        if (LengthOfExtName==n && strncmp(ExtensionName, Extensions, n)==0) return true;

        Extensions+=(n+1);
    }

    return false;
}


PFNGLMULTITEXCOORD2DARBPROC     cf::glMultiTexCoord2dARB    =NULL;  // Extension "GL_ARB_multitexture"
PFNGLMULTITEXCOORD2FARBPROC     cf::glMultiTexCoord2fARB    =NULL;
PFNGLMULTITEXCOORD2FVARBPROC    cf::glMultiTexCoord2fvARB   =NULL;
PFNGLMULTITEXCOORD3FARBPROC     cf::glMultiTexCoord3fARB    =NULL;
PFNGLMULTITEXCOORD3FVARBPROC    cf::glMultiTexCoord3fvARB   =NULL;
PFNGLMULTITEXCOORD4FARBPROC     cf::glMultiTexCoord4fARB    =NULL;
PFNGLMULTITEXCOORD4FVARBPROC    cf::glMultiTexCoord4fvARB   =NULL;
PFNGLACTIVETEXTUREARBPROC       cf::glActiveTextureARB      =NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC cf::glClientActiveTextureARB=NULL;


void cf::Init_GL_ARB_multitexture()
{
    cf::GL_ARB_multitexture_AVAIL=false;

    if (!IsExtensionAvailable("GL_ARB_multitexture")) return;

    // Betrachte die Extension nur dann als vorhanden, wenn MaxTexUnits>1 ist.
    GLint MaxTexUnits=1;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &MaxTexUnits);
    if (glGetError()!=GL_NO_ERROR || MaxTexUnits<=1) return;

    // Nun besorge die Zeiger auf die Funktionen
    cf::glMultiTexCoord2dARB    =(PFNGLMULTITEXCOORD2DARBPROC    )GetProcAddress((StringPtr)"glMultiTexCoord2dARB");
    cf::glMultiTexCoord2fARB    =(PFNGLMULTITEXCOORD2FARBPROC    )GetProcAddress((StringPtr)"glMultiTexCoord2fARB");
    cf::glMultiTexCoord2fvARB   =(PFNGLMULTITEXCOORD2FVARBPROC   )GetProcAddress((StringPtr)"glMultiTexCoord2fvARB");
    cf::glMultiTexCoord3fARB    =(PFNGLMULTITEXCOORD3FARBPROC    )GetProcAddress((StringPtr)"glMultiTexCoord3fARB");
    cf::glMultiTexCoord3fvARB   =(PFNGLMULTITEXCOORD3FVARBPROC   )GetProcAddress((StringPtr)"glMultiTexCoord3fvARB");
    cf::glMultiTexCoord4fARB    =(PFNGLMULTITEXCOORD4FARBPROC    )GetProcAddress((StringPtr)"glMultiTexCoord4fARB");
    cf::glMultiTexCoord4fvARB   =(PFNGLMULTITEXCOORD4FVARBPROC   )GetProcAddress((StringPtr)"glMultiTexCoord4fvARB");
    cf::glActiveTextureARB      =(PFNGLACTIVETEXTUREARBPROC      )GetProcAddress((StringPtr)"glActiveTextureARB");
    cf::glClientActiveTextureARB=(PFNGLCLIENTACTIVETEXTUREARBPROC)GetProcAddress((StringPtr)"glClientActiveTextureARB");


    if (cf::glMultiTexCoord2dARB    ==NULL ||
        cf::glMultiTexCoord2fARB    ==NULL ||
        cf::glMultiTexCoord2fvARB   ==NULL ||
        cf::glMultiTexCoord3fARB    ==NULL ||
        cf::glMultiTexCoord3fvARB   ==NULL ||
        cf::glMultiTexCoord4fARB    ==NULL ||
        cf::glMultiTexCoord4fvARB   ==NULL ||
        cf::glActiveTextureARB      ==NULL ||
        cf::glClientActiveTextureARB==NULL)
    {
        cf::glMultiTexCoord2dARB    =NULL;
        cf::glMultiTexCoord2fARB    =NULL;
        cf::glMultiTexCoord2fvARB   =NULL;
        cf::glMultiTexCoord3fARB    =NULL;
        cf::glMultiTexCoord3fvARB   =NULL;
        cf::glMultiTexCoord4fARB    =NULL;
        cf::glMultiTexCoord4fvARB   =NULL;
        cf::glActiveTextureARB      =NULL;
        cf::glClientActiveTextureARB=NULL;
        return;
    }

    GL_ARB_multitexture_AVAIL=true;
}


void cf::Init_GL_ARB_texture_cube_map()
{
    cf::GL_ARB_texture_cube_map_AVAIL=IsExtensionAvailable("GL_ARB_texture_cube_map");
}


PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    cf::glCompressedTexImage3DARB   =NULL;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    cf::glCompressedTexImage2DARB   =NULL;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    cf::glCompressedTexImage1DARB   =NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC cf::glCompressedTexSubImage3DARB=NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC cf::glCompressedTexSubImage2DARB=NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC cf::glCompressedTexSubImage1DARB=NULL;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   cf::glGetCompressedTexImageARB  =NULL;


void cf::Init_GL_ARB_texture_compression()
{
    cf::GL_ARB_texture_compression_AVAIL=false;

    if (!IsExtensionAvailable("GL_ARB_texture_compression")) return;

    cf::glCompressedTexImage3DARB   =(PFNGLCOMPRESSEDTEXIMAGE3DARBPROC   )GetProcAddress((StringPtr)"glCompressedTexImage3DARB");
    cf::glCompressedTexImage2DARB   =(PFNGLCOMPRESSEDTEXIMAGE2DARBPROC   )GetProcAddress((StringPtr)"glCompressedTexImage2DARB");
    cf::glCompressedTexImage1DARB   =(PFNGLCOMPRESSEDTEXIMAGE1DARBPROC   )GetProcAddress((StringPtr)"glCompressedTexImage1DARB");
    cf::glCompressedTexSubImage3DARB=(PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)GetProcAddress((StringPtr)"glCompressedTexSubImage3DARB");
    cf::glCompressedTexSubImage2DARB=(PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)GetProcAddress((StringPtr)"glCompressedTexSubImage2DARB");
    cf::glCompressedTexSubImage1DARB=(PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)GetProcAddress((StringPtr)"glCompressedTexSubImage1DARB");
    cf::glGetCompressedTexImageARB  =(PFNGLGETCOMPRESSEDTEXIMAGEARBPROC  )GetProcAddress((StringPtr)"glGetCompressedTexImageARB");

    if (cf::glCompressedTexImage3DARB    ==NULL ||
        cf::glCompressedTexImage2DARB    ==NULL ||
        cf::glCompressedTexImage1DARB    ==NULL ||
        cf::glCompressedTexSubImage3DARB ==NULL ||
        cf::glCompressedTexSubImage2DARB ==NULL ||
        cf::glCompressedTexSubImage1DARB ==NULL ||
        cf::glGetCompressedTexImageARB   ==NULL)
    {
        cf::glCompressedTexImage3DARB   =NULL;
        cf::glCompressedTexImage2DARB   =NULL;
        cf::glCompressedTexImage1DARB   =NULL;
        cf::glCompressedTexSubImage3DARB=NULL;
        cf::glCompressedTexSubImage2DARB=NULL;
        cf::glCompressedTexSubImage1DARB=NULL;
        cf::glGetCompressedTexImageARB  =NULL;
        return;
    }

    cf::GL_ARB_texture_compression_AVAIL=true;
}


PFNGLGENFRAGMENTSHADERSATIPROC        cf::glGenFragmentShadersATI       =NULL;  // Extension "GL_ATI_fragment_shader"
PFNGLBINDFRAGMENTSHADERATIPROC        cf::glBindFragmentShaderATI       =NULL;
PFNGLDELETEFRAGMENTSHADERATIPROC      cf::glDeleteFragmentShaderATI     =NULL;
PFNGLBEGINFRAGMENTSHADERATIPROC       cf::glBeginFragmentShaderATI      =NULL;
PFNGLENDFRAGMENTSHADERATIPROC         cf::glEndFragmentShaderATI        =NULL;
PFNGLPASSTEXCOORDATIPROC              cf::glPassTexCoordATI             =NULL;
PFNGLSAMPLEMAPATIPROC                 cf::glSampleMapATI                =NULL;
PFNGLCOLORFRAGMENTOP1ATIPROC          cf::glColorFragmentOp1ATI         =NULL;
PFNGLCOLORFRAGMENTOP2ATIPROC          cf::glColorFragmentOp2ATI         =NULL;
PFNGLCOLORFRAGMENTOP3ATIPROC          cf::glColorFragmentOp3ATI         =NULL;
PFNGLALPHAFRAGMENTOP1ATIPROC          cf::glAlphaFragmentOp1ATI         =NULL;
PFNGLALPHAFRAGMENTOP2ATIPROC          cf::glAlphaFragmentOp2ATI         =NULL;
PFNGLALPHAFRAGMENTOP3ATIPROC          cf::glAlphaFragmentOp3ATI         =NULL;
PFNGLSETFRAGMENTSHADERCONSTANTATIPROC cf::glSetFragmentShaderConstantATI=NULL;

PFNGLBEGINVERTEXSHADEREXTPROC         cf::glBeginVertexShaderEXT        =NULL;  // Extension "GL_EXT_vertex_shader"
PFNGLENDVERTEXSHADEREXTPROC           cf::glEndVertexShaderEXT          =NULL;
PFNGLBINDVERTEXSHADEREXTPROC          cf::glBindVertexShaderEXT         =NULL;
PFNGLGENVERTEXSHADERSEXTPROC          cf::glGenVertexShadersEXT         =NULL;
PFNGLDELETEVERTEXSHADEREXTPROC        cf::glDeleteVertexShaderEXT       =NULL;
PFNGLSHADEROP1EXTPROC                 cf::glShaderOp1EXT                =NULL;
PFNGLSHADEROP2EXTPROC                 cf::glShaderOp2EXT                =NULL;
PFNGLSHADEROP3EXTPROC                 cf::glShaderOp3EXT                =NULL;
PFNGLSWIZZLEEXTPROC                   cf::glSwizzleEXT                  =NULL;
PFNGLWRITEMASKEXTPROC                 cf::glWriteMaskEXT                =NULL;
PFNGLINSERTCOMPONENTEXTPROC           cf::glInsertComponentEXT          =NULL;
PFNGLEXTRACTCOMPONENTEXTPROC          cf::glExtractComponentEXT         =NULL;
PFNGLGENSYMBOLSEXTPROC                cf::glGenSymbolsEXT               =NULL;
PFNGLSETINVARIANTEXTPROC              cf::glSetInvariantEXT             =NULL;
PFNGLSETLOCALCONSTANTEXTPROC          cf::glSetLocalConstantEXT         =NULL;
PFNGLVARIANTBVEXTPROC                 cf::glVariantbvEXT                =NULL;
PFNGLVARIANTSVEXTPROC                 cf::glVariantsvEXT                =NULL;
PFNGLVARIANTIVEXTPROC                 cf::glVariantivEXT                =NULL;
PFNGLVARIANTFVEXTPROC                 cf::glVariantfvEXT                =NULL;
PFNGLVARIANTDVEXTPROC                 cf::glVariantdvEXT                =NULL;
PFNGLVARIANTUBVEXTPROC                cf::glVariantubvEXT               =NULL;
PFNGLVARIANTUSVEXTPROC                cf::glVariantusvEXT               =NULL;
PFNGLVARIANTUIVEXTPROC                cf::glVariantuivEXT               =NULL;
PFNGLVARIANTPOINTEREXTPROC            cf::glVariantPointerEXT           =NULL;
PFNGLENABLEVARIANTCLIENTSTATEEXTPROC  cf::glEnableVariantClientStateEXT =NULL;
PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC cf::glDisableVariantClientStateEXT=NULL;
PFNGLBINDLIGHTPARAMETEREXTPROC        cf::glBindLightParameterEXT       =NULL;
PFNGLBINDMATERIALPARAMETEREXTPROC     cf::glBindMaterialParameterEXT    =NULL;
PFNGLBINDTEXGENPARAMETEREXTPROC       cf::glBindTexGenParameterEXT      =NULL;
PFNGLBINDTEXTUREUNITPARAMETEREXTPROC  cf::glBindTextureUnitParameterEXT =NULL;
PFNGLBINDPARAMETEREXTPROC             cf::glBindParameterEXT            =NULL;
PFNGLISVARIANTENABLEDEXTPROC          cf::glIsVariantEnabledEXT         =NULL;
PFNGLGETVARIANTBOOLEANVEXTPROC        cf::glGetVariantBooleanvEXT       =NULL;
PFNGLGETVARIANTINTEGERVEXTPROC        cf::glGetVariantIntegervEXT       =NULL;
PFNGLGETVARIANTFLOATVEXTPROC          cf::glGetVariantFloatvEXT         =NULL;
PFNGLGETVARIANTPOINTERVEXTPROC        cf::glGetVariantPointervEXT       =NULL;
PFNGLGETINVARIANTBOOLEANVEXTPROC      cf::glGetInvariantBooleanvEXT     =NULL;
PFNGLGETINVARIANTINTEGERVEXTPROC      cf::glGetInvariantIntegervEXT     =NULL;
PFNGLGETINVARIANTFLOATVEXTPROC        cf::glGetInvariantFloatvEXT       =NULL;
PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC  cf::glGetLocalConstantBooleanvEXT =NULL;
PFNGLGETLOCALCONSTANTINTEGERVEXTPROC  cf::glGetLocalConstantIntegervEXT =NULL;
PFNGLGETLOCALCONSTANTFLOATVEXTPROC    cf::glGetLocalConstantFloatvEXT   =NULL;


void Init_ATI_Radeon8500Shaders()
{
    cf::GL_ATI_Radeon8500Shaders_AVAIL=false;

    if (!IsExtensionAvailable("GL_ATI_fragment_shader")) return;
    if (!IsExtensionAvailable("GL_EXT_vertex_shader"  )) return;

    cf::glGenFragmentShadersATI       =(PFNGLGENFRAGMENTSHADERSATIPROC       )GetProcAddress((StringPtr)"glGenFragmentShadersATI"       ); if (cf::glGenFragmentShadersATI       ==NULL) return;
    cf::glBindFragmentShaderATI       =(PFNGLBINDFRAGMENTSHADERATIPROC       )GetProcAddress((StringPtr)"glBindFragmentShaderATI"       ); if (cf::glBindFragmentShaderATI       ==NULL) return;
    cf::glDeleteFragmentShaderATI     =(PFNGLDELETEFRAGMENTSHADERATIPROC     )GetProcAddress((StringPtr)"glDeleteFragmentShaderATI"     ); if (cf::glDeleteFragmentShaderATI     ==NULL) return;
    cf::glBeginFragmentShaderATI      =(PFNGLBEGINFRAGMENTSHADERATIPROC      )GetProcAddress((StringPtr)"glBeginFragmentShaderATI"      ); if (cf::glBeginFragmentShaderATI      ==NULL) return;
    cf::glEndFragmentShaderATI        =(PFNGLENDFRAGMENTSHADERATIPROC        )GetProcAddress((StringPtr)"glEndFragmentShaderATI"        ); if (cf::glEndFragmentShaderATI        ==NULL) return;
    cf::glPassTexCoordATI             =(PFNGLPASSTEXCOORDATIPROC             )GetProcAddress((StringPtr)"glPassTexCoordATI"             ); if (cf::glPassTexCoordATI             ==NULL) return;
    cf::glSampleMapATI                =(PFNGLSAMPLEMAPATIPROC                )GetProcAddress((StringPtr)"glSampleMapATI"                ); if (cf::glSampleMapATI                ==NULL) return;
    cf::glColorFragmentOp1ATI         =(PFNGLCOLORFRAGMENTOP1ATIPROC         )GetProcAddress((StringPtr)"glColorFragmentOp1ATI"         ); if (cf::glColorFragmentOp1ATI         ==NULL) return;
    cf::glColorFragmentOp2ATI         =(PFNGLCOLORFRAGMENTOP2ATIPROC         )GetProcAddress((StringPtr)"glColorFragmentOp2ATI"         ); if (cf::glColorFragmentOp2ATI         ==NULL) return;
    cf::glColorFragmentOp3ATI         =(PFNGLCOLORFRAGMENTOP3ATIPROC         )GetProcAddress((StringPtr)"glColorFragmentOp3ATI"         ); if (cf::glColorFragmentOp3ATI         ==NULL) return;
    cf::glAlphaFragmentOp1ATI         =(PFNGLALPHAFRAGMENTOP1ATIPROC         )GetProcAddress((StringPtr)"glAlphaFragmentOp1ATI"         ); if (cf::glAlphaFragmentOp1ATI         ==NULL) return;
    cf::glAlphaFragmentOp2ATI         =(PFNGLALPHAFRAGMENTOP2ATIPROC         )GetProcAddress((StringPtr)"glAlphaFragmentOp2ATI"         ); if (cf::glAlphaFragmentOp2ATI         ==NULL) return;
    cf::glAlphaFragmentOp3ATI         =(PFNGLALPHAFRAGMENTOP3ATIPROC         )GetProcAddress((StringPtr)"glAlphaFragmentOp3ATI"         ); if (cf::glAlphaFragmentOp3ATI         ==NULL) return;
    cf::glSetFragmentShaderConstantATI=(PFNGLSETFRAGMENTSHADERCONSTANTATIPROC)GetProcAddress((StringPtr)"glSetFragmentShaderConstantATI"); if (cf::glSetFragmentShaderConstantATI==NULL) return;

    cf::glBeginVertexShaderEXT        =(PFNGLBEGINVERTEXSHADEREXTPROC        )GetProcAddress((StringPtr)"glBeginVertexShaderEXT"        ); if (cf::glBeginVertexShaderEXT        ==NULL) return;
    cf::glEndVertexShaderEXT          =(PFNGLENDVERTEXSHADEREXTPROC          )GetProcAddress((StringPtr)"glEndVertexShaderEXT"          ); if (cf::glEndVertexShaderEXT          ==NULL) return;
    cf::glBindVertexShaderEXT         =(PFNGLBINDVERTEXSHADEREXTPROC         )GetProcAddress((StringPtr)"glBindVertexShaderEXT"         ); if (cf::glBindVertexShaderEXT         ==NULL) return;
    cf::glGenVertexShadersEXT         =(PFNGLGENVERTEXSHADERSEXTPROC         )GetProcAddress((StringPtr)"glGenVertexShadersEXT"         ); if (cf::glGenVertexShadersEXT         ==NULL) return;
    cf::glDeleteVertexShaderEXT       =(PFNGLDELETEVERTEXSHADEREXTPROC       )GetProcAddress((StringPtr)"glDeleteVertexShaderEXT"       ); if (cf::glDeleteVertexShaderEXT       ==NULL) return;
    cf::glShaderOp1EXT                =(PFNGLSHADEROP1EXTPROC                )GetProcAddress((StringPtr)"glShaderOp1EXT"                ); if (cf::glShaderOp1EXT                ==NULL) return;
    cf::glShaderOp2EXT                =(PFNGLSHADEROP2EXTPROC                )GetProcAddress((StringPtr)"glShaderOp2EXT"                ); if (cf::glShaderOp2EXT                ==NULL) return;
    cf::glShaderOp3EXT                =(PFNGLSHADEROP3EXTPROC                )GetProcAddress((StringPtr)"glShaderOp3EXT"                ); if (cf::glShaderOp3EXT                ==NULL) return;
    cf::glSwizzleEXT                  =(PFNGLSWIZZLEEXTPROC                  )GetProcAddress((StringPtr)"glSwizzleEXT"                  ); if (cf::glSwizzleEXT                  ==NULL) return;
    cf::glWriteMaskEXT                =(PFNGLWRITEMASKEXTPROC                )GetProcAddress((StringPtr)"glWriteMaskEXT"                ); if (cf::glWriteMaskEXT                ==NULL) return;
    cf::glInsertComponentEXT          =(PFNGLINSERTCOMPONENTEXTPROC          )GetProcAddress((StringPtr)"glInsertComponentEXT"          ); if (cf::glInsertComponentEXT          ==NULL) return;
    cf::glExtractComponentEXT         =(PFNGLEXTRACTCOMPONENTEXTPROC         )GetProcAddress((StringPtr)"glExtractComponentEXT"         ); if (cf::glExtractComponentEXT         ==NULL) return;
    cf::glGenSymbolsEXT               =(PFNGLGENSYMBOLSEXTPROC               )GetProcAddress((StringPtr)"glGenSymbolsEXT"               ); if (cf::glGenSymbolsEXT               ==NULL) return;
    cf::glSetInvariantEXT             =(PFNGLSETINVARIANTEXTPROC             )GetProcAddress((StringPtr)"glSetInvariantEXT"             ); if (cf::glSetInvariantEXT             ==NULL) return;
    cf::glSetLocalConstantEXT         =(PFNGLSETLOCALCONSTANTEXTPROC         )GetProcAddress((StringPtr)"glSetLocalConstantEXT"         ); if (cf::glSetLocalConstantEXT         ==NULL) return;
    cf::glVariantbvEXT                =(PFNGLVARIANTBVEXTPROC                )GetProcAddress((StringPtr)"glVariantbvEXT"                ); if (cf::glVariantbvEXT                ==NULL) return;
    cf::glVariantsvEXT                =(PFNGLVARIANTSVEXTPROC                )GetProcAddress((StringPtr)"glVariantsvEXT"                ); if (cf::glVariantsvEXT                ==NULL) return;
    cf::glVariantivEXT                =(PFNGLVARIANTIVEXTPROC                )GetProcAddress((StringPtr)"glVariantivEXT"                ); if (cf::glVariantivEXT                ==NULL) return;
    cf::glVariantfvEXT                =(PFNGLVARIANTFVEXTPROC                )GetProcAddress((StringPtr)"glVariantfvEXT"                ); if (cf::glVariantfvEXT                ==NULL) return;
    cf::glVariantdvEXT                =(PFNGLVARIANTDVEXTPROC                )GetProcAddress((StringPtr)"glVariantdvEXT"                ); if (cf::glVariantdvEXT                ==NULL) return;
    cf::glVariantubvEXT               =(PFNGLVARIANTUBVEXTPROC               )GetProcAddress((StringPtr)"glVariantubvEXT"               ); if (cf::glVariantubvEXT               ==NULL) return;
    cf::glVariantusvEXT               =(PFNGLVARIANTUSVEXTPROC               )GetProcAddress((StringPtr)"glVariantusvEXT"               ); if (cf::glVariantusvEXT               ==NULL) return;
    cf::glVariantuivEXT               =(PFNGLVARIANTUIVEXTPROC               )GetProcAddress((StringPtr)"glVariantuivEXT"               ); if (cf::glVariantuivEXT               ==NULL) return;
    cf::glVariantPointerEXT           =(PFNGLVARIANTPOINTEREXTPROC           )GetProcAddress((StringPtr)"glVariantPointerEXT"           ); if (cf::glVariantPointerEXT           ==NULL) return;
    cf::glEnableVariantClientStateEXT =(PFNGLENABLEVARIANTCLIENTSTATEEXTPROC )GetProcAddress((StringPtr)"glEnableVariantClientStateEXT" ); if (cf::glEnableVariantClientStateEXT ==NULL) return;
    cf::glDisableVariantClientStateEXT=(PFNGLDISABLEVARIANTCLIENTSTATEEXTPROC)GetProcAddress((StringPtr)"glDisableVariantClientStateEXT"); if (cf::glDisableVariantClientStateEXT==NULL) return;
    cf::glBindLightParameterEXT       =(PFNGLBINDLIGHTPARAMETEREXTPROC       )GetProcAddress((StringPtr)"glBindLightParameterEXT"       ); if (cf::glBindLightParameterEXT       ==NULL) return;
    cf::glBindMaterialParameterEXT    =(PFNGLBINDMATERIALPARAMETEREXTPROC    )GetProcAddress((StringPtr)"glBindMaterialParameterEXT"    ); if (cf::glBindMaterialParameterEXT    ==NULL) return;
    cf::glBindTexGenParameterEXT      =(PFNGLBINDTEXGENPARAMETEREXTPROC      )GetProcAddress((StringPtr)"glBindTexGenParameterEXT"      ); if (cf::glBindTexGenParameterEXT      ==NULL) return;
    cf::glBindTextureUnitParameterEXT =(PFNGLBINDTEXTUREUNITPARAMETEREXTPROC )GetProcAddress((StringPtr)"glBindTextureUnitParameterEXT" ); if (cf::glBindTextureUnitParameterEXT ==NULL) return;
    cf::glBindParameterEXT            =(PFNGLBINDPARAMETEREXTPROC            )GetProcAddress((StringPtr)"glBindParameterEXT"            ); if (cf::glBindParameterEXT            ==NULL) return;
    cf::glIsVariantEnabledEXT         =(PFNGLISVARIANTENABLEDEXTPROC         )GetProcAddress((StringPtr)"glIsVariantEnabledEXT"         ); if (cf::glIsVariantEnabledEXT         ==NULL) return;
    cf::glGetVariantBooleanvEXT       =(PFNGLGETVARIANTBOOLEANVEXTPROC       )GetProcAddress((StringPtr)"glGetVariantBooleanvEXT"       ); if (cf::glGetVariantBooleanvEXT       ==NULL) return;
    cf::glGetVariantIntegervEXT       =(PFNGLGETVARIANTINTEGERVEXTPROC       )GetProcAddress((StringPtr)"glGetVariantIntegervEXT"       ); if (cf::glGetVariantIntegervEXT       ==NULL) return;
    cf::glGetVariantFloatvEXT         =(PFNGLGETVARIANTFLOATVEXTPROC         )GetProcAddress((StringPtr)"glGetVariantFloatvEXT"         ); if (cf::glGetVariantFloatvEXT         ==NULL) return;
    cf::glGetVariantPointervEXT       =(PFNGLGETVARIANTPOINTERVEXTPROC       )GetProcAddress((StringPtr)"glGetVariantPointervEXT"       ); if (cf::glGetVariantPointervEXT       ==NULL) return;
    cf::glGetInvariantBooleanvEXT     =(PFNGLGETINVARIANTBOOLEANVEXTPROC     )GetProcAddress((StringPtr)"glGetInvariantBooleanvEXT"     ); if (cf::glGetInvariantBooleanvEXT     ==NULL) return;
    cf::glGetInvariantIntegervEXT     =(PFNGLGETINVARIANTINTEGERVEXTPROC     )GetProcAddress((StringPtr)"glGetInvariantIntegervEXT"     ); if (cf::glGetInvariantIntegervEXT     ==NULL) return;
    cf::glGetInvariantFloatvEXT       =(PFNGLGETINVARIANTFLOATVEXTPROC       )GetProcAddress((StringPtr)"glGetInvariantFloatvEXT"       ); if (cf::glGetInvariantFloatvEXT       ==NULL) return;
    cf::glGetLocalConstantBooleanvEXT =(PFNGLGETLOCALCONSTANTBOOLEANVEXTPROC )GetProcAddress((StringPtr)"glGetLocalConstantBooleanvEXT" ); if (cf::glGetLocalConstantBooleanvEXT ==NULL) return;
    cf::glGetLocalConstantIntegervEXT =(PFNGLGETLOCALCONSTANTINTEGERVEXTPROC )GetProcAddress((StringPtr)"glGetLocalConstantIntegervEXT" ); if (cf::glGetLocalConstantIntegervEXT ==NULL) return;
    cf::glGetLocalConstantFloatvEXT   =(PFNGLGETLOCALCONSTANTFLOATVEXTPROC   )GetProcAddress((StringPtr)"glGetLocalConstantFloatvEXT"   ); if (cf::glGetLocalConstantFloatvEXT   ==NULL) return;

    cf::GL_ATI_Radeon8500Shaders_AVAIL=true;
}


void cf::Init_GL_EXT_stencil_wrap()
{
    cf::GL_EXT_stencil_wrap_AVAIL=IsExtensionAvailable("GL_EXT_stencil_wrap");
}


PFNGLACTIVESTENCILFACEEXTPROC cf::glActiveStencilFaceEXT=NULL;


void cf::Init_GL_EXT_stencil_two_side()
{
    cf::GL_EXT_stencil_two_side_AVAIL=false;

    if (!IsExtensionAvailable("GL_EXT_stencil_two_side")) return;

    cf::glActiveStencilFaceEXT=(PFNGLACTIVESTENCILFACEEXTPROC)GetProcAddress((StringPtr)"glActiveStencilFaceEXT"); if (glActiveStencilFaceEXT==NULL) return;

    cf::GL_EXT_stencil_two_side_AVAIL=true;
}


PFNGLVERTEXATTRIB1DARBPROC             cf::glVertexAttrib1dARB            =NULL;    // Extensions "GL_ARB_vertex_program" and "GL_ARB_fragment_program".
PFNGLVERTEXATTRIB1DVARBPROC            cf::glVertexAttrib1dvARB           =NULL;
PFNGLVERTEXATTRIB1FARBPROC             cf::glVertexAttrib1fARB            =NULL;
PFNGLVERTEXATTRIB1FVARBPROC            cf::glVertexAttrib1fvARB           =NULL;
PFNGLVERTEXATTRIB1SARBPROC             cf::glVertexAttrib1sARB            =NULL;
PFNGLVERTEXATTRIB1SVARBPROC            cf::glVertexAttrib1svARB           =NULL;
PFNGLVERTEXATTRIB2DARBPROC             cf::glVertexAttrib2dARB            =NULL;
PFNGLVERTEXATTRIB2DVARBPROC            cf::glVertexAttrib2dvARB           =NULL;
PFNGLVERTEXATTRIB2FARBPROC             cf::glVertexAttrib2fARB            =NULL;
PFNGLVERTEXATTRIB2FVARBPROC            cf::glVertexAttrib2fvARB           =NULL;
PFNGLVERTEXATTRIB2SARBPROC             cf::glVertexAttrib2sARB            =NULL;
PFNGLVERTEXATTRIB2SVARBPROC            cf::glVertexAttrib2svARB           =NULL;
PFNGLVERTEXATTRIB3DARBPROC             cf::glVertexAttrib3dARB            =NULL;
PFNGLVERTEXATTRIB3DVARBPROC            cf::glVertexAttrib3dvARB           =NULL;
PFNGLVERTEXATTRIB3FARBPROC             cf::glVertexAttrib3fARB            =NULL;
PFNGLVERTEXATTRIB3FVARBPROC            cf::glVertexAttrib3fvARB           =NULL;
PFNGLVERTEXATTRIB3SARBPROC             cf::glVertexAttrib3sARB            =NULL;
PFNGLVERTEXATTRIB3SVARBPROC            cf::glVertexAttrib3svARB           =NULL;
PFNGLVERTEXATTRIB4NBVARBPROC           cf::glVertexAttrib4NbvARB          =NULL;
PFNGLVERTEXATTRIB4NIVARBPROC           cf::glVertexAttrib4NivARB          =NULL;
PFNGLVERTEXATTRIB4NSVARBPROC           cf::glVertexAttrib4NsvARB          =NULL;
PFNGLVERTEXATTRIB4NUBARBPROC           cf::glVertexAttrib4NubARB          =NULL;
PFNGLVERTEXATTRIB4NUBVARBPROC          cf::glVertexAttrib4NubvARB         =NULL;
PFNGLVERTEXATTRIB4NUIVARBPROC          cf::glVertexAttrib4NuivARB         =NULL;
PFNGLVERTEXATTRIB4NUSVARBPROC          cf::glVertexAttrib4NusvARB         =NULL;
PFNGLVERTEXATTRIB4BVARBPROC            cf::glVertexAttrib4bvARB           =NULL;
PFNGLVERTEXATTRIB4DARBPROC             cf::glVertexAttrib4dARB            =NULL;
PFNGLVERTEXATTRIB4DVARBPROC            cf::glVertexAttrib4dvARB           =NULL;
PFNGLVERTEXATTRIB4FARBPROC             cf::glVertexAttrib4fARB            =NULL;
PFNGLVERTEXATTRIB4FVARBPROC            cf::glVertexAttrib4fvARB           =NULL;
PFNGLVERTEXATTRIB4IVARBPROC            cf::glVertexAttrib4ivARB           =NULL;
PFNGLVERTEXATTRIB4SARBPROC             cf::glVertexAttrib4sARB            =NULL;
PFNGLVERTEXATTRIB4SVARBPROC            cf::glVertexAttrib4svARB           =NULL;
PFNGLVERTEXATTRIB4UBVARBPROC           cf::glVertexAttrib4ubvARB          =NULL;
PFNGLVERTEXATTRIB4UIVARBPROC           cf::glVertexAttrib4uivARB          =NULL;
PFNGLVERTEXATTRIB4USVARBPROC           cf::glVertexAttrib4usvARB          =NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC        cf::glVertexAttribPointerARB       =NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC    cf::glEnableVertexAttribArrayARB   =NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC   cf::glDisableVertexAttribArrayARB  =NULL;
PFNGLPROGRAMSTRINGARBPROC              cf::glProgramStringARB             =NULL;
PFNGLBINDPROGRAMARBPROC                cf::glBindProgramARB               =NULL;
PFNGLDELETEPROGRAMSARBPROC             cf::glDeleteProgramsARB            =NULL;
PFNGLGENPROGRAMSARBPROC                cf::glGenProgramsARB               =NULL;
PFNGLPROGRAMENVPARAMETER4DARBPROC      cf::glProgramEnvParameter4dARB     =NULL;
PFNGLPROGRAMENVPARAMETER4DVARBPROC     cf::glProgramEnvParameter4dvARB    =NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC      cf::glProgramEnvParameter4fARB     =NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC     cf::glProgramEnvParameter4fvARB    =NULL;
PFNGLPROGRAMLOCALPARAMETER4DARBPROC    cf::glProgramLocalParameter4dARB   =NULL;
PFNGLPROGRAMLOCALPARAMETER4DVARBPROC   cf::glProgramLocalParameter4dvARB  =NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC    cf::glProgramLocalParameter4fARB   =NULL;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC   cf::glProgramLocalParameter4fvARB  =NULL;
PFNGLGETPROGRAMENVPARAMETERDVARBPROC   cf::glGetProgramEnvParameterdvARB  =NULL;
PFNGLGETPROGRAMENVPARAMETERFVARBPROC   cf::glGetProgramEnvParameterfvARB  =NULL;
PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC cf::glGetProgramLocalParameterdvARB=NULL;
PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC cf::glGetProgramLocalParameterfvARB=NULL;
PFNGLGETPROGRAMIVARBPROC               cf::glGetProgramivARB              =NULL;
PFNGLGETPROGRAMSTRINGARBPROC           cf::glGetProgramStringARB          =NULL;
PFNGLGETVERTEXATTRIBDVARBPROC          cf::glGetVertexAttribdvARB         =NULL;
PFNGLGETVERTEXATTRIBFVARBPROC          cf::glGetVertexAttribfvARB         =NULL;
PFNGLGETVERTEXATTRIBIVARBPROC          cf::glGetVertexAttribivARB         =NULL;
PFNGLGETVERTEXATTRIBPOINTERVARBPROC    cf::glGetVertexAttribPointervARB   =NULL;
PFNGLISPROGRAMARBPROC                  cf::glIsProgramARB                 =NULL;


void cf::Init_GL_ARB_vertex_and_fragment_program()
{
    cf::GL_ARB_vertex_and_fragment_program_AVAIL=false;

    if (!IsExtensionAvailable("GL_ARB_vertex_program"  )) return;
    if (!IsExtensionAvailable("GL_ARB_fragment_program")) return;

    cf::glVertexAttrib1dARB            =(PFNGLVERTEXATTRIB1DARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib1dARB"            ); if (glVertexAttrib1dARB            ==NULL) return;
    cf::glVertexAttrib1dvARB           =(PFNGLVERTEXATTRIB1DVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib1dvARB"           ); if (glVertexAttrib1dvARB           ==NULL) return;
    cf::glVertexAttrib1fARB            =(PFNGLVERTEXATTRIB1FARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib1fARB"            ); if (glVertexAttrib1fARB            ==NULL) return;
    cf::glVertexAttrib1fvARB           =(PFNGLVERTEXATTRIB1FVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib1fvARB"           ); if (glVertexAttrib1fvARB           ==NULL) return;
    cf::glVertexAttrib1sARB            =(PFNGLVERTEXATTRIB1SARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib1sARB"            ); if (glVertexAttrib1sARB            ==NULL) return;
    cf::glVertexAttrib1svARB           =(PFNGLVERTEXATTRIB1SVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib1svARB"           ); if (glVertexAttrib1svARB           ==NULL) return;
    cf::glVertexAttrib2dARB            =(PFNGLVERTEXATTRIB2DARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib2dARB"            ); if (glVertexAttrib2dARB            ==NULL) return;
    cf::glVertexAttrib2dvARB           =(PFNGLVERTEXATTRIB2DVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib2dvARB"           ); if (glVertexAttrib2dvARB           ==NULL) return;
    cf::glVertexAttrib2fARB            =(PFNGLVERTEXATTRIB2FARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib2fARB"            ); if (glVertexAttrib2fARB            ==NULL) return;
    cf::glVertexAttrib2fvARB           =(PFNGLVERTEXATTRIB2FVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib2fvARB"           ); if (glVertexAttrib2fvARB           ==NULL) return;
    cf::glVertexAttrib2sARB            =(PFNGLVERTEXATTRIB2SARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib2sARB"            ); if (glVertexAttrib2sARB            ==NULL) return;
    cf::glVertexAttrib2svARB           =(PFNGLVERTEXATTRIB2SVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib2svARB"           ); if (glVertexAttrib2svARB           ==NULL) return;
    cf::glVertexAttrib3dARB            =(PFNGLVERTEXATTRIB3DARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib3dARB"            ); if (glVertexAttrib3dARB            ==NULL) return;
    cf::glVertexAttrib3dvARB           =(PFNGLVERTEXATTRIB3DVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib3dvARB"           ); if (glVertexAttrib3dvARB           ==NULL) return;
    cf::glVertexAttrib3fARB            =(PFNGLVERTEXATTRIB3FARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib3fARB"            ); if (glVertexAttrib3fARB            ==NULL) return;
    cf::glVertexAttrib3fvARB           =(PFNGLVERTEXATTRIB3FVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib3fvARB"           ); if (glVertexAttrib3fvARB           ==NULL) return;
    cf::glVertexAttrib3sARB            =(PFNGLVERTEXATTRIB3SARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib3sARB"            ); if (glVertexAttrib3sARB            ==NULL) return;
    cf::glVertexAttrib3svARB           =(PFNGLVERTEXATTRIB3SVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib3svARB"           ); if (glVertexAttrib3svARB           ==NULL) return;
    cf::glVertexAttrib4NbvARB          =(PFNGLVERTEXATTRIB4NBVARBPROC          )GetProcAddress((StringPtr)"glVertexAttrib4NbvARB"          ); if (glVertexAttrib4NbvARB          ==NULL) return;
    cf::glVertexAttrib4NivARB          =(PFNGLVERTEXATTRIB4NIVARBPROC          )GetProcAddress((StringPtr)"glVertexAttrib4NivARB"          ); if (glVertexAttrib4NivARB          ==NULL) return;
    cf::glVertexAttrib4NsvARB          =(PFNGLVERTEXATTRIB4NSVARBPROC          )GetProcAddress((StringPtr)"glVertexAttrib4NsvARB"          ); if (glVertexAttrib4NsvARB          ==NULL) return;
    cf::glVertexAttrib4NubARB          =(PFNGLVERTEXATTRIB4NUBARBPROC          )GetProcAddress((StringPtr)"glVertexAttrib4NubARB"          ); if (glVertexAttrib4NubARB          ==NULL) return;
    cf::glVertexAttrib4NubvARB         =(PFNGLVERTEXATTRIB4NUBVARBPROC         )GetProcAddress((StringPtr)"glVertexAttrib4NubvARB"         ); if (glVertexAttrib4NubvARB         ==NULL) return;
    cf::glVertexAttrib4NuivARB         =(PFNGLVERTEXATTRIB4NUIVARBPROC         )GetProcAddress((StringPtr)"glVertexAttrib4NuivARB"         ); if (glVertexAttrib4NuivARB         ==NULL) return;
    cf::glVertexAttrib4NusvARB         =(PFNGLVERTEXATTRIB4NUSVARBPROC         )GetProcAddress((StringPtr)"glVertexAttrib4NusvARB"         ); if (glVertexAttrib4NusvARB         ==NULL) return;
    cf::glVertexAttrib4bvARB           =(PFNGLVERTEXATTRIB4BVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib4bvARB"           ); if (glVertexAttrib4bvARB           ==NULL) return;
    cf::glVertexAttrib4dARB            =(PFNGLVERTEXATTRIB4DARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib4dARB"            ); if (glVertexAttrib4dARB            ==NULL) return;
    cf::glVertexAttrib4dvARB           =(PFNGLVERTEXATTRIB4DVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib4dvARB"           ); if (glVertexAttrib4dvARB           ==NULL) return;
    cf::glVertexAttrib4fARB            =(PFNGLVERTEXATTRIB4FARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib4fARB"            ); if (glVertexAttrib4fARB            ==NULL) return;
    cf::glVertexAttrib4fvARB           =(PFNGLVERTEXATTRIB4FVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib4fvARB"           ); if (glVertexAttrib4fvARB           ==NULL) return;
    cf::glVertexAttrib4ivARB           =(PFNGLVERTEXATTRIB4IVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib4ivARB"           ); if (glVertexAttrib4ivARB           ==NULL) return;
    cf::glVertexAttrib4sARB            =(PFNGLVERTEXATTRIB4SARBPROC            )GetProcAddress((StringPtr)"glVertexAttrib4sARB"            ); if (glVertexAttrib4sARB            ==NULL) return;
    cf::glVertexAttrib4svARB           =(PFNGLVERTEXATTRIB4SVARBPROC           )GetProcAddress((StringPtr)"glVertexAttrib4svARB"           ); if (glVertexAttrib4svARB           ==NULL) return;
    cf::glVertexAttrib4ubvARB          =(PFNGLVERTEXATTRIB4UBVARBPROC          )GetProcAddress((StringPtr)"glVertexAttrib4ubvARB"          ); if (glVertexAttrib4ubvARB          ==NULL) return;
    cf::glVertexAttrib4uivARB          =(PFNGLVERTEXATTRIB4UIVARBPROC          )GetProcAddress((StringPtr)"glVertexAttrib4uivARB"          ); if (glVertexAttrib4uivARB          ==NULL) return;
    cf::glVertexAttrib4usvARB          =(PFNGLVERTEXATTRIB4USVARBPROC          )GetProcAddress((StringPtr)"glVertexAttrib4usvARB"          ); if (glVertexAttrib4usvARB          ==NULL) return;
    cf::glVertexAttribPointerARB       =(PFNGLVERTEXATTRIBPOINTERARBPROC       )GetProcAddress((StringPtr)"glVertexAttribPointerARB"       ); if (glVertexAttribPointerARB       ==NULL) return;
    cf::glEnableVertexAttribArrayARB   =(PFNGLENABLEVERTEXATTRIBARRAYARBPROC   )GetProcAddress((StringPtr)"glEnableVertexAttribArrayARB"   ); if (glEnableVertexAttribArrayARB   ==NULL) return;
    cf::glDisableVertexAttribArrayARB  =(PFNGLDISABLEVERTEXATTRIBARRAYARBPROC  )GetProcAddress((StringPtr)"glDisableVertexAttribArrayARB"  ); if (glDisableVertexAttribArrayARB  ==NULL) return;
    cf::glProgramStringARB             =(PFNGLPROGRAMSTRINGARBPROC             )GetProcAddress((StringPtr)"glProgramStringARB"             ); if (glProgramStringARB             ==NULL) return;
    cf::glBindProgramARB               =(PFNGLBINDPROGRAMARBPROC               )GetProcAddress((StringPtr)"glBindProgramARB"               ); if (glBindProgramARB               ==NULL) return;
    cf::glDeleteProgramsARB            =(PFNGLDELETEPROGRAMSARBPROC            )GetProcAddress((StringPtr)"glDeleteProgramsARB"            ); if (glDeleteProgramsARB            ==NULL) return;
    cf::glGenProgramsARB               =(PFNGLGENPROGRAMSARBPROC               )GetProcAddress((StringPtr)"glGenProgramsARB"               ); if (glGenProgramsARB               ==NULL) return;
    cf::glProgramEnvParameter4dARB     =(PFNGLPROGRAMENVPARAMETER4DARBPROC     )GetProcAddress((StringPtr)"glProgramEnvParameter4dARB"     ); if (glProgramEnvParameter4dARB     ==NULL) return;
    cf::glProgramEnvParameter4dvARB    =(PFNGLPROGRAMENVPARAMETER4DVARBPROC    )GetProcAddress((StringPtr)"glProgramEnvParameter4dvARB"    ); if (glProgramEnvParameter4dvARB    ==NULL) return;
    cf::glProgramEnvParameter4fARB     =(PFNGLPROGRAMENVPARAMETER4FARBPROC     )GetProcAddress((StringPtr)"glProgramEnvParameter4fARB"     ); if (glProgramEnvParameter4fARB     ==NULL) return;
    cf::glProgramEnvParameter4fvARB    =(PFNGLPROGRAMENVPARAMETER4FVARBPROC    )GetProcAddress((StringPtr)"glProgramEnvParameter4fvARB"    ); if (glProgramEnvParameter4fvARB    ==NULL) return;
    cf::glProgramLocalParameter4dARB   =(PFNGLPROGRAMLOCALPARAMETER4DARBPROC   )GetProcAddress((StringPtr)"glProgramLocalParameter4dARB"   ); if (glProgramLocalParameter4dARB   ==NULL) return;
    cf::glProgramLocalParameter4dvARB  =(PFNGLPROGRAMLOCALPARAMETER4DVARBPROC  )GetProcAddress((StringPtr)"glProgramLocalParameter4dvARB"  ); if (glProgramLocalParameter4dvARB  ==NULL) return;
    cf::glProgramLocalParameter4fARB   =(PFNGLPROGRAMLOCALPARAMETER4FARBPROC   )GetProcAddress((StringPtr)"glProgramLocalParameter4fARB"   ); if (glProgramLocalParameter4fARB   ==NULL) return;
    cf::glProgramLocalParameter4fvARB  =(PFNGLPROGRAMLOCALPARAMETER4FVARBPROC  )GetProcAddress((StringPtr)"glProgramLocalParameter4fvARB"  ); if (glProgramLocalParameter4fvARB  ==NULL) return;
    cf::glGetProgramEnvParameterdvARB  =(PFNGLGETPROGRAMENVPARAMETERDVARBPROC  )GetProcAddress((StringPtr)"glGetProgramEnvParameterdvARB"  ); if (glGetProgramEnvParameterdvARB  ==NULL) return;
    cf::glGetProgramEnvParameterfvARB  =(PFNGLGETPROGRAMENVPARAMETERFVARBPROC  )GetProcAddress((StringPtr)"glGetProgramEnvParameterfvARB"  ); if (glGetProgramEnvParameterfvARB  ==NULL) return;
    cf::glGetProgramLocalParameterdvARB=(PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC)GetProcAddress((StringPtr)"glGetProgramLocalParameterdvARB"); if (glGetProgramLocalParameterdvARB==NULL) return;
    cf::glGetProgramLocalParameterfvARB=(PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC)GetProcAddress((StringPtr)"glGetProgramLocalParameterfvARB"); if (glGetProgramLocalParameterfvARB==NULL) return;
    cf::glGetProgramivARB              =(PFNGLGETPROGRAMIVARBPROC              )GetProcAddress((StringPtr)"glGetProgramivARB"              ); if (glGetProgramivARB              ==NULL) return;
    cf::glGetProgramStringARB          =(PFNGLGETPROGRAMSTRINGARBPROC          )GetProcAddress((StringPtr)"glGetProgramStringARB"          ); if (glGetProgramStringARB          ==NULL) return;
    cf::glGetVertexAttribdvARB         =(PFNGLGETVERTEXATTRIBDVARBPROC         )GetProcAddress((StringPtr)"glGetVertexAttribdvARB"         ); if (glGetVertexAttribdvARB         ==NULL) return;
    cf::glGetVertexAttribfvARB         =(PFNGLGETVERTEXATTRIBFVARBPROC         )GetProcAddress((StringPtr)"glGetVertexAttribfvARB"         ); if (glGetVertexAttribfvARB         ==NULL) return;
    cf::glGetVertexAttribivARB         =(PFNGLGETVERTEXATTRIBIVARBPROC         )GetProcAddress((StringPtr)"glGetVertexAttribivARB"         ); if (glGetVertexAttribivARB         ==NULL) return;
    cf::glGetVertexAttribPointervARB   =(PFNGLGETVERTEXATTRIBPOINTERVARBPROC   )GetProcAddress((StringPtr)"glGetVertexAttribPointervARB"   ); if (glGetVertexAttribPointervARB   ==NULL) return;
    cf::glIsProgramARB                 =(PFNGLISPROGRAMARBPROC                 )GetProcAddress((StringPtr)"glIsProgramARB"                 ); if (glIsProgramARB                 ==NULL) return;

    cf::GL_ARB_vertex_and_fragment_program_AVAIL=true;
}
