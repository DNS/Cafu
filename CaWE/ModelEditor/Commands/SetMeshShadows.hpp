/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_SET_MESH_SHADOWS_HPP_INCLUDED
#define CAFU_MODELEDITOR_SET_MESH_SHADOWS_HPP_INCLUDED

#include "../../CommandPattern.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandSetMeshShadowsT : public CommandT
    {
        public:

        CommandSetMeshShadowsT(ModelDocumentT* ModelDoc, unsigned int MeshNr, bool NewCastShadows);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*    m_ModelDoc;
        const unsigned int m_MeshNr;
        const bool         m_NewCastShadows;
        const bool         m_OldCastShadows;
    };
}

#endif
