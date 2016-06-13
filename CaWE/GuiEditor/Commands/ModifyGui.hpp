/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_COMMAND_MODIFY_GUI_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMMAND_MODIFY_GUI_HPP_INCLUDED

#include "../../CommandPattern.hpp"


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandModifyGuiT : public CommandT
    {
        public:

        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, const wxString& NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, int             NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, long            NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, float           NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, bool            NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, const wxColour& NewValue);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        CommandModifyGuiT(GuiDocumentT* GuiDocument, const wxString& PropertyName);

        GuiDocumentT*        m_GuiDocument;
        wxString             m_PropertyName;

        wxString             m_NewString;
        int                  m_NewInt;
        long                 m_NewLong;
        float                m_NewFloat;
        bool                 m_NewBool;
        wxColour             m_NewColor;

        wxString             m_OldString;
        int                  m_OldInt;
        long                 m_OldLong;
        float                m_OldFloat;
        bool                 m_OldBool;
        wxColour             m_OldColor;
    };
}

#endif
