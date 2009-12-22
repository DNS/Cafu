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

#ifndef _DIALOG_EDIT_SURFACE_PROPS_HPP_
#define _DIALOG_EDIT_SURFACE_PROPS_HPP_

#include "MapBezierPatch.hpp"   // For TexCoordGenModeT.
#include "ObserverPattern.hpp"
#include "Templates/Array.hpp"
#include "Math3D/Vector3.hpp"


class BitmapControlT;
class cfSpinControlT;
class EditorMaterialI;
class MapBrushT;
class MapDocumentT;
class MapElementT;
class MapFaceT;
class ViewWindow3DT;


class EditSurfacePropsDialogT : public wxPanel, public ObserverT
{
    public:

    static const unsigned long ALL_FACES; // Needed to select all faces of a brush.

    /// The constructor.
    EditSurfacePropsDialogT(wxWindow* Parent, MapDocumentT* MapDoc);

    // Implementation of the ObserverT interface.
    void NotifySubjectDies(SubjectT* Subject);

    /// Overridden wxDialog::Show() function, because we also want to update the dialog on Show(true).
    /// Note that this method is called *indirectly* by the wxAUI framework, when the related pane is shown or hidden!
    bool Show(bool show=true);

    /// Clears the list of faces and patches that were selected for surface-editing.
    void ClearSelection();

    /// Called when the user left clicked on a face/patch in the 3D view in order to toggle (select) it.
    /// FaceIndex==ALL_FACES will toggle all faces of a brush (if Object is a brush).
    void ToggleClick(MapElementT* Object, unsigned long FaceIndex);

    /// Called when the user right clicked on a face/patch in the 3D view to apply a material.
    /// FaceIndex==ALL_FACES will perform the apply click on all faces of a brush (if Object is a brush).
    void ApplyClick(ViewWindow3DT& ViewWin3D, MapElementT* Object, unsigned long FaceIndex);

    /// Called when a material is picked by the eye dropper (left mouse button click in eyedropper mode).
    void EyeDropperClick(MapElementT* Object, unsigned long FaceIndex);

    /// Returns true if the hide selection overlay checkbox is unchecked and false if it is checked.
    bool WantSelectionOverlay() const { return !CheckBoxHideSelMask->IsChecked(); }

    /// Returns the number of currently selected faces.
    unsigned long GetNrOfSelectedFaces() const { return m_SelectedFaces.Size(); }

    /// Returns the number of currently selected Bezier patches.
    unsigned long GetNrOfSelectedPatches() const { return m_SelectedPatches.Size(); }

    /// Returns the current list of MRU materials (the first element is the selected material).
    ArrayT<EditorMaterialI*> GetMRUMaterials() const;


    private:

    enum RightMBClickModeT  // For SetSurfaceInfo().
    {
        ApplyNormal,
        ApplyViewAligned,
        ApplyEdgeAligned,
        ApplyProjective
    };

    enum ApplySettingT      // Determines which value is applied if ApplyNormal is called.
    {
        ApplyNone    =0x00,
        ApplyScaleX  =0x01,
        ApplyScaleY  =0x02,
        ApplyShiftX  =0x04,
        ApplyShiftY  =0x08,
        ApplyRotation=0x10,
        ApplyMaterial=0x20,
        ApplyAll     =0xFF
    };

    /// A struct that describes a selected face.
    struct SelectedFaceT
    {
        MapFaceT*     Face;
        MapBrushT*    Brush;
        unsigned long FaceIndex;
    };

    MapDocumentT*            m_MapDoc;              ///< Pointer to the currently active document, or NULL when no document active.
    TexCoordGenModeT         m_CurrentTexGenMode;   ///< The texture coordinates generation mode for the currently (i.e. last) picked face/patch.
    Vector3fT                m_CurrentUAxis;        ///< The u-axis of the currently (i.e. last) picked face/patch.
    Vector3fT                m_CurrentVAxis;        ///< The v-axis of the currently (i.e. last) picked face/patch.
    ArrayT<SelectedFaceT>    m_SelectedFaces;       ///< The list of selected faces.
    ArrayT<MapBezierPatchT*> m_SelectedPatches;     ///< The list of selected patches.

