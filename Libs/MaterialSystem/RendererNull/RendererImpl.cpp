/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

/*******************************/
/*** Renderer Implementation ***/
/*******************************/

#include <stdlib.h>

#include "RendererImpl.hpp"


using namespace MatSys;


RendererImplT& RendererImplT::GetInstance()
{
    static RendererImplT Renderer;

    return Renderer;
}


RendererImplT::RendererImplT()
    : CurrentRenderAction(AMBIENT)
{
    ExpressionSymbols.GenFloat.PushBackEmpty(32);
    ExpressionSymbols.GenInt  .PushBackEmpty(32);
}


bool RendererImplT::IsSupported() const
{
    return true;
}


bool RendererImplT::DoesSupportCompressedSHL() const
{
    return false;
}


bool RendererImplT::DoesSupportUncompressedSHL() const
{
    return false;
}


int RendererImplT::GetPreferenceNr() const
{
    return 1;
}


void RendererImplT::Initialize()
{
}


void RendererImplT::Release()
{
}


const char* RendererImplT::GetDescription() const
{
    return "The Null Renderer";
}


RenderMaterialT* RendererImplT::RegisterMaterial(const MaterialT* Material) const
{
    return NULL;
}


const MaterialT* RendererImplT::GetMaterialFromRM(MatSys::RenderMaterialT* RenderMaterial) const
{
    return NULL;
}


unsigned long RendererImplT::GetAmbientShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const
{
    return 0;
}


unsigned long RendererImplT::GetLightShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const
{
    return 0;
}


void RendererImplT::FreeMaterial(RenderMaterialT* RenderMaterial)
{
}


void RendererImplT::BeginFrame(double Time)
{
}


void RendererImplT::EndFrame()
{
}


void RendererImplT::PreCache()
{
}


void RendererImplT::SetCurrentRenderAction(RenderActionT RA)
{
    CurrentRenderAction=RA;
}


RendererImplT::RenderActionT RendererImplT::GetCurrentRenderAction() const
{
    return CurrentRenderAction;
}


void RendererImplT::SetGenPurposeRenderingParam (unsigned long Index, float Value)
{
    if (Index>=ExpressionSymbols.GenFloat.Size()) return;

    ExpressionSymbols.GenFloat[Index]=Value;
}


void RendererImplT::SetGenPurposeRenderingParam (unsigned long Index, int Value)
{
    if (Index>=ExpressionSymbols.GenInt.Size()) return;

    ExpressionSymbols.GenInt[Index]=Value;
}


float RendererImplT::GetGenPurposeRenderingParamF(unsigned long Index) const
{
    if (Index>=ExpressionSymbols.GenFloat.Size()) return 0.0;

    return ExpressionSymbols.GenFloat[Index];
}


int RendererImplT::GetGenPurposeRenderingParamI(unsigned long Index) const
{
    if (Index>=ExpressionSymbols.GenInt.Size()) return 0;

    return ExpressionSymbols.GenInt[Index];
}


void RendererImplT::SetCurrentAmbientLightColor(float r, float g, float b)
{
    ExpressionSymbols.AmbientLightColor[0]=r;
    ExpressionSymbols.AmbientLightColor[1]=g;
    ExpressionSymbols.AmbientLightColor[2]=b;
}


float* RendererImplT::GetCurrentAmbientLightColor()
{
    return ExpressionSymbols.AmbientLightColor;
}


const float* RendererImplT::GetCurrentAmbientLightColor() const
{
    return ExpressionSymbols.AmbientLightColor;
}


void RendererImplT::SetCurrentLightSourcePosition(float x, float y, float z)
{
    CurrentLightingParams.LightSourcePosition[0]=x;
    CurrentLightingParams.LightSourcePosition[1]=y;
    CurrentLightingParams.LightSourcePosition[2]=z;
}


float* RendererImplT::GetCurrentLightSourcePosition()
{
    return CurrentLightingParams.LightSourcePosition;
}


const float* RendererImplT::GetCurrentLightSourcePosition() const
{
    return CurrentLightingParams.LightSourcePosition;
}


void RendererImplT::SetCurrentLightSourceRadius(float r)
{
    CurrentLightingParams.LightSourceRadius=r;
}


float& RendererImplT::GetCurrentLightSourceRadius()
{
    return CurrentLightingParams.LightSourceRadius;
}


const float& RendererImplT::GetCurrentLightSourceRadius() const
{
    return CurrentLightingParams.LightSourceRadius;
}


void RendererImplT::SetCurrentLightSourceDiffuseColor(float r, float g, float b)
{
    CurrentLightingParams.LightSourceDiffuseColor[0]=r;
    CurrentLightingParams.LightSourceDiffuseColor[1]=g;
    CurrentLightingParams.LightSourceDiffuseColor[2]=b;
}


