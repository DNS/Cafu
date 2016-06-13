/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/****************/
/*** Renderer ***/
/****************/

#ifndef CAFU_MATSYS_RENDERER_HPP_INCLUDED
#define CAFU_MATSYS_RENDERER_HPP_INCLUDED



template<class T> class ArrayT;
class MaterialT;
template<class T> class Matrix4x4T;
typedef Matrix4x4T<float> Matrix4x4fT;


namespace MatSys
{
    class MeshT;
    class RenderMaterialT;
    class TextureMapI;


    /// This class provides an interface to a renderer.
    /// The interface is specified as ABC in order to share the renderer across exe/dll boundaries.
    class RendererI
    {
        public:

        enum RenderActionT { AMBIENT, STENCILSHADOW, LIGHTING };


        /// Returns true if this renderer is supported on this system (e.g. required OpenGL extensions are present,
        /// version of DirectX matches, ...), and returns false otherwise.
        /// This function must not be called before the underlying video environment (i.e. the OpenGL rendering context) has been initialized.
        /// No preceeding call to Initialize() is required, and the environment is left as clean as it was found,
        /// such that no subsequent actions (i.e. calls to this function of *other* renderers) get into trouble.
        virtual bool IsSupported() const=0;

        /// Returns true if this renderer supports SHL rendering with compressed (indexed) SHLMaps, false otherwise.
        virtual bool DoesSupportCompressedSHL() const=0;

        /// Returns true if this renderer supports SHL rendering with uncompressed SHLMaps (four coefficients per SHLMap), false otherwise.
        virtual bool DoesSupportUncompressedSHL() const=0;

        /// Returns a "global" preference number for this renderer.
        /// This helps the caller code the select the "best" or "most preferred" renderer among many.
        virtual int GetPreferenceNr() const=0;

        /// Call this after a successful call to IsSupported() in order to prepare the renderer for use.
        virtual void Initialize()=0;

        /// Call this in order to release this renderer.
        virtual void Release()=0;

        /// Returns a description of this renderer, e.g. for informing the users about the
        /// underlying technique (OpenGL, DirectX, ...), the capabilities, or details about the implementation.
        virtual const char* GetDescription() const=0;

        // Tells the renderer where the base directory is.
        // The texture file names of the materials are specified relative to this base dir.
        // OBSOLETE virtual void SetBaseDir(const std::string& BaseDir_)=0;

        // This simply returns the previously set base directory name.
        // OBSOLETE virtual const std::string& GetBaseDir()=0;

        /// Registers the Material with the renderer, and returns a handle for future reference.
        /// NULL can be passed for Material, in which case a "null" handle will be returned, which is nonetheless valid
        /// (but just won't cause any rendering output later).
        virtual MatSys::RenderMaterialT* RegisterMaterial(const MaterialT* Material) const=0;

        /// Returns the MaterialT object that was used in RegisterMaterial() to create the RenderMaterial.
        virtual const MaterialT* GetMaterialFromRM(MatSys::RenderMaterialT* RenderMaterial) const=0;

        /// Returns the ID of the ambient shader of the RenderMaterial.
        /// This number can be used by the calling code in order to adjust the draw order (for the AMBIENT render action).
        /// It is guaranteed that the returned value is reasonably small, that is, not greater than the number of shaders that are employed in this renderer.
        virtual unsigned long GetAmbientShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const=0;

        /// Returns the ID of the light shader of the RenderMaterial.
        /// This number can be used by the calling code in order to adjust the draw order (for the LIGHTING render action).
        /// It is guaranteed that the returned value is reasonably small, that is, not greater than the number of shaders that are employed in this renderer.
        virtual unsigned long GetLightShaderIDFromRM(MatSys::RenderMaterialT* RenderMaterial) const=0;

        /// Used to free the previously registered (render-)material.
        /// If RenderMaterial happens to be the currently used material, SetCurrentMaterial(NULL) is automatically called before RenderMaterial is deleted.
        virtual void FreeMaterial(MatSys::RenderMaterialT* RenderMaterial)=0;

        /// Call this to begin rendering a new frame. This function typically clears the screen and calls other
        /// implementation specific functions to start a new frame.
        /// Time is the global time in seconds at which this frame starts.
        virtual void BeginFrame(double Time)=0;

        /// Call this to end the current frame. This function typically calls functions like SwapBuffers() and other
        /// implementation specific functions to end a frame.
        virtual void EndFrame()=0;

        /// This function pre-caches all textures of all materials that have been registered with RegisterMaterial() so far.
        /// As a result, the delay that otherwise occurs when new textures are first used for rendering is minimized.
        virtual void PreCache()=0;