    /// Applies the dialogs data to a face/patch using a specific ApplyMode, that decides how the data is applied.
    /// The ApplySetting is only used if ApplyMode is ApplyNormal to specify which data should be applied (e.g. only
    /// scale values). The ApplySetting should never be set when calling an ApplyMode other than ApplyNormal.
    void SetSurfaceInfo(const MapFaceT*        Face,  SurfaceInfoT& SI, EditorMaterialI** Material, const RightMBClickModeT ApplyMode, const ApplySettingT Setting, ViewWindow3DT* ViewWin3D=NULL) const;
    void SetSurfaceInfo(const MapBezierPatchT* Patch, SurfaceInfoT& SI, EditorMaterialI** Material, const RightMBClickModeT ApplyMode, const ApplySettingT Setting, ViewWindow3DT* ViewWin3D=NULL) const;

    // Updates the face normal and material vector info in the dialog.
    void UpdateVectorInfo();

    // "Orientation" section controls.
    cfSpinControlT* SpinCtrlScaleX;
    cfSpinControlT* SpinCtrlScaleY;
    cfSpinControlT* SpinCtrlShiftX;
    cfSpinControlT* SpinCtrlShiftY;
    cfSpinControlT* SpinCtrlRotation;

    // Face and Material Vector info text.
    wxStaticText*   MaterialXInfo;
    wxStaticText*   MaterialYInfo;

    // "Alignment" section controls.
    wxCheckBox*     CheckBoxAlignWrtWorld;
    wxCheckBox*     CheckBoxAlignWrtFace;
    wxCheckBox*     CheckBoxTreatMultipleAsOne;

    // "Material" section controls.
    wxChoice*       ChoiceCurrentMat;
    BitmapControlT* BitmapCurrentMat;
    wxStaticText*   StaticTextCurrentMatSize;

    // "Tool Mode" section controls.
    wxCheckBox*     CheckBoxHideSelMask;
    wxChoice*       ChoiceRightMBMode;

    // "Material Orientation" section event handlers (one for all spin controls).
    void OnSpinCtrlValueChanged(wxCommandEvent& Event);

    // "Alignment" section event handlers.
    void OnButtonAlign               (wxCommandEvent& Event);
    void OnCheckBoxAlignWorld        (wxCommandEvent& Event);
    void OnCheckBoxAlignFace         (wxCommandEvent& Event);
    void OnCheckBoxTreatMultipleAsOne(wxCommandEvent& Event);

    // "Material" section event handlers.
    void OnSelChangeCurrentMat(wxCommandEvent& Event);
    void OnButtonBrowseMats   (wxCommandEvent& Event);
    void OnButtonReplaceMats  (wxCommandEvent& Event);

    // "Tool Mode" section event handlers.
    void OnCheckBoxHideSelMask     (wxCommandEvent& Event);
    void OnSelChangeRightMB        (wxCommandEvent& Event);
    void OnButtonApplyToAllSelected(wxCommandEvent& Event);


    // IDs for the controls whose events we are interested in.
    enum
    {
        ID_SPINCTRL_SCALE_X=wxID_HIGHEST+1,
        ID_SPINCTRL_SCALE_Y,
        ID_SPINCTRL_SHIFT_X,
        ID_SPINCTRL_SHIFT_Y,
        ID_SPINCTRL_ROTATION,
        ID_BUTTON_ALIGN2FITFACE,
        ID_BUTTON_ALIGN2TOP,
        ID_BUTTON_ALIGN2LEFT,
        ID_BUTTON_ALIGN2CENTER,
        ID_BUTTON_ALIGN2RIGHT,
        ID_BUTTON_ALIGN2BOTTOM,
        ID_CHECKBOX_ALIGN_WRT_WORLD,
        ID_CHECKBOX_ALIGN_WRT_FACE,
        ID_CHECKBOX_TREAT_MULTIPLE_AS_ONE,
        ID_CHOICE_CURRENT_MAT,
        ID_BUTTON_BROWSE_MATS,
        ID_BUTTON_REPLACE_MATS,
        ID_CHECKBOX_HIDE_SEL_MASK,
        ID_CHOICE_RIGHT_MB_MODE,
        ID_BUTTON_APPLY_TO_ALL_SELECTED
    };

    DECLARE_EVENT_TABLE()
};

#endif
