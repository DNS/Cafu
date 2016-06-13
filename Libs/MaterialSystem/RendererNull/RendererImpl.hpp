/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*******************************/
/*** Renderer Implementation ***/
/*******************************/

#ifndef CAFU_MATSYS_RENDERER_IMPLEMENTATION_HPP_INCLUDED
#define CAFU_MATSYS_RENDERER_IMPLEMENTATION_HPP_INCLUDED

#include "../Renderer.hpp"
#include "../Expression.hpp"
#include "Templates/Array.hpp"
#include "Math3D/Matrix.hpp"


class RendererImplT : public MatSys::RendererI
{
    public:

    // RendererI implementation.
    bool IsSupported() const;
    bool DoesSupportCompressedSHL() const;
    bool DoesSupportUncompressedSHL() const;
    int GetPreferenceNr() const;
    void Initialize();
    void Release();
    const char* GetDescription() const;
    // OBSOLETE void SetBaseDir(const std::string& BaseDir_);
    // OBSOLETE const std::string& GetBaseDir();
    MatSys::RenderMaterialT* RegisterMaterial(const MaterialT* Material) const;
    const MaterialT* GetMaterialFromRM(MatSys::RenderMaterialT* RenderMaterial) const;
    unsigned long GetAmbientShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const;
    unsigned long GetLightShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const;
    void FreeMaterial(MatSys::RenderMaterialT* RenderMaterial);
    void BeginFrame(double Time);
    void EndFrame();
    void PreCache();
    void SetCurrentRenderAction(RenderActionT RA);
    RenderActionT GetCurrentRenderAction() const;

    void  SetGenPurposeRenderingParam (unsigned long Index, float Value);
    void  SetGenPurposeRenderingParam (unsigned long Index, int   Value);
    float GetGenPurposeRenderingParamF(unsigned long Index) const;
    int   GetGenPurposeRenderingParamI(unsigned long Index) const;

    void         SetCurrentAmbientLightColor(float r, float g, float b);
    float*       GetCurrentAmbientLightColor();
    const float* GetCurrentAmbientLightColor() const;

    void         SetCurrentLightSourcePosition(float x, float y, float z);
    float*       GetCurrentLightSourcePosition();
    const float* GetCurrentLightSourcePosition() const;
    void         SetCurrentLightSourceRadius(float r);
    float&       GetCurrentLightSourceRadius();
    const float& GetCurrentLightSourceRadius() const;
    void         SetCurrentLightSourceDiffuseColor (float r, float g, float b);
    float*       GetCurrentLightSourceDiffuseColor();
    const float* GetCurrentLightSourceDiffuseColor() const;
    void         SetCurrentLightSourceSpecularColor(float r, float g, float b);
    float*       GetCurrentLightSourceSpecularColor();
    const float* GetCurrentLightSourceSpecularColor() const;

    void         SetCurrentEyePosition(float x, float y, float z);
    float*       GetCurrentEyePosition();
    const float* GetCurrentEyePosition() const;

    void PushLightingParameters();
    void PopLightingParameters();

    void ClearColor(float r, float g, float b, float a);
    void Flush();

    const MatrixT& GetMatrix(MatrixNameT MN) const;
    const MatrixT& GetMatrixInv(MatrixNameT MN) const;
    const MatrixT& GetMatrixModelView() const;

    void SetMatrix(MatrixNameT MN, const MatrixT& Matrix);
    void Translate(MatrixNameT MN, float x, float y, float z);
    void Scale    (MatrixNameT MN, float scale);
    void RotateX  (MatrixNameT MN, float angle);
    void RotateY  (MatrixNameT MN, float angle);
    void RotateZ  (MatrixNameT MN, float angle);

    void PushMatrix(MatrixNameT MN);
    void PopMatrix (MatrixNameT MN);

    void SetViewport(int x, int y, int width, int height);
    void GetViewport(int viewport[4]);

    void SetSelectionBuffer(unsigned long Size, unsigned int* Buffer);
    unsigned long SetPickingRenderMode(PickingRenderModeT PRM);
    void InitNameStack();
    void LoadName(unsigned long Name);
    void PushName(unsigned long Name);
    void PopName();

    void SetCurrentMaterial(MatSys::RenderMaterialT* RenderMaterial);
    MatSys::RenderMaterialT* GetCurrentMaterial() const;
    void LockCurrentMaterial(bool LockCM);
    void SetCurrentLightMap(MatSys::TextureMapI* LightMap);
    void SetCurrentLightDirMap(MatSys::TextureMapI* LightDirMap);
    void SetCurrentSHLMaps(const ArrayT<MatSys::TextureMapI*>& SHLMaps);
    void SetCurrentSHLLookupMap(MatSys::TextureMapI* SHLLookupMap);
    void RenderMesh(const MatSys::MeshT& Mesh);


    // Internal interface.
    MatSys::RenderMaterialT* GetCurrentRenderMaterial() const;

    static RendererImplT& GetInstance();


    private:

    struct LightingParamsT
    {
        float LightSourcePosition[3];
        float LightSourceRadius;
        float LightSourceDiffuseColor[3];
        float LightSourceSpecularColor[3];
        float EyePosition[3];
    };

 // std::string              BaseDir;
    RenderActionT            CurrentRenderAction;
    LightingParamsT          CurrentLightingParams;
    ArrayT<LightingParamsT>  LightingParamsStack;

    /// This is where the symbol values for material expressions are stored.
    ExpressionT::SymbolsT ExpressionSymbols;


    // Private constructor for the Singleton pattern.
    RendererImplT();
};

#endif
