/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/********************/
/*** OpenGL State ***/
/********************/

#include <stdlib.h>
#include "OpenGLState.hpp"
#include "OpenGLEx.hpp"
#include "DepRelMatrix.hpp"


const GLenum OpenGLStateT::MeshToOpenGLType[]={ GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUADS, GL_QUAD_STRIP, GL_POLYGON };
const GLenum OpenGLStateT::WindingToOpenGL[]={ GL_CW, GL_CCW };
const GLenum OpenGLStateT::BlendFactorToOpenGL[]={ GL_ZERO, GL_ZERO, GL_ONE, GL_DST_COLOR, GL_SRC_COLOR, GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
const GLenum OpenGLStateT::PolygonModeToOpenGL[]={ GL_FILL, GL_LINE, GL_POINT };
const GLenum OpenGLStateT::PolygonModeToOpenGL_Offset[]={ GL_POLYGON_OFFSET_FILL, GL_POLYGON_OFFSET_LINE, GL_POLYGON_OFFSET_POINT };
const GLenum OpenGLStateT::MatrixModeToOpenGL[]={ GL_MODELVIEW, GL_PROJECTION, GL_TEXTURE, GL_MATRIX0_ARB };


OpenGLStateT::OpenGLStateT()
{
    Reset();
}


OpenGLStateT* OpenGLStateT::GetInstance()
{
    static OpenGLStateT* OpenGLState=new OpenGLStateT();

    return OpenGLState;
}


void OpenGLStateT::Reset()
{
    ActiveTexUnit_Unit =GL_TEXTURE0_ARB;
    ActiveTexUnit_Index=0;
    cf::glActiveTextureARB(ActiveTexUnit_Unit);

    AlphaFunc_Func=GL_ALWAYS;
    AlphaFunc_Ref =0;
    glAlphaFunc(AlphaFunc_Func, AlphaFunc_Ref);

    BlendFunc_sfactor=GL_ONE;
    BlendFunc_dfactor=GL_ZERO;
    glBlendFunc(BlendFunc_sfactor, BlendFunc_dfactor);

    DepthFunc_Func=GL_LEQUAL;
    glDepthFunc(DepthFunc_Func);

    if (cf::GL_EXT_stencil_two_side_AVAIL) cf::glActiveStencilFaceEXT(GL_FRONT);
    StencilFunc_Func[0]=GL_ALWAYS;
    StencilFunc_ref [0]=0;
    StencilFunc_mask[0]=~0;
    glStencilFunc(StencilFunc_Func[0], StencilFunc_ref[0], StencilFunc_mask[0]);
    StencilOp_fail [0]=GL_KEEP;
    StencilOp_zfail[0]=GL_KEEP;
    StencilOp_zpass[0]=GL_KEEP;
    glStencilOp(StencilOp_fail[0], StencilOp_zfail[0], StencilOp_zpass[0]);

    if (cf::GL_EXT_stencil_two_side_AVAIL) cf::glActiveStencilFaceEXT(GL_BACK);
    StencilFunc_Func[1]=GL_ALWAYS;
    StencilFunc_ref [1]=0;
    StencilFunc_mask[1]=~0;
    glStencilFunc(StencilFunc_Func[1], StencilFunc_ref[1], StencilFunc_mask[1]);
    StencilOp_fail [1]=GL_KEEP;
    StencilOp_zfail[1]=GL_KEEP;
    StencilOp_zpass[1]=GL_KEEP;
    glStencilOp(StencilOp_fail[1], StencilOp_zfail[1], StencilOp_zpass[1]);

    ActiveStencilFace_Mode=GL_FRONT;
    if (cf::GL_EXT_stencil_two_side_AVAIL) cf::glActiveStencilFaceEXT(ActiveStencilFace_Mode);

    ColorMask_FlagRed  =GL_TRUE;
    ColorMask_FlagGreen=GL_TRUE;
    ColorMask_FlagBlue =GL_TRUE;
    ColorMask_FlagAlpha=GL_TRUE;
    glColorMask(ColorMask_FlagRed, ColorMask_FlagGreen, ColorMask_FlagBlue, ColorMask_FlagAlpha);

    DepthMask_Flag=GL_TRUE;
    glDepthMask(DepthMask_Flag);

    CullFace_mode=GL_BACK;
    glCullFace(CullFace_mode);

    FrontFace_mode=GL_CCW;
    glFrontFace(FrontFace_mode);

    PolygonMode_mode=GL_FILL;
    glPolygonMode(GL_FRONT_AND_BACK, PolygonMode_mode);

    PolygonOffset_factor=0.0;
    PolygonOffset_units =0.0;
    glPolygonOffset(PolygonOffset_factor, PolygonOffset_units);

    IsEnabled_AlphaTest         =false; glDisable(GL_ALPHA_TEST          );
    IsEnabled_AutoNormal        =false; glDisable(GL_AUTO_NORMAL         );
    IsEnabled_Blend             =false; glDisable(GL_BLEND               );
 // IsEnabled_ClipPlanei        =false; glDisable(GL_CLIP_PLANEi         );
    IsEnabled_ColorMaterial     =false; glDisable(GL_COLOR_MATERIAL      );
    IsEnabled_CullFace          =false; glDisable(GL_CULL_FACE           );
    IsEnabled_DepthTest         =true;  glEnable (GL_DEPTH_TEST          );
    IsEnabled_Dither            =false; glDisable(GL_DITHER              );
    IsEnabled_Fog               =false; glDisable(GL_FOG                 );
 // IsEnabled_Lighti            =false; glDisable(GL_LIGHTi              );
    IsEnabled_Lighting          =false; glDisable(GL_LIGHTING            );
    IsEnabled_LineSmooth        =false; glDisable(GL_LINE_SMOOTH         );
    IsEnabled_LineStipple       =false; glDisable(GL_LINE_STIPPLE        );
    IsEnabled_LogicOp           =false; glDisable(GL_LOGIC_OP            );
    IsEnabled_Normalize         =false; glDisable(GL_NORMALIZE           );
    IsEnabled_PointSmooth       =false; glDisable(GL_POINT_SMOOTH        );
    IsEnabled_PolygonSmooth     =false; glDisable(GL_POLYGON_SMOOTH      );
    IsEnabled_PolygonStipple    =false; glDisable(GL_POLYGON_STIPPLE     );
    IsEnabled_PolygonOffsetFill =false; glDisable(GL_POLYGON_OFFSET_FILL );
    IsEnabled_PolygonOffsetLine =false; glDisable(GL_POLYGON_OFFSET_LINE );
    IsEnabled_PolygonOffsetPoint=false; glDisable(GL_POLYGON_OFFSET_POINT);
    IsEnabled_ScissorTest       =false; glDisable(GL_SCISSOR_TEST        );
    IsEnabled_StencilTest       =false; glDisable(GL_STENCIL_TEST        );
    IsEnabled_StencilTestTwoSide=false; if (cf::GL_EXT_stencil_two_side_AVAIL) glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);


    // Reset the matrices.
    for (unsigned long MM=0; MM<3+32; MM++)
    {
        LoadedMatrixID [MM]=0xFFFFFFFF;
        LoadedMatrixAge[MM]=0;
    }
    // Intentionally don't pre-load any matrices here, because the GL_PROJECTION matrix might have been
    // set by the OpenGL window code, which is external to the MatSys, opened before us, and has probably loaded a matrix already.
    // It also isn't necessary to preload anything -- the above values guarantee that the next call to LoadMatrix() will always succeed.


    // Reset the texture units.
    GLint MaxTexUnits;
    glGetIntegerv(cf::GL_ARB_vertex_and_fragment_program_AVAIL ? GL_MAX_TEXTURE_IMAGE_UNITS_ARB : GL_MAX_TEXTURE_UNITS_ARB, &MaxTexUnits);

    // if (MaxTexUnits>8) Error("MaxTexUnits==%lu > 8.\n", MaxTexUnits);

    for (unsigned long TUNr=0; TUNr<(unsigned long)MaxTexUnits && TUNr<8; TUNr++)
    {
        TexUnitStateT& TU=TexUnitStates[TUNr];

        ActiveTextureUnit(GL_TEXTURE0_ARB+TUNr);

        TU.TexObj1D  =0; glBindTexture(GL_TEXTURE_1D,           0);
        TU.TexObj2D  =0; glBindTexture(GL_TEXTURE_2D,           0);
        TU.TexObj3D  =0; glBindTexture(GL_TEXTURE_3D,           0);
        TU.TexObjCube=0; glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);

        glDisable(GL_TEXTURE_1D          );
        glDisable(GL_TEXTURE_2D          );
        glDisable(GL_TEXTURE_3D          );
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);

        TU.Texturing_IsEnabled=false;
        TU.Texturing_Target   =GL_TEXTURE_2D;   // Arbitrary choice.

        TU.IsEnabled_TextureGenQ=false; glDisable(GL_TEXTURE_GEN_Q);
        TU.IsEnabled_TextureGenR=false; glDisable(GL_TEXTURE_GEN_R);
        TU.IsEnabled_TextureGenS=false; glDisable(GL_TEXTURE_GEN_S);
        TU.IsEnabled_TextureGenT=false; glDisable(GL_TEXTURE_GEN_T);
    }

    ActiveTextureUnit(GL_TEXTURE0_ARB);
}