float* RendererImplT::GetCurrentLightSourceDiffuseColor()
{
    return CurrentLightingParams.LightSourceDiffuseColor;
}


const float* RendererImplT::GetCurrentLightSourceDiffuseColor() const
{
    return CurrentLightingParams.LightSourceDiffuseColor;
}


void RendererImplT::SetCurrentLightSourceSpecularColor(float r, float g, float b)
{
    CurrentLightingParams.LightSourceSpecularColor[0]=r;
    CurrentLightingParams.LightSourceSpecularColor[1]=g;
    CurrentLightingParams.LightSourceSpecularColor[2]=b;
}


float* RendererImplT::GetCurrentLightSourceSpecularColor()
{
    return CurrentLightingParams.LightSourceSpecularColor;
}


const float* RendererImplT::GetCurrentLightSourceSpecularColor() const
{
    return CurrentLightingParams.LightSourceSpecularColor;
}


void RendererImplT::SetCurrentEyePosition(float x, float y, float z)
{
    CurrentLightingParams.EyePosition[0]=x;
    CurrentLightingParams.EyePosition[1]=y;
    CurrentLightingParams.EyePosition[2]=z;
}


float* RendererImplT::GetCurrentEyePosition()
{
    return CurrentLightingParams.EyePosition;
}


const float* RendererImplT::GetCurrentEyePosition() const
{
    return CurrentLightingParams.EyePosition;
}


void RendererImplT::PushLightingParameters()
{
    LightingParamsStack.PushBack(CurrentLightingParams);
}


void RendererImplT::PopLightingParameters()
{
    if (LightingParamsStack.Size()>0)
    {
        CurrentLightingParams=LightingParamsStack[LightingParamsStack.Size()-1];
        LightingParamsStack.DeleteBack();
    }
}


void RendererImplT::ClearColor(float r, float g, float b, float a)
{
}


void RendererImplT::Flush()
{
}


/**************************************************************************************************************/
/*** BEGIN of matrix-related code.                                                                          ***/
/**************************************************************************************************************/

static const MatrixT Identity;

const MatrixT& RendererImplT::GetMatrix(MatrixNameT MN) const
{
    return Identity;
}


const MatrixT& RendererImplT::GetMatrixInv(MatrixNameT MN) const
{
    return Identity;
}


const MatrixT& RendererImplT::GetMatrixModelView() const
{
    return Identity;
}


void RendererImplT::SetMatrix(MatrixNameT MN, const MatrixT& Matrix_)
{
}


void RendererImplT::Translate(MatrixNameT MN, float x, float y, float z)
{
}


void RendererImplT::Scale(MatrixNameT MN, float scale)
{
}


void RendererImplT::RotateX(MatrixNameT MN, float angle)
{
}


void RendererImplT::RotateY(MatrixNameT MN, float angle)
{
}


void RendererImplT::RotateZ(MatrixNameT MN, float angle)
{
}


void RendererImplT::PushMatrix(MatrixNameT MN)
{
}


void RendererImplT::PopMatrix(MatrixNameT MN)
{
}

/************************************************************************************************************/
/*** END of matrix-related code.                                                                          ***/
/************************************************************************************************************/


void RendererImplT::SetViewport(int x, int y, int width, int height)
{
}


void RendererImplT::GetViewport(int viewport[4])
{
    viewport[0]=0;
    viewport[1]=0;
    viewport[2]=400;
    viewport[3]=300;
}


void RendererImplT::SetSelectionBuffer(unsigned long Size, unsigned int* Buffer)
{
}


unsigned long RendererImplT::SetPickingRenderMode(PickingRenderModeT PRM)
{
    return 0;
}


void RendererImplT::InitNameStack()
{
}


void RendererImplT::LoadName(unsigned long Name)
{
}


void RendererImplT::PushName(unsigned long Name)
{
}


void RendererImplT::PopName()
{
}


void RendererImplT::SetCurrentMaterial(RenderMaterialT* RenderMaterial)
{
}


MatSys::RenderMaterialT* RendererImplT::GetCurrentMaterial() const
{
    return GetCurrentRenderMaterial();
}


void RendererImplT::LockCurrentMaterial(bool LockCM)
{
}


void RendererImplT::SetCurrentLightMap(TextureMapI* LightMap)
{
}


void RendererImplT::SetCurrentLightDirMap(TextureMapI* LightDirMap)
{
}


void RendererImplT::SetCurrentSHLMaps(const ArrayT<TextureMapI*>& SHLMaps)
{
}


void RendererImplT::SetCurrentSHLLookupMap(TextureMapI* SHLLookupMap)
{
}


void RendererImplT::RenderMesh(const MeshT& Mesh)
{
}


RenderMaterialT* RendererImplT::GetCurrentRenderMaterial() const
{
    return NULL;
}