        /// This activates a specific render action, as is required for multi-pass rendering with dynamic lights and stencil shadows.
        /// Rendering geometry will only consider those parts of the materials that are relevant for their currently set action.
        virtual void SetCurrentRenderAction(RenderActionT RA)=0;

        /// Returns the currently set render action.
        virtual RenderActionT GetCurrentRenderAction() const=0;

        // These methods set/get general-purpose rendering parameters.
        //
        // The Set... methods are normally called from entity code, before the entity draws itself.
        // The entity in turn may have the values from its map file description, i.e. from CaWE, where the user set them.
        // Or an entity script may set them, or of course the engine or MOD (entity) C++ code.
        // For example, a weapon could set the percentage of its remaining ammo, or anything else that reflects its current state.
        //
        // These parameters are then available in the cmat material script files, where they can be used in expressions.
        // This means that the cmat script can for example specify how the color of a material changes depending on some weapons remaining ammo level!
        // Another example are toggle-switches that can change their color (e.g. red/green) depending on their state.
        //
        // How does the writer of the cmap script know the meaning of a rendering parameter?  Answer: By agreement/arrangement (Vereinbarung/Absprache)!
        // He knows that he's writing a script for a material that is supposed to be on a certain entity, e.g. a weapon.
        // The weapon coder then tells him that rendering parameter 7 is set to the remaining relative ammo level, and the cmap writer can then use it.
        //
        // The cmap script in turn may hand over these *rendering* parameters as *shader* parameters to the shader by using the "shaderExprParam" keyword.
        // The meaning of the *shader* parameters does ALSO rely on a previous agreement/arrangement, but this time with the shader coder!
        // That in turn means that the full control over a shader (== ShaderT, e.g. a vertex+fragment program) can be *anywhere* between those who
        // provide the rendering parameter values and those who write the cmap script! E.g. for a shader parameter ("shaderExprParam ..."),
        // you can pass-in a constant value, a rendering parameter, or any combination between them that can be expressed by an expression.
        //
        // Index can currently be between 0 and 31 (inclusive).
        virtual void  SetGenPurposeRenderingParam (unsigned long Index, float Value)=0;
        virtual void  SetGenPurposeRenderingParam (unsigned long Index, int   Value)=0;
        virtual float GetGenPurposeRenderingParamF(unsigned long Index) const=0;
        virtual int   GetGenPurposeRenderingParamI(unsigned long Index) const=0;

        // Sets/gets the ambient light color that is used from material script rgb expressions.
        virtual void         SetCurrentAmbientLightColor(float r, float g, float b)=0;
        virtual float*       GetCurrentAmbientLightColor()=0;                     // Can also be used for writing.
        virtual const float* GetCurrentAmbientLightColor() const=0;

        // This sets the current light source that is to be used with RenderActions STENCILSHADOW and LIGHTING.
        // Please note that the Position and Radius must be given in MODEL space!
        virtual void         SetCurrentLightSourcePosition(float x, float y, float z)=0;
        virtual float*       GetCurrentLightSourcePosition()=0;                   // Can also be used for writing.
        virtual const float* GetCurrentLightSourcePosition() const=0;
        virtual void         SetCurrentLightSourceRadius(float r)=0;
        virtual float&       GetCurrentLightSourceRadius()=0;                     // Can also be used for writing.
        virtual const float& GetCurrentLightSourceRadius() const=0;
        virtual void         SetCurrentLightSourceDiffuseColor (float r, float g, float b)=0;
        virtual float*       GetCurrentLightSourceDiffuseColor()=0;               // Can also be used for writing.
        virtual const float* GetCurrentLightSourceDiffuseColor() const=0;
        virtual void         SetCurrentLightSourceSpecularColor(float r, float g, float b)=0;
        virtual float*       GetCurrentLightSourceSpecularColor()=0;              // Can also be used for writing.
        virtual const float* GetCurrentLightSourceSpecularColor() const=0;

        // This sets the current eye position in MODEL space(!) that is used in some lighting shaders (e.g. for specular highlights).
        virtual void         SetCurrentEyePosition(float x, float y, float z)=0;
        virtual float*       GetCurrentEyePosition()=0;                           // Can also be used for writing.
        virtual const float* GetCurrentEyePosition() const=0;

        /// Pushes all lighting parameters onto a stack. The depth of the stack is only limited by available memory.
        /// The following parameters are covered:
        /// Light source position and radius, light source diffuse and specular color, and the eye position.
        /// The ambient light color is currently NOT covered.
        virtual void PushLightingParameters()=0;

