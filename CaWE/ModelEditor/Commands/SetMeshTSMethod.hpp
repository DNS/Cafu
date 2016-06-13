/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_SET_MESH_TANGENTSPACE_METHOD_HPP_INCLUDED
#define CAFU_MODELEDITOR_SET_MESH_TANGENTSPACE_METHOD_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Models/Model_cmdl.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandSetMeshTSMethodT : public CommandT
    {
        public:

        CommandSetMeshTSMethodT(ModelDocumentT* ModelDoc, unsigned int MeshNr, CafuModelT::MeshT::TangentSpaceMethodT NewTSMethod);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*                              m_ModelDoc;
        const unsigned int                           m_MeshNr;
        const CafuModelT::MeshT::TangentSpaceMethodT m_NewTSMethod;
        const CafuModelT::MeshT::TangentSpaceMethodT m_OldTSMethod;
    };
}

#endif
