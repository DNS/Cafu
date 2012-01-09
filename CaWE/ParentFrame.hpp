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

#ifndef CAFU_PARENT_FRAME_HPP_INCLUDED
#define CAFU_PARENT_FRAME_HPP_INCLUDED

#include "wx/docview.h"     // Needed for wxFileHistory.
#include "wx/mdi.h"
#include "Templates/Array.hpp"

#if __linux__
#define HMODULE void*
#endif


class wxGLCanvas;
class wxGLContext;
class wxFileName;
class ChildFrameT;
class GameConfigT;
class MapDocumentT;
namespace ModelEditor { class ChildFrameT; }
namespace GuiEditor   { class ChildFrameT; }
namespace MatSys { class TextureMapI; }


/// This class represents the CaWE parent (main) frame.
class ParentFrameT : public wxMDIParentFrame
{
    public:

    /// IDs for the controls whose events we are interested in.
    /// This is a public enum so that our children (and possibly grandchildren) can have controls
    /// that trigger these events as well, e.g. child frames that duplicate our menu items or dialogs
    /// (any children of the child frames) that provide buttons for the same events.
    /// The IDs that the ParentFrameT class uses start at wxID_HIGHEST+1+1000 and the IDs that the various
    /// ChildFrameT classes (map, gui, and model editors) use start at wxID_HIGHEST+1+2000. This way, the
    /// children of the child frames (dialogs, panes, toolbars, ...) can start their own ID enumerations
    /// at wxID_HIGHEST+1. This keeps all IDs nicely unique when events bubble up from the dialogs, pane
    /// and toolbars first to the child frame and finally to the parent frame.
    /// See the "Events and Event Handling: How Events are Processed" in the wx documentation for more details.
    enum
    {
        ID_MENU_FILE_NEW_MAP=wxID_HIGHEST+1+1000,
        ID_MENU_FILE_NEW_MODEL,
        ID_MENU_FILE_NEW_GUI,
        ID_MENU_FILE_NEW_FONT,
        ID_MENU_FILE_OPEN,
        ID_MENU_FILE_CONFIGURE,
        ID_MENU_FILE_EXIT,

     // Note that the entire "Windows" menu is provided and managed by wx.
     // ID_MENU_WINDOW_NEW,
     // ID_MENU_WINDOW_CASCADE,
     // ID_MENU_WINDOW_TILE,
     // ID_MENU_WINDOW_ARRANGE_ICONS,
     // ID_MENU_WINDOW_LOG_MESSAGES,

        ID_MENU_HELP_CONTENTS,
        ID_MENU_HELP_CAFU_WEBSITE,
        ID_MENU_HELP_CAFU_FORUM,
        ID_MENU_HELP_D3_MTR_CONVERTER,
        ID_MENU_HELP_ABOUT
    };


    /// The constructor.
    ParentFrameT();

    /// The destructor.
    ~ParentFrameT();

    ChildFrameT*  GetActiveMapChildFrame() const;   ///< Returns the currently active child frame or NULL if no map childframe is active (e.g. no map open or GUI editor is active).
    MapDocumentT* GetActiveMapDoc() const;          ///< Returns the document of the currently active map child frame or NULL if no map document is active.

    // These member variables are public because they must be available to other code anyway.
    wxGLCanvas*                       m_GLCanvas;       ///< Our persistent "home" of the shared GL context. Used whenever there is no view.
    wxGLContext*                      m_GLContext;      ///< The OpenGL rendering context that represents our app-global OpenGL state.
    MatSys::TextureMapI*              m_WhiteTexture;   ///< A white texture map that is set as default lightmap whenever nothing else is available.
    wxFileHistory                     m_FileHistory;    ///< The file history of our and all our childrens "File" menu.
    ArrayT<ChildFrameT*>              m_ChildFrames;    ///< The list where all map   child frames register themselves on construction and unregister on destruction.
    ArrayT<ModelEditor::ChildFrameT*> m_MdlChildFrames; ///< The list where all model child frames register themselves on construction and unregister on destruction.
    ArrayT<GuiEditor::ChildFrameT*>   m_GuiChildFrames; ///< The list where all GUI   child frames register themselves on construction and unregister on destruction.

    /// The OpenGL attribute list for this window. The same list must be used for all child windows, so that they get identical pixel formats!
    static int OpenGLAttributeList[];


    private:

    /// A helper function for opening or creating Cafu documents (maps, models or GUIs), for learning which game config should be used
    /// (first by extrapolating the config from the document path or (if unsuccessful) by querying it from the user).
    GameConfigT* AskUserForGameConfig(const wxFileName& DocumentPath);

    /// Using the specified game config, this method opens the specified file in a new child frame:
    /// It inspects the suffix of the given filename in order to determine the proper document type (map, model or GUI),
    /// creates the document from the file, and finally creates a new child frame for the newly loaded document.
    /// Files that have an ambiguous filename suffix (e.g. ".map") must have a type specifier appended to their filename.
    /// Currently supported type specifiers are " (HL1)", " (HL2)" and " (D3)".
    /// Errors on opening the file are gracefully handled and the user is informed.
    wxMDIChildFrame* OpenFile(GameConfigT* GameConfig, wxString FileName);

#ifdef __WXGTK__
    void OnSize    (wxSizeEvent&    SE);
#endif
    void OnShow    (wxShowEvent&    SE);    ///< Event handler for "has been shown" events.
    void OnClose   (wxCloseEvent&   CE);    ///< Event handler for close events, e.g. after a system close button or command or a call to Close(). See wx Window Deletion Overview for more details.
    void OnMenuFile(wxCommandEvent& CE);    ///< Event handler for File menu events.
    void OnMenuHelp(wxCommandEvent& CE);    ///< Event handler for Help menu events.

    HMODULE m_RendererDLL;

    DECLARE_EVENT_TABLE()
};

#endif
