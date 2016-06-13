/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_UPDATE_TRIANGLE_HPP_INCLUDED
#define CAFU_MODELEDITOR_UPDATE_TRIANGLE_HPP_INCLUDED

#include "../../CommandPattern.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandUpdateTriangleT : public CommandT
    {
        public:

        CommandUpdateTriangleT(ModelDocumentT* ModelDoc, unsigned int MeshNr, unsigned int TriNr, bool SkipDraw);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*    m_ModelDoc;
        const unsigned int m_MeshNr;
        const unsigned int m_TriNr;
        const bool         m_NewSkipDraw;
        const bool         m_OldSkipDraw;
    };
}

#endif
