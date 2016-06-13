/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************/
/*** Shader ***/
/**************/

#ifndef CAFU_MATSYS_SHADER_HPP_INCLUDED
#define CAFU_MATSYS_SHADER_HPP_INCLUDED

#include <string>


template<class T> class ArrayT;
namespace MatSys
{
    class MeshT;
    class RenderMaterialT;
}
class MaterialT;


/// This class represents a shader. A shader works together with a material for rendering a chunk of geometry with that material.
/// However, a shader does not handle the rendering itself, it only sets up the appropriate API state.
class ShaderT
{
    protected:

    /// The constructor registers this shader at the global shader repository.
    /// It is protected so that only derived classes can be instantiated.
    /// TODO: Make the default ctor private, and e.g. ShaderT(std::string Name_) protected?
    ShaderT();

    /// Mark the destructor as being virtual (g++ 4.x raises a warning otherwise).
    virtual ~ShaderT() { }


    public:

    /// Returns the name of this shader.
    /// IMPLEMENTORS: Shader names must be unique and renderer-independent, e.g. "MyCarPaintMetallicBlue1"
    /// (such that Materials that specify a specific Shader are also renderer-independent).
    virtual const std::string& GetName() const=0;

    /// Returns if and how well this shader can handle the ambient parts of the Material (fully, limited, or not at all).
    /// Returns   0 if this shader cannot handle the Material at all.
    /// Returns 255 if this shader is sure that it can fully handle the Material at maxmimum quality.
    /// Returns a number between 1 and 254 if this shader provides limited handling of the Material.
    /// The higher the number, the better the provided quality and the less the limitations.
    /// The advantage over simple true/false statements is that not every shader needs detail knowledge about every other shader in this renderer.
    virtual char CanHandleAmbient(const MaterialT& Material) const=0;

    /// Returns if and how well this shader can handle the per-lightsource parts of the Material (fully, limited, or not at all).
    /// Returns   0 if this shader cannot handle the Material at all.
    /// Returns 255 if this shader is sure that it can fully handle the Material at maxmimum quality.
    /// Returns a number between 1 and 254 if this shader provides limited handling of the Material.
    /// The higher the number, the better the provided quality and the less the limitations.
    /// The advantage over simple true/false statements is that not every shader needs detail knowledge about every other shader in this renderer.
    virtual char CanHandleLighting(const MaterialT& Material) const=0;

    /// Returns if this shader can handle the rendering of stencil shadow volumes.
    /// There should only be at most one such shader in each renderer.
    virtual bool CanHandleStencilShadowVolumes() const=0;


    /// This function activates this shader.
    virtual void Activate()=0;

    /// This function deactivates this shader.
    virtual void Deactivate()=0;

    /// The caller can use these functions in order to learn what attributes and parameters it has to pass-in
    /// with the mesh / mesh vertices for the currently bound / activated Material.
    virtual bool NeedsNormals() const=0;
    virtual bool NeedsTangentSpace() const=0;
    virtual bool NeedsXYAttrib() const=0;

    /// Renders the Mesh, using the renderers currently bound material.
    virtual void RenderMesh(const MatSys::MeshT& Mesh)=0;


    // Returns the number of passes that this shader needs to render its effect.
    // CanHandleAmbient(Material) or CanHandleLighting(Material) should previously have returned true for the Material.
    // virtual unsigned long GetNrOfPasses(const RenderMaterialT& Material) const;

    // This function activates this shader in combination with the Material.
    // CanHandleAmbient(Material) or CanHandleLighting(Material) should previously have returned true for the Material.
    // virtual void ActivatePass(unsigned long PassNr, const RenderMaterialT& Material) const;

    // A shader needs a way to determine global rendering parameters: E.g. the currently active lightsource params (pos, color, ...),
    // Eye Pos, ...   This is easiest achieved by providing it with ("circular") access to the renderer, which in turn provides an interface
    // for this purpose.
};


/// This returns the global shader repository, where all shaders of a renderer register themselves.
ArrayT<ShaderT*>& GetShaderRepository();

/// This returns the generic shadow volume shader for rendering stencil shadows.
/// NULL may be returned if no such shader exists.
ShaderT* GetStencilShadowVolumesShader();

#endif
