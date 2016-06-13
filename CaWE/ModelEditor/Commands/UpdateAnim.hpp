/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_UPDATE_ANIM_HPP_INCLUDED
#define CAFU_MODELEDITOR_UPDATE_ANIM_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Models/Model_cmdl.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandUpdateAnimT : public CommandT
    {
        public:

        CommandUpdateAnimT(ModelDocumentT* ModelDoc, unsigned int AnimNr, const CafuModelT::AnimT& Anim);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*         m_ModelDoc;
        const unsigned int      m_AnimNr;
        const CafuModelT::AnimT m_NewAnim;
        const CafuModelT::AnimT m_OldAnim;
    };
}

#endif
