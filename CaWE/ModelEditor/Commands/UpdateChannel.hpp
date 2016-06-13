/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_UPDATE_CHANNEL_HPP_INCLUDED
#define CAFU_MODELEDITOR_UPDATE_CHANNEL_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Models/Model_cmdl.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandUpdateChannelT : public CommandT
    {
        public:

        CommandUpdateChannelT(ModelDocumentT* ModelDoc, unsigned int ChannelNr, unsigned int JointNr, bool IsMember);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ModelDocumentT*    m_ModelDoc;
        const unsigned int m_ChannelNr;
        const unsigned int m_JointNr;
        const bool         m_NewIsMember;
        const bool         m_OldIsMember;
    };
}

#endif
