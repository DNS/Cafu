/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "Loader_Collada.hpp"
#include "COLLADABUURI.h"
#include "COLLADAFWIWriter.h"
#include "COLLADAFWRoot.h"
#include "COLLADASaxFWLLoader.h"


class CafuMdlWriterT : public COLLADAFW::IWriter
{
    public:

    CafuMdlWriterT(const COLLADABU::URI& outputFile);
    virtual ~CafuMdlWriterT() { }


    /** Deletes the entire scene.
    @param errorMessage A message containing informations about the error that occurred.*/
    void cancel(const std::string& errorMessage);

    /** Prepare to receive data.*/
    void start();

    /** Remove all objects that don't have an object. Deletes unused visual scenes.*/
    void finish();

    /** When this method is called, the writer must write the global document asset.
    @return The writer should return true, if writing succeeded, false otherwise.*/
    virtual bool writeGlobalAsset(const COLLADAFW::FileInfo* asset);

    /** Writes the entire visual scene.
    @return True on succeeded, false otherwise.*/
    virtual bool writeVisualScene(const COLLADAFW::VisualScene* visualScene);

    /** Writes the scene.
    @return True on succeeded, false otherwise.*/
    virtual bool writeScene(const COLLADAFW::Scene* scene);

    /** Handles all nodes in the library nodes.
    @return True on succeeded, false otherwise.*/
    virtual bool writeLibraryNodes(const COLLADAFW::LibraryNodes* libraryNodes);

    /** Writes the geometry.
    @return True on succeeded, false otherwise.*/
    virtual bool writeGeometry(const COLLADAFW::Geometry* geometry);

    /** Writes the material.
    @return True on succeeded, false otherwise.*/
    virtual bool writeMaterial(const COLLADAFW::Material* material);

    /** Writes the effect.
    @return True on succeeded, false otherwise.*/
    virtual bool writeEffect(const COLLADAFW::Effect* effect);

    /** Writes the camera.
    @return True on succeeded, false otherwise.*/
    virtual bool writeCamera(const COLLADAFW::Camera* camera);

    /** Writes the image.
    @return True on succeeded, false otherwise.*/
    virtual bool writeImage(const COLLADAFW::Image* image);

    /** Writes the light.
    @return True on succeeded, false otherwise.*/
    virtual bool writeLight(const COLLADAFW::Light* light);

    /** Writes the animation.
    @return True on succeeded, false otherwise.*/
    virtual bool writeAnimation(const COLLADAFW::Animation* animation) { return true; }

    /** Writes the animation.
    @return True on succeeded, false otherwise.*/
    virtual bool writeAnimationList(const COLLADAFW::AnimationList* animationList) { return true; }

    /** Writes the skin controller data.
    @return True on succeeded, false otherwise.*/
    virtual bool writeSkinControllerData(const COLLADAFW::SkinControllerData* skinControllerData) { return true; }

    /** Writes the controller.
    @return True on succeeded, false otherwise.*/
    virtual bool writeController(const COLLADAFW::Controller* Controller) { return true; }

    /** When this method is called, the writer must write the formulas. All the formulas of the entire
    COLLADA file are contained in @a formulas.
    @return The writer should return true, if writing succeeded, false otherwise.*/
    virtual bool writeFormulas(const COLLADAFW::Formulas* formulas) { return true; }

    /** When this method is called, the writer must write the kinematics scene.
    @return The writer should return true, if writing succeeded, false otherwise.*/
    virtual bool writeKinematicsScene(const COLLADAFW::KinematicsScene* kinematicsScene) { return true; }


    private:

    CafuMdlWriterT(const CafuMdlWriterT&);          ///< Use of the Copy Constructor    is not allowed.
    void operator = (const CafuMdlWriterT&);        ///< Use of the Assignment Operator is not allowed.

    COLLADABU::URI m_OutputFile;
    std::ofstream  OutStream;
};


CafuMdlWriterT::CafuMdlWriterT(const COLLADABU::URI& OutputFile)
    : m_OutputFile(OutputFile),
      OutStream("collada-out.log")
{
}


void CafuMdlWriterT::cancel(const std::string& errorMessage)
{
    OutStream << __FUNCTION__ << "\n";
}


void CafuMdlWriterT::start()
{
    OutStream << __FUNCTION__ << "\n";
}


void CafuMdlWriterT::finish()
{
    OutStream << __FUNCTION__ << "\n";
}


bool CafuMdlWriterT::writeGlobalAsset(const COLLADAFW::FileInfo* asset)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeVisualScene(const COLLADAFW::VisualScene* visualScene)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeScene(const COLLADAFW::Scene* scene)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeLibraryNodes(const COLLADAFW::LibraryNodes* libraryNodes)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeGeometry(const COLLADAFW::Geometry* geometry)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeMaterial(const COLLADAFW::Material* material)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeEffect(const COLLADAFW::Effect* effect)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeCamera(const COLLADAFW::Camera* camera)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeImage(const COLLADAFW::Image* image)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


bool CafuMdlWriterT::writeLight(const COLLADAFW::Light* light)
{
    OutStream << __FUNCTION__ << "\n";
    return true;
}


/**********************/
/*** LoaderColladaT ***/
/**********************/

LoaderColladaT::LoaderColladaT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : ModelLoaderT(FileName)
{
    COLLADASaxFWL::Loader ColladaLoader;
    CafuMdlWriterT        CafuMdlWriter(COLLADABU::URI::nativePathToUri(FileName+"_out"));
    COLLADAFW::Root       root(&ColladaLoader, &CafuMdlWriter);

    if (!root.loadDocument(FileName))
    {
        throw LoadErrorT("Could not load the Collada document.");
    }
}


bool LoaderColladaT::UseGivenTS() const
{
    // TODO...!
    return false;
}


void LoaderColladaT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
}


void LoaderColladaT::Load(ArrayT<CafuModelT::GuiLocT>& GuiLocs)
{
}
