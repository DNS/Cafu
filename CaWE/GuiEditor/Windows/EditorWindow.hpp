/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_GUIEDITOR_EDITOR_WINDOW_HPP_INCLUDED
#define CAFU_GUIEDITOR_EDITOR_WINDOW_HPP_INCLUDED

#include "GuiSys/Window.hpp"
#include "wx/wx.h"


class wxPGProperty;
class wxPropertyGridEvent;
class wxPropertyGridManager;
namespace GuiEditor { class ChildFrameT; }
namespace GuiEditor { class GuiDocumentT; }


namespace GuiEditor
{
    /// The Gui Editor derives from \c cf::GuiSys::WindowT::ExtDataT in order to create its own
    /// "sibling" class hierarchy of windows that parallels the hierarchy in the GuiSys, effectively
    /// augmenting the original class hierarchy with any data or functions required for Gui Editor purposes.
    class EditorWindowT : public cf::GuiSys::WindowT::ExtDataT
    {
        public:

        /// The constructor.
        EditorWindowT(IntrusivePtrT<cf::GuiSys::WindowT> Win, GuiDocumentT* GuiDoc);

        /// The destructor.
        virtual ~EditorWindowT() { }

        /// Returns the GuiSys "dual" or "sibling" of this window.
        const IntrusivePtrT<cf::GuiSys::WindowT> GetDual() const { return m_Win; }

        // /// Returns the GuiDocumentT instance that this window lives in.
        // GuiDocumentT* GetGuiDoc() { return m_GuiDoc; }

        void SetSelected(bool IsSel) { m_IsSelected=IsSel; }

        bool IsSelected() const { return m_IsSelected; }

        /// Renders this window in the editor.
        /// This is used to render GUI editor specific things like e.g. selection state.
        virtual void Render() const;


        protected:

        IntrusivePtrT<cf::GuiSys::WindowT> m_Win;           ///< The GuiSys's "dual" or "sibling" of this window.
        GuiDocumentT*                      m_GuiDoc;        ///< The GUI document that this window lives in.
        bool                               m_IsSelected;    ///< Is this window selected for editing?
    };
}

#endif
