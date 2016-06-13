/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_SET_ANIM_NEXT_HPP_INCLUDED
#define CAFU_MODELEDITOR_SET_ANIM_NEXT_HPP_INCLUDED

#include "../../CommandPattern.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandSetAnimNextT : public CommandT
    {
        public:

        CommandSetAnimNextT(ModelDocumentT* ModelDoc, unsigned int AnimNr, int NewNext);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*    m_ModelDoc;
        const unsigned int m_AnimNr;
        const int          m_NewNext;
        const int          m_OldNext;
    };
}

#endif
