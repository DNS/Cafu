/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_COMMAND_RENAME_HPP_INCLUDED
#define CAFU_MODELEDITOR_COMMAND_RENAME_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "../ModelDocument.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandRenameT : public CommandT
    {
        public:

        CommandRenameT(ModelDocumentT* ModelDoc, ModelElementTypeT Type, unsigned int ElemNr, const wxString& NewName);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        std::string& GetStringRef() const;
        void UpdateAllObservers() const;

        ModelDocumentT*         m_ModelDoc;
        const ModelElementTypeT m_Type;
        const unsigned int      m_ElemNr;
        const std::string       m_NewName;
        const std::string       m_OldName;
    };
}

#endif
