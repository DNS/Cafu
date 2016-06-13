/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_GUI_DOCUMENT_HPP_INCLUDED
#define CAFU_GUIEDITOR_GUI_DOCUMENT_HPP_INCLUDED

#include "CompSelection.hpp"
#include "ObserverPattern.hpp"
#include "../DocumentAdapter.hpp"

#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "Templates/Pointer.hpp"
#include "UniScriptState.hpp"
#include "wx/wx.h"


class GameConfigT;
class EditorMaterialI;


namespace GuiEditor
{
    struct GuiPropertiesT
    {
        GuiPropertiesT() {}
        GuiPropertiesT(cf::GuiSys::GuiImplT& Gui);

        bool     Activate;
        bool     Interactive;
        bool     ShowMouse;
        wxString DefaultFocus;
    };


    class GuiDocumentT : public SubjectT
    {
        public:

        GuiDocumentT(GameConfigT* GameConfig, const wxString& GuiInitFileName="");
        ~GuiDocumentT();

        IntrusivePtrT<cf::GuiSys::GuiImplT> GetGui() { return m_Gui; }
        IntrusivePtrT<cf::GuiSys::WindowT> GetRootWindow() { return m_Gui->GetRootWindow(); }

        GuiPropertiesT& GetGuiProperties() { return m_GuiProperties; }

        void SetSelection(const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection);
        const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& GetSelection() const { return m_Selection; }

        const ArrayT<EditorMaterialI*>& GetEditorMaterials() const { return m_EditorMaterials; }
        GameConfigT* GetGameConfig() { return m_GameConfig; }
        GuiDocAdapterT& GetAdapter() { return m_DocAdapter; }

        bool SaveInit_cgui(std::ostream& OutFile);

        static IntrusivePtrT<ComponentSelectionT> GetSelComp(IntrusivePtrT<cf::GuiSys::WindowT> Win);


        private:

        GuiDocumentT(const GuiDocumentT&);          ///< Use of the Copy    Constructor is not allowed.
        void operator = (const GuiDocumentT&);      ///< Use of the Assignment Operator is not allowed.

        cf::UniScriptStateT                          m_ScriptState;     ///< The script state that the m_Gui is bound to and lives in.
        IntrusivePtrT<cf::GuiSys::GuiImplT>          m_Gui;
        ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > m_Selection;
        GuiPropertiesT                               m_GuiProperties;
        ArrayT<EditorMaterialI*>                     m_EditorMaterials; ///< One editor material for each material in the GUI (its material manager).
        GameConfigT*                                 m_GameConfig;
        GuiDocAdapterT                               m_DocAdapter;      ///< Kept here because it sometimes needs the same lifetime as the GuiDocumentT itself, e.g. when referenced by a "material" property of the Window Inspector, or by commands in the command history.
    };
}

#endif
