/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_EDIT_SURFACE_PROPS_HPP_INCLUDED
#define CAFU_DIALOG_EDIT_SURFACE_PROPS_HPP_INCLUDED

#include "SurfaceInfo.hpp"
#include "ObserverPattern.hpp"
#include "wx/wx.h"
#include "wx/spinctrl.h"


class EditorMaterialI;
class MapBezierPatchT;
class MapBrushT;
class MapDocumentT;
class MapElementT;
class MapFaceT;
class ViewWindow3DT;


/**
 * The "Edit Surface Properties" dialog is the counterpart of the ToolEditSurfaceT tool.
 *
 * Its state is comprised of two largely independent sub-states:
 *   - the set of selected faces and Bezier patches,
 *   - the surface information of the last picked face or Bezier patch, some of
 *     which is kept in explicit members, some as values of the dialog controls.
 */
class EditSurfacePropsDialogT : public wxPanel, public ObserverT
{
    public:

    static const unsigned long ALL_FACES; // Needed to select all faces of a brush.

    /// The constructor.
    EditSurfacePropsDialogT(wxWindow* Parent, MapDocumentT* MapDoc);

    /// The destructor.
    ~EditSurfacePropsDialogT();

    // Implementation of the ObserverT interface.
    void NotifySubjectDies(SubjectT* Subject);

    /// Overridden wxDialog::Show() function, because we also want to update the dialog on Show(true).
    /// Note that this method is called *indirectly* by the wxAUI framework, when the related pane is shown or hidden!
    bool Show(bool show=true);

    /// Clears the list of faces and patches that were selected for surface-editing.
    void ClearSelection();

    /// This method toggles the selection status of the given object.
    /// It is called when the user left-clicked a face, Bezier patch or terrain in the 3D view.
    ///
    /// @param Object        The basic map object that is to be toggled.
    /// @param FaceIndex     If `Object` is a brush, `FaceIndex` indicates which face of the
    ///     brush is to be toggled, or `ALL_FACES` for all faces.
    /// @param IsRecursive   Used for implementation purposes only, user code should always
    ///     pass the default value (`false`).
    void ToggleClick(MapElementT* Object, unsigned long FaceIndex, bool IsRecursive = false);

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

    /// This enum describes how previously picked surface details
    /// are to be applied to another surface.
    enum ApplyModeT
    {
        ApplyNormal,
        ApplyViewAligned,
        ApplyEdgeAligned,
        ApplyProjective
    };

    /// This enum describes what surface detail (or combination of details)
    /// is to be applied to a face or patch.
    enum ApplyDetailT
    {
        ApplyNone    =0x00,
        ApplyScaleX  =0x01,
        ApplyScaleY  =0x02,
        ApplyShiftX  =0x04,
        ApplyShiftY  =0x08,
        ApplyRotation=0x10,
        ApplyAll     =0xFF
    };

    /// A struct that describes a selected face.
    struct SelectedFaceT
    {
        MapFaceT*     Face;
        MapBrushT*    Brush;
        unsigned long FaceIndex;
    };

    /// This struct helps to provide a better user experience when a material is applied to
    /// several faces or patches: If the application is not possible for some reason, we want
    /// to inform the user only once, not once per face or patch.
    struct MsgCountsT
    {
        MsgCountsT()
            : NoRefPlane(0), NoCenterFace(0), NoEdgeAlign(0) {}

        unsigned int NoRefPlane;
        unsigned int NoCenterFace;
        unsigned int NoEdgeAlign;
    };

    MapDocumentT*            m_MapDoc;              ///< Pointer to the currently active document, or NULL when no document active.
    TexCoordGenModeT         m_CurrentTexGenMode;   ///< The tex-coords generation mode for the currently (i.e. last) picked face/patch (`PlaneProj` or `MatFit`, never `Custom`).
    Vector3fT                m_CurrentUAxis;        ///< The u-axis of the currently (i.e. last) picked face/patch.
    Vector3fT                m_CurrentVAxis;        ///< The v-axis of the currently (i.e. last) picked face/patch.
    ArrayT<SelectedFaceT>    m_SelectedFaces;       ///< The list of selected faces.
    ArrayT<MapBezierPatchT*> m_SelectedPatches;     ///< The list of selected patches.

    /// Returns the material that is currently set in the ChoiceCurrentMat (or NULL for none).
    EditorMaterialI* GetCurrentMaterial() const;

