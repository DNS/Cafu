/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#ifndef _GUIEDITOR_GUI_DOCUMENT_HPP_
#define _GUIEDITOR_GUI_DOCUMENT_HPP_

#include "ObserverPattern.hpp"

#include "wx/wx.h"


namespace cf { namespace GuiSys { class WindowT; } }
namespace cf { namespace GuiSys { class GuiImplT; } }
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

        cf::GuiSys::WindowT* GetRootWindow();
        cf::GuiSys::WindowT* FindWindowByName(const wxString& WindowName);

        GuiPropertiesT& GetGuiProperties() { return m_GuiProperties; }

        void SetSelection(const ArrayT<cf::GuiSys::WindowT*>& NewSelection);
        const ArrayT<cf::GuiSys::WindowT*>& GetSelection();

        GameConfigT* GetGameConfig() { return m_GameConfig; }

        void GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials);

        bool SaveInit_cgui(std::ostream& OutFile);


        private:

        cf::GuiSys::WindowT*         m_RootWindow;
        ArrayT<cf::GuiSys::WindowT*> m_Selection;
        GuiPropertiesT               m_GuiProperties;
        GameConfigT*                 m_GameConfig;
    };
}

#endif