void OpenGLStateT::ActiveTextureUnit(GLenum texUnit)
{
    if (ActiveTexUnit_Unit==texUnit) return;

    ActiveTexUnit_Unit =texUnit;
    ActiveTexUnit_Index=ActiveTexUnit_Unit-GL_TEXTURE0_ARB;

    cf::glActiveTextureARB(ActiveTexUnit_Unit);
}


void OpenGLStateT::AlphaFunc(GLenum func, GLclampf ref)
{
    if (AlphaFunc_Func==func && AlphaFunc_Ref==ref) return;

    AlphaFunc_Func=func;
    AlphaFunc_Ref =ref;

    glAlphaFunc(AlphaFunc_Func, AlphaFunc_Ref);
}


void OpenGLStateT::BlendFunc(GLenum sfactor, GLenum dfactor)
{
    if (BlendFunc_sfactor==sfactor && BlendFunc_dfactor==dfactor) return;

    BlendFunc_sfactor=sfactor;
    BlendFunc_dfactor=dfactor;

    glBlendFunc(BlendFunc_sfactor, BlendFunc_dfactor);
}


void OpenGLStateT::DepthFunc(GLenum func)
{
    if (DepthFunc_Func==func) return;

    DepthFunc_Func=func;

    glDepthFunc(DepthFunc_Func);
}


