/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_SET_ANIM_FPS_HPP_INCLUDED
#define CAFU_MODELEDITOR_SET_ANIM_FPS_HPP_INCLUDED

#include "../../CommandPattern.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandSetAnimFPST : public CommandT
    {
        public:

        CommandSetAnimFPST(ModelDocumentT* ModelDoc, unsigned int AnimNr, float NewFPS);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*    m_ModelDoc;
        const unsigned int m_AnimNr;
        const float        m_NewFPS;
        const float        m_OldFPS;
    };
}

#endif
