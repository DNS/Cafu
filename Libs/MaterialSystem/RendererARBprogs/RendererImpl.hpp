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
#include "Templates/Array.hpp"
#include "Math3D/Matrix.hpp"
#include "../Common/DepRelMatrix.hpp"
#include "../Expression.hpp"


class ShaderT;
class TextureMap2DT;
class TextureMapCubeT;


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


    // Internal Interface
    MatSys::RenderMaterialT* GetCurrentRenderMaterial() const;
    TextureMap2DT*           GetCurrentLightMap() const;
    TextureMap2DT*           GetCurrentLightDirMap() const;
    TextureMap2DT*           GetCurrentSHLMap(unsigned long Index) const;
    TextureMap2DT*           GetCurrentSHLLookupMap() const;
    TextureMapCubeT*         GetNormalizationCubeMap() const;
    TextureMap2DT*           GetHelperGrayMap() const;
    TextureMap2DT*           GetHelperBlackMap() const;
    TextureMap2DT*           GetHelperLightBlue001Map() const;
    unsigned long            GetInitCounter() const;

    const DepRelMatrixT& GetDepRelMatrix(MatrixNameT MN) const;
    const DepRelMatrixT& GetDepRelMatrixInv(MatrixNameT MN) const;
    const DepRelMatrixT& GetDepRelMatrixModelView() const;

    const ExpressionT::SymbolsT& GetExpressionSymbols() const { return ExpressionSymbols; }

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

    /// Based on the CurrentRenderAction and the CurrentRenderMaterial, set the CurrentShader.
    /// Called from SetCurrentRenderAction() and SetCurrentMaterial().
    void SetCurrentShader();

 // std::string              BaseDir;
    bool                     IsInitialized;
    RenderActionT            CurrentRenderAction;
    MatSys::RenderMaterialT* CurrentRenderMaterial;
    bool                     LockCurrentRM;
    ShaderT*                 CurrentShader;
    TextureMap2DT*           CurrentLightMap;
    TextureMap2DT*           CurrentLightDirMap;
    TextureMap2DT*           CurrentSHLMaps[16];
    TextureMap2DT*           CurrentSHLLookupMap;
    LightingParamsT          CurrentLightingParams;
    ArrayT<LightingParamsT>  LightingParamsStack;
    TextureMapCubeT*         NormalizationCubeMap;
 // TextureMap2DT*           HelperWhiteMap;
    TextureMap2DT*           HelperGrayMap;
    TextureMap2DT*           HelperBlackMap;
    TextureMap2DT*           HelperLightBlue001Map;
    unsigned long            InitCounter;
    unsigned long            ShaderChangeCounter;


    // The original source/parent matrices. Each matrix has an associated age, and they grow older whenever they are modified.
    DepRelMatrixT   Matrix     [END_MARKER];    ///< The current MODEL_TO_WORLD, WORLD_TO_VIEW and PROJECITON matrices. All other matrices eventually depend on these.
    ArrayT<MatrixT> MatrixStack[END_MARKER];    ///< The storage for the matrix stack.

    // Dependent matrices. They all depend in some way or another on the elements of the Matrix array above.
    // A dependent matrix is updated whenever its source matrix (or one of its source matrices) has grown older
    // than it was when the dependent matrix was last created.
    mutable InverseMatrixT MatrixInv[END_MARKER];   ///< The inverses of the above matrices. MatrixInv[MN] depends on Matrix[MN].
    mutable ProductMatrixT MatrixModelView;         ///< The model-to-view matrix. It depends on the Matrix[MODEL_TO_WORLD] and Matrix[WORLD_TO_VIEW] matrices.

    /// This is where the symbol values for material expressions are stored.
    ExpressionT::SymbolsT ExpressionSymbols;


    /// Private constructor for the Singleton pattern.
    /// The destructor is only private on accident.
    RendererImplT();

#if defined(_WIN32) && defined(_MSC_VER)
    public:
#endif
    ~RendererImplT();
};

#endif