void OpenGLStateT::ActiveStencilFace(GLenum mode)
{
    if (ActiveStencilFace_Mode==mode) return;

    ActiveStencilFace_Mode=mode;

    if (!cf::GL_EXT_stencil_two_side_AVAIL) return;
    cf::glActiveStencilFaceEXT(ActiveStencilFace_Mode);
}


void OpenGLStateT::StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    const unsigned long Index=(ActiveStencilFace_Mode==GL_FRONT) ? 0 : 1;

    if (StencilFunc_Func[Index]==func && StencilFunc_ref[Index]==ref && StencilFunc_mask[Index]==mask) return;

    StencilFunc_Func[Index]=func;
    StencilFunc_ref [Index]=ref;
    StencilFunc_mask[Index]=mask;

    glStencilFunc(StencilFunc_Func[Index], StencilFunc_ref[Index], StencilFunc_mask[Index]);
}


void OpenGLStateT::StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    const unsigned long Index=(ActiveStencilFace_Mode==GL_FRONT) ? 0 : 1;

    if (StencilOp_fail[Index]==fail && StencilOp_zfail[Index]==zfail && StencilOp_zpass[Index]==zpass) return;

    StencilOp_fail [Index]=fail;
    StencilOp_zfail[Index]=zfail;
    StencilOp_zpass[Index]=zpass;

    glStencilOp(StencilOp_fail[Index], StencilOp_zfail[Index], StencilOp_zpass[Index]);
}


void OpenGLStateT::ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    if (ColorMask_FlagRed==red && ColorMask_FlagGreen==green && ColorMask_FlagBlue==blue && ColorMask_FlagAlpha==alpha) return;

    ColorMask_FlagRed  =red;
    ColorMask_FlagGreen=green;
    ColorMask_FlagBlue =blue;
    ColorMask_FlagAlpha=alpha;

    glColorMask(ColorMask_FlagRed, ColorMask_FlagGreen, ColorMask_FlagBlue, ColorMask_FlagAlpha);
}


void OpenGLStateT::DepthMask(GLboolean flag)
{
    if (DepthMask_Flag==flag) return;

    DepthMask_Flag=flag;

    glDepthMask(DepthMask_Flag);
}


void OpenGLStateT::CullFace(GLenum mode)
{
    if (CullFace_mode==mode) return;

    CullFace_mode=mode;

    glCullFace(CullFace_mode);
}


void OpenGLStateT::FrontFace(GLenum mode)
{
    if (FrontFace_mode==mode) return;

    FrontFace_mode=mode;

    glFrontFace(FrontFace_mode);
}


