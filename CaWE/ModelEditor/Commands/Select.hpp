/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_COMMAND_SELECT_HPP_INCLUDED
#define CAFU_MODELEDITOR_COMMAND_SELECT_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "../ModelDocument.hpp"


namespace ModelEditor
{
    class CommandSelectT : public CommandT
    {
        public:

        // Named constructors for easier command creation.
        static CommandSelectT* Clear (ModelDocumentT* ModelDoc, ModelElementTypeT Type);
        static CommandSelectT* Add   (ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Elems);
        static CommandSelectT* Add   (ModelDocumentT* ModelDoc, ModelElementTypeT Type, unsigned int Elem);
        static CommandSelectT* Remove(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Elems);
        static CommandSelectT* Remove(ModelDocumentT* ModelDoc, ModelElementTypeT Type, unsigned int Elem);
        static CommandSelectT* Set   (ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& Elems);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        /// Only named constructors can create a CommandSelectT.
        CommandSelectT(ModelDocumentT* ModelDoc, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel);

        ModelDocumentT*            m_ModelDoc;
        const ModelElementTypeT    m_Type;
        const ArrayT<unsigned int> m_OldSel;
        const ArrayT<unsigned int> m_NewSel;
    };
}

#endif