    /// Computes a SurfaceInfoT instance for a MapFaceT according to the current dialog
    /// settings and the given parameters.
    ///
    /// @param Face
    ///     The face to compute a new SurfaceInfoT for. The defaults for all values that don't
    ///     change are copied from the existing SurfaceInfoT instance of this face.
    ///
    /// @param Mat
    ///     The "reference" material that provides the material's width and height values that
    ///     are needed for some of the computations. This can be `NULL` or `Face->GetMaterial()`
    ///     whenever the material is not intended to be replaced by the caller. If however the
    ///     caller intends to replace the face's material anyway, it must be the new material!
    ///
    /// @param ApplyMode
    ///     The algorithm that is used for computing the details of the new SurfaceInfoT
    ///     instance.
    ///
    /// @param Detail
    ///     If `ApplyMode` is `ApplyNormal`, `Detail` determines the subset of values to
    ///     account for. Must be `ApplyAll` in in all other `ApplyModes`.
    ///
    /// @param MsgCounts
    ///     If problems occur, this helps to inform the user about each problem only once.
    ///
    /// @param ViewWin3D
    ///     The 3D view that is needed for the computations when `ApplyMode` is
    ///     `ApplyViewAligned`.
    SurfaceInfoT ObtainSurfaceInfo(const MapFaceT*        Face,  EditorMaterialI* Mat, const ApplyModeT ApplyMode, const ApplyDetailT Detail, MsgCountsT& MsgCounts, ViewWindow3DT* ViewWin3D = NULL) const;

    /// Computes a SurfaceInfoT instance for a MapBezierPatchT according to the current dialog
    /// settings and the given parameters. For details, see ObtainSurfaceInfo() above.
    SurfaceInfoT ObtainSurfaceInfo(const MapBezierPatchT* Patch, EditorMaterialI* Mat, const ApplyModeT ApplyMode, const ApplyDetailT Detail, MsgCountsT& MsgCounts, ViewWindow3DT* ViewWin3D = NULL) const;

    // Updates the face normal and material vector info in the dialog.
    void UpdateVectorInfo();

    /// Updates the dialog controls depending on the current selection.
    void UpdateAfterSelChange();

    // "Orientation" section controls.
    wxSpinCtrlDouble* m_SpinCtrlScaleX;
    wxSpinCtrlDouble* m_SpinCtrlScaleY;
    wxSpinCtrlDouble* m_SpinCtrlShiftX;
    wxSpinCtrlDouble* m_SpinCtrlShiftY;
    wxSpinCtrlDouble* m_SpinCtrlRotation;
    wxStaticText*     m_TexGenModeInfo;

    // Face and Material Vector info text.
    wxStaticText*     MaterialXInfo;
    wxStaticText*     MaterialYInfo;

    // "Alignment" section controls.
    wxStaticText*     m_wrtWorldAxesText;
    wxStaticText*     m_wrtWorldAxesInfo;
    wxButton*         m_wrtWorldAxesButton;
    wxStaticText*     m_wrtFacePlaneText;
    wxStaticText*     m_wrtFacePlaneInfo;
    wxButton*         m_wrtFacePlaneButton;
    wxCheckBox*       m_CheckBoxTreatMultipleAsOne;

    // "Material" section controls.
    wxChoice*         ChoiceCurrentMat;
    wxStaticBitmap*   m_BitmapCurrentMat;
    wxStaticText*     StaticTextCurrentMatSize;

    // "Tool Mode" section controls.
    wxCheckBox*       CheckBoxHideSelMask;
    wxChoice*         ChoiceRightMBMode;

    // "Material Orientation" section event handlers (one for all spin controls).
    void OnSpinCtrlValueChanged(wxSpinDoubleEvent& Event);

    // "Alignment" section event handlers.
    void OnButtonAlign               (wxCommandEvent& Event);
    void OnButtonAlignWrtAxes        (wxCommandEvent& Event);
    void OnCheckBoxTreatMultipleAsOne(wxCommandEvent& Event);

    // "Material" section event handlers.
    void OnSelChangeCurrentMat(wxCommandEvent& Event);
    void OnButtonBrowseMats   (wxCommandEvent& Event);
    void OnButtonReplaceMats  (wxCommandEvent& Event);

    // "Tool Mode" section event handlers.
    void OnCheckBoxHideSelMask(wxCommandEvent& Event);
    void OnSelChangeRightMB   (wxCommandEvent& Event);


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
        ID_BUTTON_ALIGN_WRT_WORLD,
        ID_BUTTON_ALIGN_WRT_FACE,
        ID_CHECKBOX_TREAT_MULTIPLE_AS_ONE,
        ID_CHOICE_CURRENT_MAT,
        ID_BUTTON_BROWSE_MATS,
        ID_BUTTON_REPLACE_MATS,
        ID_CHECKBOX_HIDE_SEL_MASK,
        ID_CHOICE_RIGHT_MB_MODE
    };

    DECLARE_EVENT_TABLE()
};

#endif