void OpenGLStateT::PolygonMode(GLenum mode)
{
    if (PolygonMode_mode==mode) return;

    PolygonMode_mode=mode;

    glPolygonMode(GL_FRONT_AND_BACK, PolygonMode_mode);
}


void OpenGLStateT::PolygonOffset(GLfloat factor, GLfloat units)
{
    if (PolygonOffset_factor==factor && PolygonOffset_units==units) return;

    PolygonOffset_factor=factor;
    PolygonOffset_units =units;

    glPolygonOffset(PolygonOffset_factor, PolygonOffset_units);
}


void OpenGLStateT::Enable(GLenum cap)
{
    switch (cap)
    {
        case GL_ALPHA_TEST:                if (IsEnabled_AlphaTest         ) return; IsEnabled_AlphaTest         =true; break;
        case GL_AUTO_NORMAL:               if (IsEnabled_AutoNormal        ) return; IsEnabled_AutoNormal        =true; break;
        case GL_BLEND:                     if (IsEnabled_Blend             ) return; IsEnabled_Blend             =true; break;
     // case GL_CLIP_PLANEi:               if (IsEnabled_ClipPlanei        ) return; IsEnabled_ClipPlanei        =true; break;
        case GL_COLOR_MATERIAL:            if (IsEnabled_ColorMaterial     ) return; IsEnabled_ColorMaterial     =true; break;
        case GL_CULL_FACE:                 if (IsEnabled_CullFace          ) return; IsEnabled_CullFace          =true; break;
        case GL_DEPTH_TEST:                if (IsEnabled_DepthTest         ) return; IsEnabled_DepthTest         =true; break;
        case GL_DITHER:                    if (IsEnabled_Dither            ) return; IsEnabled_Dither            =true; break;
        case GL_FOG:                       if (IsEnabled_Fog               ) return; IsEnabled_Fog               =true; break;
     // case GL_LIGHTi:                    if (IsEnabled_Lighti            ) return; IsEnabled_Lighti            =true; break;
        case GL_LIGHTING:                  if (IsEnabled_Lighting          ) return; IsEnabled_Lighting          =true; break;
        case GL_LINE_SMOOTH:               if (IsEnabled_LineSmooth        ) return; IsEnabled_LineSmooth        =true; break;
        case GL_LINE_STIPPLE:              if (IsEnabled_LineStipple       ) return; IsEnabled_LineStipple       =true; break;
        case GL_LOGIC_OP:                  if (IsEnabled_LogicOp           ) return; IsEnabled_LogicOp           =true; break;
        case GL_NORMALIZE:                 if (IsEnabled_Normalize         ) return; IsEnabled_Normalize         =true; break;
        case GL_POINT_SMOOTH:              if (IsEnabled_PointSmooth       ) return; IsEnabled_PointSmooth       =true; break;
        case GL_POLYGON_SMOOTH:            if (IsEnabled_PolygonSmooth     ) return; IsEnabled_PolygonSmooth     =true; break;
        case GL_POLYGON_STIPPLE:           if (IsEnabled_PolygonStipple    ) return; IsEnabled_PolygonStipple    =true; break;
        case GL_POLYGON_OFFSET_FILL:       if (IsEnabled_PolygonOffsetFill ) return; IsEnabled_PolygonOffsetFill =true; break;
        case GL_POLYGON_OFFSET_LINE:       if (IsEnabled_PolygonOffsetLine ) return; IsEnabled_PolygonOffsetLine =true; break;
        case GL_POLYGON_OFFSET_POINT:      if (IsEnabled_PolygonOffsetPoint) return; IsEnabled_PolygonOffsetPoint=true; break;
        case GL_SCISSOR_TEST:              if (IsEnabled_ScissorTest       ) return; IsEnabled_ScissorTest       =true; break;
        case GL_STENCIL_TEST:              if (IsEnabled_StencilTest       ) return; IsEnabled_StencilTest       =true; break;
        case GL_STENCIL_TEST_TWO_SIDE_EXT: if (IsEnabled_StencilTestTwoSide) return; IsEnabled_StencilTestTwoSide=true; if (!cf::GL_EXT_stencil_two_side_AVAIL) return; break;
        case GL_TEXTURE_GEN_Q:             if (TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenQ) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenQ=true; break;
        case GL_TEXTURE_GEN_R:             if (TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenR) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenR=true; break;
        case GL_TEXTURE_GEN_S:             if (TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenS) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenS=true; break;
        case GL_TEXTURE_GEN_T:             if (TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenT) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenT=true; break;

        case GL_TEXTURE_1D: // Intentional fall-through!
        case GL_TEXTURE_2D: // Intentional fall-through!
        case GL_TEXTURE_3D: // Intentional fall-through!
        case GL_TEXTURE_CUBE_MAP_ARB:
            if (TexUnitStates[ActiveTexUnit_Index].Texturing_IsEnabled)
            {
                // The desired target is already enabled. Do nothing.
                if (TexUnitStates[ActiveTexUnit_Index].Texturing_Target==cap) return;

                // Another target is enabled. Disable it.
                glDisable(TexUnitStates[ActiveTexUnit_Index].Texturing_Target);
            }

            // Enable the desired target.
            TexUnitStates[ActiveTexUnit_Index].Texturing_IsEnabled=true;
            TexUnitStates[ActiveTexUnit_Index].Texturing_Target   =cap;
            break;

     // default: Warning("Unknown cap %lu (0x%X).\n", cap, cap);
    }

    glEnable(cap);
}


