/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/********************/
/*** OpenGL State ***/
/********************/

#ifndef CAFU_MATSYS_OPENGLSTATE_HPP_INCLUDED
#define CAFU_MATSYS_OPENGLSTATE_HPP_INCLUDED

// Required for #include <GL/gl.h> with MS VC++.
#if defined(_WIN32) && defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/gl.h>


class DepRelMatrixT;


/// This class encapsulates the OpenGL state, with the intention of minimizing actual OpenGL state changes.
class OpenGLStateT
{
    public:

    void Reset();

    void ActiveTextureUnit(GLenum texUnit);
    void AlphaFunc(GLenum func, GLclampf ref);
    void BlendFunc(GLenum sfactor, GLenum dfactor);
    void DepthFunc(GLenum func);
    void ActiveStencilFace(GLenum mode);
    void StencilFunc(GLenum func, GLint ref, GLuint mask);
    void StencilOp(GLenum fail, GLenum zfail, GLenum zpass);
    void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    void DepthMask(GLboolean flag);
    void CullFace(GLenum mode);
    void FrontFace(GLenum mode);
    void PolygonMode(/*GLenum face=GL_FRONT_AND_BACK,*/ GLenum mode);
    void PolygonOffset(GLfloat factor, GLfloat units);
    void Enable(GLenum cap);
    void Disable(GLenum cap);


    // Matrix related functions.
    enum MatrixModeT { MODELVIEW, PROJECTION, TEXTURE, MATRIX0 };   // Simply use MATRIX0+i for more.

    // Note that this method (in combination with DepRelMatrixTs) subsumes *all* OpenGL matrix functions!
    // (That is, there is *intentionally* no glMatrixMode(), glLoadIdentity(), glTranslate(), glRotate(), ...)
    void LoadMatrix(MatrixModeT MatrixMode, const DepRelMatrixT& Matrix);


    // Functions that only affect the current texture unit.
    void BindTexture(GLenum target, GLuint texture);
    void TexEnv(GLenum target, GLenum pname, GLint    param);
    void TexEnv(GLenum target, GLenum pname, GLfloat  param);
    void TexEnv(GLenum target, GLenum pname, GLint*   param);
    void TexEnv(GLenum target, GLenum pname, GLfloat* param);
 // void TexGen*();
 // void TexImage*();
 // void TexParameter*();


    static const GLenum MeshToOpenGLType[];             ///< Translates from MeshT primitive types to OpenGL rendering primitives.
    static const GLenum WindingToOpenGL[];              ///< Translates from MeshT windings to OpenGL windings (cw or ccw).
    static const GLenum BlendFactorToOpenGL[];          ///< Translates from MaterialT blend factors to OpenGL blend factors.
    static const GLenum PolygonModeToOpenGL[];          ///< Translates from MaterialT::PolygonModeT to OpenGL polygon modes.
    static const GLenum PolygonModeToOpenGL_Offset[];   ///< Translates from MaterialT::PolygonModeT to OpenGL depth offset polygon modes.
    static const GLenum MatrixModeToOpenGL[];           ///< Translates from our MatrixModeT modes to OpenGL matrix modes.

    static OpenGLStateT* GetInstance();


    private:

    struct TexUnitStateT
    {
        GLuint TexObj1D;
        GLuint TexObj2D;
        GLuint TexObj3D;
        GLuint TexObjCube;

        // OpenGL permits to enable multiple texturing targets simultaneously, and uses in such cases the one with the highest dimensionality enabled.
        // See the "OpenGL Programming Guide (Red Book)", page 357 for details.
        // I change the semantic of calls to OpenGLStateT::Enable(GL_TEXTURE_*) such that only one target is ever enabled at any one time.
        bool   Texturing_IsEnabled;
        GLenum Texturing_Target;        ///< One of GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP_ARB.

        bool   IsEnabled_TextureGenQ;
        bool   IsEnabled_TextureGenR;
        bool   IsEnabled_TextureGenS;
        bool   IsEnabled_TextureGenT;
    };

    GLenum        ActiveTexUnit_Unit;
    unsigned long ActiveTexUnit_Index;
    GLenum        AlphaFunc_Func;
    GLclampf      AlphaFunc_Ref;
    GLenum        BlendFunc_sfactor;
    GLenum        BlendFunc_dfactor;
    GLenum        DepthFunc_Func;
    GLenum        ActiveStencilFace_Mode;
    GLenum        StencilFunc_Func[2];
    GLint         StencilFunc_ref[2];
    GLuint        StencilFunc_mask[2];
    GLenum        StencilOp_fail[2];
    GLenum        StencilOp_zfail[2];
    GLenum        StencilOp_zpass[2];
    GLboolean     ColorMask_FlagRed;
    GLboolean     ColorMask_FlagGreen;
    GLboolean     ColorMask_FlagBlue;
    GLboolean     ColorMask_FlagAlpha;
    GLboolean     DepthMask_Flag;
    GLenum        CullFace_mode;
    GLenum        FrontFace_mode;
    GLenum        PolygonMode_mode;
    GLfloat       PolygonOffset_factor;
    GLfloat       PolygonOffset_units;
    bool          IsEnabled_AlphaTest;
    bool          IsEnabled_AutoNormal;
    bool          IsEnabled_Blend;
 // bool          IsEnabled_ClipPlanei;
    bool          IsEnabled_ColorMaterial;
    bool          IsEnabled_CullFace;
    bool          IsEnabled_DepthTest;
    bool          IsEnabled_Dither;
    bool          IsEnabled_Fog;
 // bool          IsEnabled_Lighti;
    bool          IsEnabled_Lighting;
    bool          IsEnabled_LineSmooth;
    bool          IsEnabled_LineStipple;
    bool          IsEnabled_LogicOp;
    bool          IsEnabled_Normalize;
    bool          IsEnabled_PointSmooth;
    bool          IsEnabled_PolygonSmooth;
    bool          IsEnabled_PolygonStipple;
    bool          IsEnabled_PolygonOffsetFill;
    bool          IsEnabled_PolygonOffsetLine;
    bool          IsEnabled_PolygonOffsetPoint;
    bool          IsEnabled_ScissorTest;
    bool          IsEnabled_StencilTest;
    bool          IsEnabled_StencilTestTwoSide;
    unsigned long LoadedMatrixID [3+32];    ///< MatrixID[MM] stores the ID  of the DepRelMatrixT that is bound to (loaded at) the corresponding OpenGL matrix mode.
    unsigned long LoadedMatrixAge[3+32];    ///< MatrixID[MM] stores the age of the DepRelMatrixT that is bound to (loaded at) the corresponding OpenGL matrix mode.
    TexUnitStateT TexUnitStates[8];

    /// The constructor. Private because this is a singleton class.
    OpenGLStateT();
};

#endif