        /// Pops (restores) the lighting parameters that have previously been pushed onto the stack.
        /// Nothing happens if the stack is empty.
        virtual void PopLightingParameters()=0;


        /******************************************/
        /*** Interface for output window issues ***/
        /******************************************/

        virtual void ClearColor(float r, float g, float b, float a)=0;
        virtual void Flush()=0;


        /***********************************************/
        /*** Interface for dealing with the matrices ***/
        /***********************************************/

        enum MatrixNameT { MODEL_TO_WORLD, WORLD_TO_VIEW, PROJECTION /*VIEW_TO_CLIP*/ /*, TEXTURE*/, END_MARKER };

        // Methods for getting the native and dependent matrices.
        virtual const Matrix4x4fT& GetMatrix(MatrixNameT MN) const=0;
        virtual const Matrix4x4fT& GetMatrixInv(MatrixNameT MN) const=0;
        virtual const Matrix4x4fT& GetMatrixModelView() const=0;

        // Methods for modifying the matrices.
        virtual void SetMatrix(MatrixNameT MN, const Matrix4x4fT& Matrix)=0;
        virtual void Translate(MatrixNameT MN, float x, float y, float z)=0;    // Convenience method, equivalent to SetMatrix(GetMatrix()*T);
        virtual void Scale    (MatrixNameT MN, float scale)=0;
        virtual void RotateX  (MatrixNameT MN, float angle)=0;
        virtual void RotateY  (MatrixNameT MN, float angle)=0;
        virtual void RotateZ  (MatrixNameT MN, float angle)=0;

        // Methods for pushing the matrices on and popping them off a stack.
        virtual void PushMatrix(MatrixNameT MN)=0;
        virtual void PopMatrix (MatrixNameT MN)=0;


        // Viewport functions.
        virtual void SetViewport(int x, int y, int width, int height)=0;
        virtual void GetViewport(int viewport[4])=0;

        // Selection, Picking, and Feedback.
        enum PickingRenderModeT { RENDER, SELECT, FEEDBACK };

        virtual void SetSelectionBuffer(unsigned long Size, unsigned int* Buffer)=0;
        virtual unsigned long SetPickingRenderMode(PickingRenderModeT PRM)=0;
        virtual void InitNameStack()=0;
        virtual void LoadName(unsigned long Name)=0;
        virtual void PushName(unsigned long Name)=0;
        virtual void PopName()=0;


        /**************************************************************/
        /*** 1st interface for handing in geometry (immediate mode) ***/
        /**************************************************************/

        virtual void SetCurrentMaterial(MatSys::RenderMaterialT* RenderMaterial)=0;
        virtual MatSys::RenderMaterialT* GetCurrentMaterial() const=0;
        virtual void LockCurrentMaterial(bool LockCM_)=0;

        virtual void SetCurrentLightMap(TextureMapI* LightMap)=0;
        virtual void SetCurrentLightDirMap(TextureMapI* LightDirMap)=0;

        virtual void SetCurrentSHLMaps(const ArrayT<TextureMapI*>& SHLMaps)=0;  // Each SHLMap stores 4 coefficients (uncompressed SHL) or there is one index map (compressed SHL).
        virtual void SetCurrentSHLLookupMap(TextureMapI* SHLLookupMap)=0;

        /* virtual void BeginGeom(type)=0;
        virtual void VertexAttrib1(...)=0;    // e.g. TexCoord
        virtual void VertexAttrib2(...)=0;    // e.g. LMCoord
        virtual void Vertex(...)=0;
        virtual void EndGeom()=0; */

    /*  // SOME OLD STUFF - REMOVE!

        // ?????????????????????????????????????
        virtual void BeginShadowSilhouetteGeom()=0;
        virtual void Vertex(...)=0;
        virtual void EndShadowSilhouetteGeom()=0;  */

        virtual void RenderMesh(const MatSys::MeshT& Mesh)=0;

        /// This ABC does neither have nor need a destructor, because no implementation will ever be deleted via a pointer to a RendererI.
        /// (The implementations are singletons after all.)  See the Singleton pattern and the C++ FAQ 21.05 (the "precise rule") for more information.
        /// g++ however issues a warning with no such destructor, so I provide one anyway and am safe.
        virtual ~RendererI() { }
    };


    /// A global pointer to the current renderer, for common access by all modules that use the MatSys.
    /// Just set this after you loaded the desired renderer DLL to the pointer returned by the DLLs GetRenderer() function.
    /// (And NULL it on unloading the DLL.)
    /// An analogous object exists for the TextureMapManager interface, see TextureMap.hpp.
    extern RendererI* Renderer;
}

#endif