void OpenGLStateT::Disable(GLenum cap)
{
    switch (cap)
    {
        case GL_ALPHA_TEST:                if (!IsEnabled_AlphaTest         ) return; IsEnabled_AlphaTest         =false; break;
        case GL_AUTO_NORMAL:               if (!IsEnabled_AutoNormal        ) return; IsEnabled_AutoNormal        =false; break;
        case GL_BLEND:                     if (!IsEnabled_Blend             ) return; IsEnabled_Blend             =false; break;
     // case GL_CLIP_PLANEi:               if (!IsEnabled_ClipPlanei        ) return; IsEnabled_ClipPlanei        =false; break;
        case GL_COLOR_MATERIAL:            if (!IsEnabled_ColorMaterial     ) return; IsEnabled_ColorMaterial     =false; break;
        case GL_CULL_FACE:                 if (!IsEnabled_CullFace          ) return; IsEnabled_CullFace          =false; break;
        case GL_DEPTH_TEST:                if (!IsEnabled_DepthTest         ) return; IsEnabled_DepthTest         =false; break;
        case GL_DITHER:                    if (!IsEnabled_Dither            ) return; IsEnabled_Dither            =false; break;
        case GL_FOG:                       if (!IsEnabled_Fog               ) return; IsEnabled_Fog               =false; break;
     // case GL_LIGHTi:                    if (!IsEnabled_Lighti            ) return; IsEnabled_Lighti            =false; break;
        case GL_LIGHTING:                  if (!IsEnabled_Lighting          ) return; IsEnabled_Lighting          =false; break;
        case GL_LINE_SMOOTH:               if (!IsEnabled_LineSmooth        ) return; IsEnabled_LineSmooth        =false; break;
        case GL_LINE_STIPPLE:              if (!IsEnabled_LineStipple       ) return; IsEnabled_LineStipple       =false; break;
        case GL_LOGIC_OP:                  if (!IsEnabled_LogicOp           ) return; IsEnabled_LogicOp           =false; break;
        case GL_NORMALIZE:                 if (!IsEnabled_Normalize         ) return; IsEnabled_Normalize         =false; break;
        case GL_POINT_SMOOTH:              if (!IsEnabled_PointSmooth       ) return; IsEnabled_PointSmooth       =false; break;
        case GL_POLYGON_SMOOTH:            if (!IsEnabled_PolygonSmooth     ) return; IsEnabled_PolygonSmooth     =false; break;
        case GL_POLYGON_STIPPLE:           if (!IsEnabled_PolygonStipple    ) return; IsEnabled_PolygonStipple    =false; break;
        case GL_POLYGON_OFFSET_FILL:       if (!IsEnabled_PolygonOffsetFill ) return; IsEnabled_PolygonOffsetFill =false; break;
        case GL_POLYGON_OFFSET_LINE:       if (!IsEnabled_PolygonOffsetLine ) return; IsEnabled_PolygonOffsetLine =false; break;
        case GL_POLYGON_OFFSET_POINT:      if (!IsEnabled_PolygonOffsetPoint) return; IsEnabled_PolygonOffsetPoint=false; break;
        case GL_SCISSOR_TEST:              if (!IsEnabled_ScissorTest       ) return; IsEnabled_ScissorTest       =false; break;
        case GL_STENCIL_TEST:              if (!IsEnabled_StencilTest       ) return; IsEnabled_StencilTest       =false; break;
        case GL_STENCIL_TEST_TWO_SIDE_EXT: if (!IsEnabled_StencilTestTwoSide) return; IsEnabled_StencilTestTwoSide=false; if (!cf::GL_EXT_stencil_two_side_AVAIL) return; break;
        case GL_TEXTURE_GEN_Q:             if (!TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenQ) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenQ=false; break;
        case GL_TEXTURE_GEN_R:             if (!TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenR) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenR=false; break;
        case GL_TEXTURE_GEN_S:             if (!TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenS) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenS=false; break;
        case GL_TEXTURE_GEN_T:             if (!TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenT) return; TexUnitStates[ActiveTexUnit_Index].IsEnabled_TextureGenT=false; break;

        case GL_TEXTURE_1D: // Intentional fall-through!
        case GL_TEXTURE_2D: // Intentional fall-through!
        case GL_TEXTURE_3D: // Intentional fall-through!
        case GL_TEXTURE_CUBE_MAP_ARB:
            // Nothing enabled? So there is nothing to disable.
            if (!TexUnitStates[ActiveTexUnit_Index].Texturing_IsEnabled) return;

            // Something is enabled, but they want to disable something else? (That in turn is already disabled anyway!) Don't.
            if (cap!=TexUnitStates[ActiveTexUnit_Index].Texturing_Target) return;

            // Disable the desired target.
            TexUnitStates[ActiveTexUnit_Index].Texturing_IsEnabled=false;
            break;

     // default: Warning("Unknown cap %lu (0x%X).\n", cap, cap);
    }

    glDisable(cap);
}


