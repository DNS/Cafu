/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODEL_LOADER_HPP_INCLUDED
#define CAFU_MODEL_LOADER_HPP_INCLUDED

#include "Model_cmdl.hpp"


/// The base class for importing arbitrary model files into Cafu models.
/// The concrete derived classes determine which specific file format is imported.
class ModelLoaderT
{
    public:

    class UserCallbacksI;
    class LoadErrorT;

    enum FlagsT
    {
        NONE                  =0x00,
        REMOVE_DEGEN_TRIANGLES=0x01,
        REMOVE_UNUSED_VERTICES=0x02,
        REMOVE_UNUSED_WEIGHTS =0x04
    };


    /// The constructor.
    ModelLoaderT(const std::string& FileName, int Flags);

    /// The virtual destructor.
    virtual ~ModelLoaderT() { }

    /// Returns the file name of the imported model.
    /// This method is reimplemented in the \c LoaderDlodT class.
    virtual const std::string& GetFileName() const { return m_FileName; }

    /// Actually loads the file data into the appropriate parts of the Cafu model.
    virtual void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)=0;

    /// Loads the skins of the Cafu model.
    virtual void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)=0;

    /// Loads the GUI fixtures of the Cafu model.
    virtual void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)=0;

    /// Loads the animation channels (groups of joints) of the Cafu model.
    virtual void Load(ArrayT<CafuModelT::ChannelT>& Channels)=0;

    /// Loads the dlod-model and dlod-distance at the given level.
    virtual bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist)=0;

    /// Postprocesses the file data according to flags given to the constructor.
    virtual void Postprocess(ArrayT<CafuModelT::MeshT>& Meshes);


    protected:

    /// Removes triangles with zero-length edges from the given mesh.
    /// This is especially important because such triangles "connect" two vertices that the CafuModelT code
    /// considers as "geometrical duplicates" of each other. That is, a single triangle refers to the same
    /// vertex coordinate twice, which triggers related assertions in debug builds.
    void RemoveDegenTriangles(CafuModelT::MeshT& Mesh);

    /// Removes unused vertices from the given mesh.
    void RemoveUnusedVertices(CafuModelT::MeshT& Mesh);

    /// Makes sure that vertices that are geo-dups of each other refer to the same set of weights.
    void AbandonDuplicateWeights(CafuModelT::MeshT& Mesh);

    /// Removes unused weights from the given mesh (should be called after RemoveUnusedVertices()).
    void RemoveUnusedWeights(CafuModelT::MeshT& Mesh);

    /// Creates and returns a fail-safe wire-frame material with the given name,
    /// for use when a material with more detailed or more specific settings is not available.
    MaterialT CreateDefaultMaterial(const std::string& MatName, bool EditorSave=true) const;

    const std::string m_FileName;
    const int         m_Flags;
};


/// An interface for user callbacks.
/// A concrete model loader may require e.g. asking the user for a password to open the file.
/// The calling code can implement this interface and pass it to the concrete loader in order to achieve the desired functionality.
class ModelLoaderT::UserCallbacksI
{
    public:

    /// Asks the user for a password to open the model file.
    /// @returns The entered password, or the empty string for cancel/none.
    virtual std::string GetPasswordFromUser(const std::string& Message, const std::string& Caption="Enter password")=0;

    // /// Returns an output stream for any log messages that the loader wants to present to the user.
    // std::ostream& GetLog()=0;

    // /// Tells the implementation how much of the model has already been loaded.
    // void SetProgress(float Percent)=0;
};


/// A class that is thrown on model load errors.
class ModelLoaderT::LoadErrorT : public std::runtime_error
{
    public:

    LoadErrorT(const std::string& Message);
};


/// The base class for importing additional animations into an existing CafuModelT.
class AnimImporterT
{
    public:

    /// The constructor.
    AnimImporterT(const std::string& FileName);

    /// Returns the name of the file the animations are imported from.
    virtual const std::string& GetFileName() const { return m_FileName; }

    /// Imports and returns the animation sequences from the file, optionally referring to the joints and meshes of the related model.
    /// It is up to the caller to actually add the imported sequences to the related model instance.
    virtual ArrayT<CafuModelT::AnimT> Import(const ArrayT<CafuModelT::JointT>& Joints, const ArrayT<CafuModelT::MeshT>& Meshes)=0;


    protected:

    const std::string m_FileName;
};

#endif
