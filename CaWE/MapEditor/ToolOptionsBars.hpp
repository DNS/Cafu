/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_OPTIONS_BARS_HPP_INCLUDED
#define CAFU_TOOL_OPTIONS_BARS_HPP_INCLUDED

/// \file
/// This file contains the declarations of the tool options bars.
/// The bars are all derived from wxPanels, and add only little functionality.
/// Having them as separate classes makes many things a lot cleaner and clearer, though.

// Turn off bogus warnings that occur with VC11's static code analysis.
// (Should move this to a better place though, e.g. some `compat.h` file...)
#if defined(_WIN32) && defined(_MSC_VER)
    // warning C6011: dereferencing NULL pointer <name>
    #pragma warning(disable:6011)
#endif

#include "wx/wx.h"
#include "wx/spinctrl.h"


class ToolClipT;
class MapDocumentT;
class ToolMorphT;
class wxSpinEvent;


/// The options bar for the Selection tool.
class OptionsBar_SelectionToolT : public wxPanel
{
    public:

    /// The constructor.
    /// @param Parent is the parent window of this panel.
    OptionsBar_SelectionToolT(wxWindow* Parent);


    private:

    wxCheckBox* m_AutoGroupEntities; ///< The "auto-group entities" checkbox.
};


/// The options bar for the Camera tool.
class OptionsBar_CameraToolT : public wxPanel
{
    public:

    /// The constructor.
    OptionsBar_CameraToolT(wxWindow* Parent);
};


/// The options bar for the New Brush tool.
class OptionsBar_NewBrushToolT : public wxPanel
{
    public:

    struct BrushTypeInfoT
    {
        const char*   Name;
        unsigned long NrOfFaces;
        unsigned long NrOfFacesMin;
        unsigned long NrOfFacesMax;
    };

    /// The constructor.
    OptionsBar_NewBrushToolT(wxWindow* Parent);

    const int GetNrOfFaces () const { return m_NrOfFacesSpinControl->GetValue();     }
    const int GetBrushIndex() const { return m_BrushPrimitiveChoice->GetSelection(); }


    private:

    wxChoice*   m_BrushPrimitiveChoice;     ///< The wxChoice for the current brush primitive.
    wxSpinCtrl* m_NrOfFacesSpinControl;     ///< The wxSpinCtrl for the number of faces of the brush primitive.

    /// The event handlers.
    void OnSelChangeBrushPrimitives(wxCommandEvent&  Event);
    void OnSpinCtrlNrOfFaces       (wxSpinEvent&     Event);

    /// IDs for the controls whose events we are interested in.
    enum
    {
        ID_CHOICE_BRUSH_PRIMITIVES=wxID_HIGHEST+1,
        ID_SPINCTRL_NR_OF_FACES
    };

    DECLARE_EVENT_TABLE()
};


/// The options bar for the New Entity tool.
class OptionsBar_NewEntityToolT : public wxPanel
{
    public:

    /// The constructor.
    /// @param Parent is the parent window of this panel.
    /// @param MapDoc is a reference to our document.
    OptionsBar_NewEntityToolT(wxWindow* Parent, MapDocumentT& MapDoc);
};


/// The options bar for the New Bezier Patch tool.
class OptionsBar_NewBezierPatchToolT : public wxPanel
{
    public:

    /// The constructor.
    OptionsBar_NewBezierPatchToolT(wxWindow* Parent, MapDocumentT& MapDoc);

    unsigned long GetPatchResX() const;
    unsigned long GetPatchResY() const;
    bool          WithConvexEndCaps()  const { return m_CheckConvex ->GetValue(); }
    bool          WithConcaveEndCaps() const { return m_CheckConcave->GetValue(); }

    wxChoice*     m_ChoicePatchType;
    wxSpinCtrl*   m_SpinCtrlSubdivsHorz;
    wxSpinCtrl*   m_SpinCtrlSubdivsVert;


    private:

    MapDocumentT& m_MapDoc;     ///< A reference to our document.
 // wxChoice*     m_ChoicePatchType;
    wxChoice*     m_ChoicePatchResX;
    wxChoice*     m_ChoicePatchResY;
 // wxSpinCtrl*   m_SpinCtrlSubdivsHorz;
 // wxSpinCtrl*   m_SpinCtrlSubdivsVert;
    wxCheckBox*   m_CheckConvex;
    wxCheckBox*   m_CheckConcave;

    void OnPatchTypeChoice(wxCommandEvent& Event);      ///< Handles events that occur when a patch type is choosen from the patch type choice box.

    /// IDs for the controls whose events we are interested in.
    enum
    {
        ID_PATCHTYPE,
        ID_SUBDIVSHORZ,
        ID_SUBDIVSVERT
    };

    DECLARE_EVENT_TABLE()
};


/// The options bar for the New Terrain tool.
class OptionsBar_NewTerrainToolT : public wxPanel
{
    public:

    /// The constructor.
    OptionsBar_NewTerrainToolT(wxWindow* Parent, MapDocumentT& MapDoc);

    wxComboBox* m_ComboBoxHeightmapName;    /// The heightmap name combobox: "a text field plus the MRU list". It is maintained by the MapTerrainT helper.
    wxCheckBox* m_CheckBoxAddWallsAndCeil;  /// The New Terrain tool should add walls and a ceiling if this is checked.
    wxCheckBox* m_CheckBoxAddFloor;         /// The New Terrain tool should add a floor if this is checked.


    private:

    /// A reference to our document.
    MapDocumentT& m_MapDoc;

    /// The Browse button event handler.
    void OnButtonBrowse(wxCommandEvent& Event);

    /// IDs for the controls whose events we are interested in.
    enum
    {
        ID_BUTTON_BROWSE=wxID_HIGHEST+1,
    };

    DECLARE_EVENT_TABLE()
};


/// The options bar for the New Decal tool.
class OptionsBar_NewDecalToolT : public wxPanel
{
    public:

    /// The constructor.
    OptionsBar_NewDecalToolT(wxWindow* Parent);
};


/// The options bar for the Edit Face Properties tool.
class OptionsBar_EditFacePropsToolT : public wxPanel
{
    public:

    /// The constructor.
    OptionsBar_EditFacePropsToolT(wxWindow* Parent);
};


/// The options bar for the Clip Brushes tool.
class OptionsBar_ClipBrushesToolT : public wxPanel
{
    public:

    /// This enumeration describes the clip mode of the clip brushes tool.
    enum ClipModeT { KeepFront, KeepBack, KeepBoth };


    /// The constructor.
    OptionsBar_ClipBrushesToolT(wxWindow* Parent, ToolClipT& ToolClipBrushes);

    /// Returns the current clip mode.
    ClipModeT GetClipMode() const;

    /// Switches to the next clip mode, and wraps if necessary.
    /// This also calls the NoteClipModeChanged() method of the m_ToolClipBrushes
    /// (as if the user had changed the clip mode manually).
    void CycleClipMode();


    private:

    ToolClipT&     m_ToolClipBrushes;       ///< Our clip brushes tool.
    wxRadioButton* m_RB_ClipModeKeepFront;  ///< The wxRadioButton for keeping the front part.
    wxRadioButton* m_RB_ClipModeKeepBack;   ///< The wxRadioButton for keeping the back part.
    wxRadioButton* m_RB_ClipModeKeepBoth;   ///< The wxRadioButton for keeping both parts.

    /// The event handler.
    void OnSelChangeClipMode(wxCommandEvent& CE);

    /// IDs for the controls whose events we are interested in.
    enum
    {
        ID_RB_CLIPMODE_KEEP_FRONT=wxID_HIGHEST+1,
        ID_RB_CLIPMODE_KEEP_BACK,
        ID_RB_CLIPMODE_KEEP_BOTH
    };

    DECLARE_EVENT_TABLE()
};


/// The options bar for the Edit Vertices tool.
class OptionsBar_EditVerticesToolT : public wxPanel
{
    public:

    enum EditModeT { EditVertices, EditEdges, EditBoth };


    /// The constructor.
    OptionsBar_EditVerticesToolT(wxWindow* Parent, ToolMorphT& ToolEditVertices);

    /// Returns the current edit mode.
    EditModeT GetEditMode() const;

    /// Returns true if the current edit mode is EditVertices or EditBoth.
    bool IsEditingVertices() const;

    /// Returns true if the current edit mode is EditEdges or EditBoth.
    bool IsEditingEdges() const;

    /// Switches to the next edit mode, and wraps if necessary.
    /// This also calls the NoteEditModeChanged() method of the m_ToolEditVertices
    /// (as if the user had changed the edit mode manually).
    void CycleEditMode();


    private:

    ToolMorphT&    m_ToolEditVertices;      ///< Our edit vertices tool.
    wxRadioButton* m_RB_EditModeVertices;   ///< The wxRadioButton for editing the vertices.
    wxRadioButton* m_RB_EditModeEdges;      ///< The wxRadioButton for editing the edges.
    wxRadioButton* m_RB_EditModeBoth;       ///< The wxRadioButton for editing both the vertices and edges.

    /// The event handler.
    void OnSelChangeEditMode(wxCommandEvent& CE);

    /// The "Insert Vertex" button event handler.
    void OnButtonInsertVertex(wxCommandEvent& Event);

    /// IDs for the controls whose events we are interested in.
    enum
    {
        ID_RB_EDITMODE_VERTICES=wxID_HIGHEST+1,
        ID_RB_EDITMODE_EDGES,
        ID_RB_EDITMODE_BOTH,
        ID_BUTTON_INSERT_VERTEX
    };

    DECLARE_EVENT_TABLE()
};

#endif