/// This method loads a matrix.
/// @param MatrixMode   The matrix mode for which the matrix should be loaded (MODELVIEW, PROJECTION, TEXTURE, MATRIX0, ...).
/// @param Matrix       The matrix that is to be loaded. The matrix must be up-to-date already, i.e. Matrix.Update() must have been called by the caller!
/// Note that individual matrices are discriminated by their ID(!), not by their actual content value.
/// Also, the age of a matrix should increase whenever the matrix is modified.
void OpenGLStateT::LoadMatrix(MatrixModeT MatrixMode, const DepRelMatrixT& Matrix)
{
    if (Matrix.ID!=LoadedMatrixID[MatrixMode] || Matrix.Age>LoadedMatrixAge[MatrixMode])
    {
        glMatrixMode(MatrixMode<MATRIX0 ? MatrixModeToOpenGL[MatrixMode] : GL_MATRIX0_ARB+(MatrixMode-MATRIX0));
        glLoadMatrixf(&Matrix.Matrix.GetTranspose().m[0][0]);

        LoadedMatrixID [MatrixMode]=Matrix.ID;
        LoadedMatrixAge[MatrixMode]=Matrix.Age;
    }
}


void OpenGLStateT::BindTexture(GLenum target, GLuint texture)
{
    switch (target)
    {
        case GL_TEXTURE_1D:           if (TexUnitStates[ActiveTexUnit_Index].TexObj1D  ==texture) return; TexUnitStates[ActiveTexUnit_Index].TexObj1D  =texture; break;
        case GL_TEXTURE_2D:           if (TexUnitStates[ActiveTexUnit_Index].TexObj2D  ==texture) return; TexUnitStates[ActiveTexUnit_Index].TexObj2D  =texture; break;
        case GL_TEXTURE_3D:           if (TexUnitStates[ActiveTexUnit_Index].TexObj3D  ==texture) return; TexUnitStates[ActiveTexUnit_Index].TexObj3D  =texture; break;
        case GL_TEXTURE_CUBE_MAP_ARB: if (TexUnitStates[ActiveTexUnit_Index].TexObjCube==texture) return; TexUnitStates[ActiveTexUnit_Index].TexObjCube=texture; break;

        // default: Warning("Unknown target %lu (0x%X).\n", target, target);
    }

    glBindTexture(target, texture);
}


void OpenGLStateT::TexEnv(GLenum target, GLenum pname, GLint    param)
{
    glTexEnvi(target, pname, param);
}


void OpenGLStateT::TexEnv(GLenum target, GLenum pname, GLfloat  param)
{
    glTexEnvf(target, pname, param);
}


void OpenGLStateT::TexEnv(GLenum target, GLenum pname, GLint*   param)
{
    glTexEnviv(target, pname, param);
}


void OpenGLStateT::TexEnv(GLenum target, GLenum pname, GLfloat* param)
{
    glTexEnvfv(target, pname, param);
}
