/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrame.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "DialogReplaceMaterials.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "MapDocument.hpp"
#include "MapFace.hpp"
#include "MapBezierPatch.hpp"
#include "MapBrush.hpp"
#include "MapTerrain.hpp"
#include "Tool.hpp"
#include "ToolManager.hpp"
#include "ToolbarMaterials.hpp"

#include "../Camera.hpp"
#include "../CommandHistory.hpp"
#include "../DocumentAdapter.hpp"
#include "../EditorMaterial.hpp"
#include "../EditorMaterialManager.hpp"
#include "../GameConfig.hpp"
#include "../ParentFrame.hpp"

#include "../MaterialBrowser/MaterialBrowserDialog.hpp"
#include "Commands/UpdateSurface.hpp"

#include "wx/image.h"
#include "wx/confbase.h"


const unsigned long EditSurfacePropsDialogT::ALL_FACES=0xFFFFFFFF;
static const int PREVIEW_BITMAP_SIZE=128;


BEGIN_EVENT_TABLE(EditSurfacePropsDialogT, wxPanel)
    EVT_SPINCTRLDOUBLE(EditSurfacePropsDialogT::ID_SPINCTRL_SCALE_X,               EditSurfacePropsDialogT::OnSpinCtrlValueChanged)
    EVT_SPINCTRLDOUBLE(EditSurfacePropsDialogT::ID_SPINCTRL_SCALE_Y,               EditSurfacePropsDialogT::OnSpinCtrlValueChanged)
    EVT_SPINCTRLDOUBLE(EditSurfacePropsDialogT::ID_SPINCTRL_SHIFT_X,               EditSurfacePropsDialogT::OnSpinCtrlValueChanged)
    EVT_SPINCTRLDOUBLE(EditSurfacePropsDialogT::ID_SPINCTRL_SHIFT_Y,               EditSurfacePropsDialogT::OnSpinCtrlValueChanged)
    EVT_SPINCTRLDOUBLE(EditSurfacePropsDialogT::ID_SPINCTRL_ROTATION,              EditSurfacePropsDialogT::OnSpinCtrlValueChanged)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN2FITFACE,           EditSurfacePropsDialogT::OnButtonAlign)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN2TOP,               EditSurfacePropsDialogT::OnButtonAlign)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN2LEFT,              EditSurfacePropsDialogT::OnButtonAlign)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN2CENTER,            EditSurfacePropsDialogT::OnButtonAlign)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN2RIGHT,             EditSurfacePropsDialogT::OnButtonAlign)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN2BOTTOM,            EditSurfacePropsDialogT::OnButtonAlign)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN_WRT_WORLD,         EditSurfacePropsDialogT::OnButtonAlignWrtAxes)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_ALIGN_WRT_FACE,          EditSurfacePropsDialogT::OnButtonAlignWrtAxes)
    EVT_CHECKBOX      (EditSurfacePropsDialogT::ID_CHECKBOX_TREAT_MULTIPLE_AS_ONE, EditSurfacePropsDialogT::OnCheckBoxTreatMultipleAsOne)
    EVT_CHOICE        (EditSurfacePropsDialogT::ID_CHOICE_CURRENT_MAT,             EditSurfacePropsDialogT::OnSelChangeCurrentMat)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_BROWSE_MATS,             EditSurfacePropsDialogT::OnButtonBrowseMats)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_REPLACE_MATS,            EditSurfacePropsDialogT::OnButtonReplaceMats)
    EVT_CHECKBOX      (EditSurfacePropsDialogT::ID_CHECKBOX_HIDE_SEL_MASK,         EditSurfacePropsDialogT::OnCheckBoxHideSelMask)
    EVT_CHOICE        (EditSurfacePropsDialogT::ID_CHOICE_RIGHT_MB_MODE,           EditSurfacePropsDialogT::OnSelChangeRightMB)
END_EVENT_TABLE()


EditSurfacePropsDialogT::EditSurfacePropsDialogT(wxWindow* Parent, MapDocumentT* MapDoc)
    : wxPanel(Parent, -1),
      m_MapDoc(MapDoc),
      m_CurrentTexGenMode(PlaneProj),   // Per default use a texture generation mode that has no restrictions when applying.
      m_CurrentUAxis(1.0f, 0.0f, 0.0f),
      m_CurrentVAxis(0.0f, 1.0f, 0.0f),
      m_SpinCtrlScaleX(NULL),
      m_SpinCtrlScaleY(NULL),
      m_SpinCtrlShiftX(NULL),
      m_SpinCtrlShiftY(NULL),
      m_SpinCtrlRotation(NULL),
      m_TexGenModeInfo(NULL),
      MaterialXInfo(NULL),
      MaterialYInfo(NULL),
      m_wrtWorldAxesText(NULL),
      m_wrtWorldAxesInfo(NULL),
      m_wrtWorldAxesButton(NULL),
      m_wrtFacePlaneText(NULL),
      m_wrtFacePlaneInfo(NULL),
      m_wrtFacePlaneButton(NULL),
      m_CheckBoxTreatMultipleAsOne(NULL),
      ChoiceCurrentMat(NULL),
      m_BitmapCurrentMat(NULL),
      StaticTextCurrentMatSize(NULL),
      CheckBoxHideSelMask(NULL),
      ChoiceRightMBMode(NULL)
{
    // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item2 = new wxStaticBox(this, -1, wxT("Orientation") );
    wxStaticBoxSizer *item1 = new wxStaticBoxSizer( item2, wxVERTICAL );

    wxFlexGridSizer *item3 = new wxFlexGridSizer( 5, 0, 0 );
    item3->AddGrowableCol( 1 );
    item3->AddGrowableCol( 3 );
    item3->AddGrowableCol( 4 );

    item3->Add( 10, 10, 0, wxALIGN_CENTER, 5 );

    wxStaticText *item4 = new wxStaticText(this, -1, wxT("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item4, 0, wxALIGN_BOTTOM|wxRIGHT|wxTOP, 5 );

    item3->Add( 10, 10, 0, wxALIGN_CENTER, 5 );

    wxStaticText *item5 = new wxStaticText(this, -1, wxT("Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item5, 0, wxALIGN_BOTTOM|wxRIGHT|wxTOP, 5 );

    wxStaticText *item6 = new wxStaticText(this, -1, wxT("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item6, 0, wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );

    wxStaticText *item7 = new wxStaticText(this, -1, wxT("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item7, 0, wxALIGN_CENTER|wxALL, 5 );

    m_SpinCtrlScaleX=new wxSpinCtrlDouble(this, ID_SPINCTRL_SCALE_X, "1.0", wxDefaultPosition, wxSize(70-16, -1), wxSP_ARROW_KEYS, -1000.0, 1000.0, 1.0, 0.05);
    item3->Add(m_SpinCtrlScaleX, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    wxStaticText *item9 = new wxStaticText(this, -1, wxT("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item9, 0, wxALIGN_CENTER|wxALL, 5 );

    m_SpinCtrlShiftX=new wxSpinCtrlDouble(this, ID_SPINCTRL_SHIFT_X, "0.0", wxDefaultPosition, wxSize(70-16, -1), wxSP_ARROW_KEYS, -999999999.0, 999999999.0, 0.0, 1.0);
    item3->Add(m_SpinCtrlShiftX, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

    m_SpinCtrlRotation=new wxSpinCtrlDouble(this, ID_SPINCTRL_ROTATION, "0.0", wxDefaultPosition, wxSize(70-16, -1), wxSP_ARROW_KEYS | wxSP_WRAP, 0.0, 360.0, 0.0, 5.0);
    item3->Add(m_SpinCtrlRotation, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

    wxStaticText *item12 = new wxStaticText(this, -1, wxT("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item12, 0, wxALIGN_CENTER|wxALL, 5 );

    m_SpinCtrlScaleY=new wxSpinCtrlDouble(this, ID_SPINCTRL_SCALE_Y, "1.0", wxDefaultPosition, wxSize(70-16, -1), wxSP_ARROW_KEYS, -1000.0, 1000.0, 1.0, 0.05);
    item3->Add(m_SpinCtrlScaleY, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5 );

    wxStaticText *item14 = new wxStaticText(this, -1, wxT("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item14, 0, wxALIGN_CENTER|wxALL, 5 );

    m_SpinCtrlShiftY=new wxSpinCtrlDouble(this, ID_SPINCTRL_SHIFT_Y, "0.0", wxDefaultPosition, wxSize(70-16, -1), wxSP_ARROW_KEYS, -999999999.0, 999999999.0, 0.0, 1.0);
    item3->Add(m_SpinCtrlShiftY, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxBOTTOM, 5 );

    m_TexGenModeInfo=new wxStaticText(this, -1, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE | wxST_NO_AUTORESIZE);
    item3->Add(m_TexGenModeInfo, 0, wxGROW|wxALL, 5 );

    item1->Add( item3, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticBox *item81 = new wxStaticBox( this, -1, wxT("Material Vectors") );
    wxStaticBoxSizer *item80 = new wxStaticBoxSizer( item81, wxVERTICAL );

    wxBoxSizer *item85 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item86 = new wxStaticText( this, -1, wxT("Material X:"), wxDefaultPosition, wxDefaultSize, 0 );
    item85->Add( item86, 1, wxALIGN_CENTER|wxALL, 5 );

    MaterialXInfo = new wxStaticText( this, -1, wxT("( 0.000000 | 0.000000 | 0.000000 )"), wxDefaultPosition, wxDefaultSize, 0 );
    item85->Add( MaterialXInfo, 3, wxALIGN_CENTER|wxTOP|wxBOTTOM, 5 );

    item80->Add( item85, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item88 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item89 = new wxStaticText( this, -1, wxT("Material Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    item88->Add( item89, 1, wxALIGN_CENTER|wxALL, 5 );

    MaterialYInfo = new wxStaticText( this, -1, wxT("( 0.000000 | 0.000000 | 0.000000 )"), wxDefaultPosition, wxDefaultSize, 0 );
    item88->Add( MaterialYInfo, 3, wxALIGN_CENTER|wxTOP|wxBOTTOM, 5 );

    item80->Add( item88, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item80, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    // Hide vector info if hidden features are not activated.
    if (wxConfigBase::Get()->Read("General/Activate Hidden", 0L)!=0x1978)
        item0->Show(item80, false, true);

    wxStaticBox *item30 = new wxStaticBox(this, -1, wxT("Material") );
    wxStaticBoxSizer *item29 = new wxStaticBoxSizer( item30, wxVERTICAL );

    ChoiceCurrentMat= new wxChoice(this, ID_CHOICE_CURRENT_MAT, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    item29->Add(ChoiceCurrentMat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxBoxSizer *item37 = new wxBoxSizer( wxHORIZONTAL );

    m_BitmapCurrentMat=new wxStaticBitmap(this, -1, wxBitmap(), wxDefaultPosition, wxSize(PREVIEW_BITMAP_SIZE, PREVIEW_BITMAP_SIZE), wxSUNKEN_BORDER);
    item37->Add(m_BitmapCurrentMat, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    wxBoxSizer *item39 = new wxBoxSizer( wxVERTICAL );

    StaticTextCurrentMatSize= new wxStaticText(this, -1, wxT("Size: a x b"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
    item39->Add(StaticTextCurrentMatSize, 0, wxGROW|wxALL, 5 );

    wxButton *item35 = new wxButton( this, ID_BUTTON_BROWSE_MATS, wxT("Browse"), wxDefaultPosition, wxDefaultSize, 0 );
    item35->SetDefault();
    item39->Add( item35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item39->Add( 15, 15, 1, wxALIGN_CENTER_VERTICAL, 5 );

    wxButton *item53 = new wxButton(this, ID_BUTTON_REPLACE_MATS, wxT("Globally Replace"), wxDefaultPosition, wxDefaultSize, 0 );
    item39->Add( item53, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item37->Add( item39, 1, wxGROW|wxALIGN_CENTER_HORIZONTAL, 5 );

    item29->Add( item37, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item29, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticBox *item17 = new wxStaticBox(this, -1, wxT("Alignment") );
    wxStaticBoxSizer *item16 = new wxStaticBoxSizer( item17, wxHORIZONTAL );

    wxGridSizer *item18 = new wxGridSizer( 3, 0, 0 );

    wxButton *item19 = new wxButton(this, ID_BUTTON_ALIGN2FITFACE, wxT("Fit"), wxDefaultPosition, wxSize(21,21), 0 );
    item18->Add( item19, 0, wxALIGN_CENTER, 5 );

    wxButton *item20 = new wxButton(this, ID_BUTTON_ALIGN2TOP, wxT("T"), wxDefaultPosition, wxSize(21,21), 0 );
    item18->Add( item20, 0, wxALIGN_CENTER, 5 );

    item18->Add( 5, 5, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item21 = new wxButton(this, ID_BUTTON_ALIGN2LEFT, wxT("L"), wxDefaultPosition, wxSize(21,21), 0 );
    item18->Add( item21, 0, wxALIGN_CENTER, 5 );

    wxButton *item22 = new wxButton(this, ID_BUTTON_ALIGN2CENTER, wxT("C"), wxDefaultPosition, wxSize(21,21), 0 );
    item18->Add( item22, 0, wxALIGN_CENTER, 5 );

    wxButton *item23 = new wxButton(this, ID_BUTTON_ALIGN2RIGHT, wxT("R"), wxDefaultPosition, wxSize(21,21), 0 );
    item18->Add( item23, 0, wxALIGN_CENTER, 5 );

    item18->Add( 5, 5, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item24 = new wxButton(this, ID_BUTTON_ALIGN2BOTTOM, wxT("B"), wxDefaultPosition, wxSize(21,21), 0 );
    item18->Add( item24, 0, wxALIGN_CENTER, 5 );

    item18->Add( 5, 5, 0, wxALIGN_CENTER|wxALL, 5 );

    item16->Add( item18, 0, wxALIGN_CENTER|wxALL, 5 );

    item16->Add( 20, 20, 1, wxALIGN_CENTER|wxALL, 5 );

    wxBoxSizer *item25 = new wxBoxSizer( wxVERTICAL );


    wxFlexGridSizer* AlignWrtSizer = new wxFlexGridSizer(3, 0, 0);
    // AlignWrtSizer->AddGrowableCol(2);

    // Init the "wrt. world axes" row.
    // The text for wrtWorldAxesInfo is properly set in UpdateAfterSelChange().
    m_wrtWorldAxesText = new wxStaticText(this, -1, "wrt. world axes:", wxDefaultPosition, wxDefaultSize, 0 );
    AlignWrtSizer->Add(m_wrtWorldAxesText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_wrtWorldAxesInfo = new wxStaticText(this, -1, "", wxDefaultPosition, wxSize(35, -1), 0 );
    AlignWrtSizer->Add(m_wrtWorldAxesInfo, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    m_wrtWorldAxesButton = new wxButton(this, ID_BUTTON_ALIGN_WRT_WORLD, "Align", wxDefaultPosition, wxSize(42, 21), 0);
    AlignWrtSizer->Add(m_wrtWorldAxesButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    // Init the "wrt. face plane" row.
    // The text for wrtFacePlaneInfo is properly set in UpdateAfterSelChange().
    m_wrtFacePlaneText = new wxStaticText(this, -1, "wrt. face plane:", wxDefaultPosition, wxDefaultSize, 0 );
    AlignWrtSizer->Add(m_wrtFacePlaneText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_wrtFacePlaneInfo = new wxStaticText(this, -1, "", wxDefaultPosition, wxSize(35, -1), 0 );
    AlignWrtSizer->Add(m_wrtFacePlaneInfo, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    m_wrtFacePlaneButton = new wxButton(this, ID_BUTTON_ALIGN_WRT_FACE, "Align", wxDefaultPosition, wxSize(42, 21), 0);
    AlignWrtSizer->Add(m_wrtFacePlaneButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    item25->Add(AlignWrtSizer);


    item25->Add( 15, 15, 0, wxALIGN_CENTER, 5 );

    m_CheckBoxTreatMultipleAsOne = new wxCheckBox(this, ID_CHECKBOX_TREAT_MULTIPLE_AS_ONE, wxT("Treat multiple as one"), wxDefaultPosition, wxDefaultSize, 0 );
    item25->Add(m_CheckBoxTreatMultipleAsOne, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5);

    item16->Add( item25, 0, wxGROW|wxALIGN_CENTER_HORIZONTAL, 5 );

    item0->Add( item16, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticBox *item55 = new wxStaticBox(this, -1, wxT("Tool Mode") );
    wxStaticBoxSizer *item54 = new wxStaticBoxSizer( item55, wxVERTICAL );

    CheckBoxHideSelMask= new wxCheckBox(this, ID_CHECKBOX_HIDE_SEL_MASK, wxT("Hide Selection Overlay"), wxDefaultPosition, wxDefaultSize, 0 );
    item54->Add(CheckBoxHideSelMask, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxFlexGridSizer *item57 = new wxFlexGridSizer( 2, 0, 0 );
    item57->AddGrowableCol( 1 );

    wxStaticText *item60 = new wxStaticText(this, -1, wxT("Right MB mode:"), wxDefaultPosition, wxDefaultSize, 0 );
    item57->Add( item60, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM|wxTOP, 5 );

    ChoiceRightMBMode= new wxChoice(this, ID_CHOICE_RIGHT_MB_MODE, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    ChoiceRightMBMode->Append("Apply Normal"       , (void*)ApplyNormal     );
    ChoiceRightMBMode->Append("Apply View Aligned" , (void*)ApplyViewAligned);
    ChoiceRightMBMode->Append("Apply Edge Aligned" , (void*)ApplyEdgeAligned);
    ChoiceRightMBMode->Append("Apply Projective"   , (void*)ApplyProjective );
    ChoiceRightMBMode->SetSelection(0);
    item57->Add(ChoiceRightMBMode, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item54->Add( item57, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item54, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);


    UpdateAfterSelChange();

    // Update selected material.
    EditorMaterialI* DefaultMat=m_MapDoc->GetGameConfig()->GetMatMan().GetDefaultMaterial();

    wxASSERT(ChoiceCurrentMat->IsEmpty());
    ChoiceCurrentMat->Append(DefaultMat->GetName(), DefaultMat);
    ChoiceCurrentMat->SetSelection(0);


    // Call this event method to repaint displayed material.
    wxCommandEvent CE;
    CE.SetExtraLong(-1);    // Don't try to apply the material to the selection.
    OnSelChangeCurrentMat(CE);

    m_MapDoc->RegisterObserver(this);
}


EditSurfacePropsDialogT::~EditSurfacePropsDialogT()
{
    if (m_MapDoc)
        m_MapDoc->UnregisterObserver(this);
}


void EditSurfacePropsDialogT::NotifySubjectDies(SubjectT* Subject)
{
    wxASSERT(Subject==m_MapDoc);

    m_MapDoc=NULL;
}


bool EditSurfacePropsDialogT::Show(bool show)
{
    if (show)
    {
        UpdateAfterSelChange();

        wxASSERT(m_MapDoc->GetChildFrame()->GetMaterialsToolbar()!=NULL);

        // Synchronize with materials toolbar.
        ArrayT<EditorMaterialI*> MRUMaterials=m_MapDoc->GetChildFrame()->GetMaterialsToolbar()->GetMRUMaterials();

        wxASSERT(MRUMaterials.Size()>0 && MRUMaterials.Size()<=10);

        ChoiceCurrentMat->Clear();

        for (unsigned long MatNr=0; MatNr<MRUMaterials.Size(); MatNr++)
            ChoiceCurrentMat->Append(MRUMaterials[MatNr]->GetName(), MRUMaterials[MatNr]);

        ChoiceCurrentMat->SetSelection(0);

        // Call this event method to repaint displayed material.
        wxCommandEvent CE;
        CE.SetExtraLong(-1);    // Don't try to apply the material to the selection.
        OnSelChangeCurrentMat(CE);
    }

    return wxPanel::Show(show);
}


void EditSurfacePropsDialogT::ClearSelection()
{
    for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        m_SelectedFaces[FaceNr].Face->m_IsSelected=false;

    m_SelectedFaces.Overwrite();

    for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); PatchNr++)
        m_SelectedPatches[PatchNr]->SetSelected(false);

    m_SelectedPatches.Overwrite();

    UpdateAfterSelChange();
}


void EditSurfacePropsDialogT::ToggleClick(MapElementT* Object, unsigned long FaceIndex, bool IsRecursive)
{
    if (!Object) return;

    MapBrushT*       Brush=dynamic_cast<MapBrushT*>(Object);
    MapBezierPatchT* Patch=dynamic_cast<MapBezierPatchT*>(Object);

    if (Brush)
    {
        if (FaceIndex == ALL_FACES)
        {
            // Solve the problem "recursively".
            for (unsigned long FaceNr = 0; FaceNr < Brush->GetFaces().Size(); FaceNr++)
                ToggleClick(Object, FaceNr, true);
        }
        else
        {
            MapFaceT*     Face=&Brush->GetFaces()[FaceIndex];
            unsigned long FaceNr;

            for (FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
                if (m_SelectedFaces[FaceNr].Face==Face)
                    break;

            if (FaceNr>=m_SelectedFaces.Size())
            {
                // Add face to list of selected faces.
                m_SelectedFaces.PushBackEmpty();

                m_SelectedFaces[m_SelectedFaces.Size()-1].Face     =Face;
                m_SelectedFaces[m_SelectedFaces.Size()-1].Brush    =Brush;
                m_SelectedFaces[m_SelectedFaces.Size()-1].FaceIndex=FaceIndex;

                Face->m_IsSelected=true;
            }
            else
            {
                // Remove face from list of selected faces.
                m_SelectedFaces[FaceNr].Face->m_IsSelected=false;
                m_SelectedFaces.RemoveAtAndKeepOrder(FaceNr);
            }
        }
    }

    if (Patch)
    {
        unsigned long PatchNr;

        for (PatchNr=0; PatchNr<m_SelectedPatches.Size(); ++PatchNr)
            if (m_SelectedPatches[PatchNr]==Patch)
                break;

        if (PatchNr>=m_SelectedPatches.Size())
        {
            m_SelectedPatches.PushBack(Patch);
            Patch->SetSelected(true);
        }
        else
        {
            m_SelectedPatches[PatchNr]->SetSelected(false);
            m_SelectedPatches.RemoveAtAndKeepOrder(PatchNr);
        }
    }

    if (!IsRecursive)
    {
        UpdateAfterSelChange();

        ArrayT<MapElementT*> UpdateObjects;
        UpdateObjects.PushBack(Object);

        m_MapDoc->UpdateAllObservers_Modified(UpdateObjects, MEMD_SURFACE_INFO_CHANGED);
    }
}


void EditSurfacePropsDialogT::ApplyClick(ViewWindow3DT& ViewWin3D, MapElementT* Object, unsigned long FaceIndex)
{
    if (Object==NULL) return;

    const ApplyModeT ApplyMode = (ApplyModeT)(uintptr_t)ChoiceRightMBMode->GetClientData(ChoiceRightMBMode->GetSelection());

    MapBrushT*       Brush  =dynamic_cast<MapBrushT*>(Object);
    MapBezierPatchT* Patch  =dynamic_cast<MapBezierPatchT*>(Object);
    MapTerrainT*     Terrain=dynamic_cast<MapTerrainT*>(Object);

    MsgCountsT        MsgCounts;
    ArrayT<CommandT*> SurfaceCommands;

    if (Brush!=NULL)
    {
        if (FaceIndex==ALL_FACES) // Apply on all faces of Brush.
        {
            for (unsigned long FaceNr=0; FaceNr<Brush->GetFaces().Size(); FaceNr++)
            {
                EditorMaterialI*   Material = GetCurrentMaterial();
                const SurfaceInfoT SI = ObtainSurfaceInfo(&Brush->GetFaces()[FaceNr], Material, ApplyMode, ApplyAll, MsgCounts, &ViewWin3D);

                SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, Brush, FaceNr, SI, Material));
            }
        }
        else // Just apply on face at FaceIndex.
        {
            EditorMaterialI*   Material = GetCurrentMaterial();
            const SurfaceInfoT SI = ObtainSurfaceInfo(&Brush->GetFaces()[FaceIndex], Material, ApplyMode, ApplyAll, MsgCounts, &ViewWin3D);

            SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, Brush, FaceIndex, SI, Material));
        }
    }

    if (Patch!=NULL)
    {
        EditorMaterialI*   Material = GetCurrentMaterial();
        const SurfaceInfoT SI = ObtainSurfaceInfo(Patch, Material, ApplyMode, ApplyAll, MsgCounts, &ViewWin3D);

        SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(*m_MapDoc, Patch, SI, Material));
    }

    if (Terrain!=NULL)
    {
        EditorMaterialI* Material = GetCurrentMaterial();

        SurfaceCommands.PushBack(new CommandUpdateSurfaceTerrainT(*m_MapDoc, Terrain, Material));
    }

    if (SurfaceCommands.Size()>0)
    {
        CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Apply material");

        m_MapDoc->CompatSubmitCommand(Macro);
    }
}


void EditSurfacePropsDialogT::EyeDropperClick(MapElementT* Object, unsigned long FaceIndex)
{
    EditorMaterialI* Material=NULL;

    MapBrushT*       Brush  =dynamic_cast<MapBrushT*>(Object);
    MapBezierPatchT* Patch  =dynamic_cast<MapBezierPatchT*>(Object);
    MapTerrainT*     Terrain=dynamic_cast<MapTerrainT*>(Object);

    if (Brush!=NULL)
    {
        const MapFaceT*     Face=&Brush->GetFaces()[FaceIndex];
        const SurfaceInfoT& SI  =Face->GetSurfaceInfo();

        m_SpinCtrlScaleX  ->SetValue(1.0/(SI.Scale[0]*Face->GetMaterial()->GetWidth()));
        m_SpinCtrlScaleY  ->SetValue(1.0/(SI.Scale[1]*Face->GetMaterial()->GetHeight()));
        m_SpinCtrlShiftX  ->SetValue(SI.Trans[0]*Face->GetMaterial()->GetWidth());
        m_SpinCtrlShiftY  ->SetValue(SI.Trans[1]*Face->GetMaterial()->GetHeight());
        m_SpinCtrlRotation->SetValue(SI.Rotate);
        m_TexGenModeInfo  ->SetLabel("");

        Material=Face->GetMaterial();

        // Faces should never have anything other than PlaneProj mode.
        wxASSERT(SI.TexCoordGenMode == PlaneProj);
        m_CurrentTexGenMode = SI.TexCoordGenMode;

        m_CurrentUAxis=SI.UAxis;
        m_CurrentVAxis=SI.VAxis;

        UpdateVectorInfo();
    }

    if (Patch!=NULL)
    {
        Material=Patch->GetMaterial();

        const SurfaceInfoT& SI=Patch->GetSurfaceInfo();

        if (SI.TexCoordGenMode==Custom)
        {
            wxMessageBox("Cannot pick the surface orientation of this Bezier patch.\n\n"
                "The texture coordinates of this Bezier Patch are in a custom\n"
                "format that cannot be picked into the dialog.\n"
                "Therefore, only the material is picked, but not the orientation.\n\n"
                "You can fix this problem by assigning new surface information to\n"
                "the Bezier patch, using the right mouse button.");
        }
        else
        {
            m_SpinCtrlScaleX  ->SetValue((SI.TexCoordGenMode==MatFit) ? SI.Scale[0] : 1.0/SI.Scale[0]/Patch->GetMaterial()->GetWidth());
            m_SpinCtrlScaleY  ->SetValue((SI.TexCoordGenMode==MatFit) ? SI.Scale[1] : 1.0/SI.Scale[1]/Patch->GetMaterial()->GetHeight());
            m_SpinCtrlShiftX  ->SetValue(SI.Trans[0]*Patch->GetMaterial()->GetWidth());
            m_SpinCtrlShiftY  ->SetValue(SI.Trans[1]*Patch->GetMaterial()->GetHeight());
            m_SpinCtrlRotation->SetValue(SI.Rotate);
            m_TexGenModeInfo  ->SetLabel("");   // The proper value is set below.

            m_CurrentTexGenMode=SI.TexCoordGenMode;

            // If SI.TexCoordGenMode == MatFit, the axes are of zero-length and thus invalid.
            m_CurrentUAxis=SI.UAxis;
            m_CurrentVAxis=SI.VAxis;

            UpdateVectorInfo();

            switch (m_CurrentTexGenMode)
            {
                case Custom: m_TexGenModeInfo->SetLabel("Mode: Custom"); break;   // This is already caught above.
                case MatFit: m_TexGenModeInfo->SetLabel("Mode: Fit");    break;
                default:     m_TexGenModeInfo->SetLabel("");             break;
            }
        }
    }

    if (Terrain!=NULL)
    {
        Material=Terrain->GetMaterial();
    }

    if (Material!=NULL)
    {
        int Index=ChoiceCurrentMat->FindString(Material->GetName());

        if (Index==-1)
        {
            // If there are already 10 or more items in the control, delete the last one.
            while (ChoiceCurrentMat->GetCount()>=10)
                ChoiceCurrentMat->Delete(ChoiceCurrentMat->GetCount()-1);

            // Material not found in list, add it now.
            ChoiceCurrentMat->Append(Material->GetName(), Material);
            Index=ChoiceCurrentMat->GetCount()-1;
        }

        ChoiceCurrentMat->SetSelection(Index);
    }

    wxCommandEvent CE;
    CE.SetExtraLong(-1);    // Tell the dialog not to update material of currently selected faces/patches.
    OnSelChangeCurrentMat(CE);
}


/********************************/
/*** Private Helper Functions ***/
/********************************/

EditorMaterialI* EditSurfacePropsDialogT::GetCurrentMaterial() const
{
    const int CurrentMatIndex = ChoiceCurrentMat->GetSelection();

    if (CurrentMatIndex == -1) return NULL;

    return (EditorMaterialI*)ChoiceCurrentMat->GetClientData(CurrentMatIndex);
}


namespace
{
    // Make sure that the given value doesn't get too close to 0.
    float NonZero(float f)
    {
        if (f >= 0.0f && f <  0.01f) return  0.01f;
        if (f <  0.0f && f > -0.01f) return -0.01f;

        return f;
    }
}


/**
 * When a material is applied to the Face instance, the following combinations of ApplyMode
 * and m_CurrentTexGenMode ("What was last picked into the dialog?") must be considered:
 *
 *                                    Apply to Face instance using mode:
 *                            +--------+--------------+--------------+-----------+
 *                            | Normal | View Aligned | Edge Aligned | Projected |
 *                +-----------+--------+--------------+--------------+-----------+
 *   last picked  | PlaneProj |   ok   |      ok      |      ok      |    ok     |
 *   m_CurrTexGM  | MatFit    |   ok   |      ok      |      ok      |    -/-    |
 *                +-----------+--------+--------------+--------------+-----------+
 */
SurfaceInfoT EditSurfacePropsDialogT::ObtainSurfaceInfo(const MapFaceT* Face, EditorMaterialI* Mat, const ApplyModeT ApplyMode, const ApplyDetailT Detail, MsgCountsT& MsgCounts, ViewWindow3DT* ViewWin3D) const
{
    wxASSERT(ApplyMode == ApplyNormal || Detail == ApplyAll);

    if (!Mat)
    {
        // For convenience, allow the caller to pass NULL in place of Face->GetMaterial().
        Mat = Face->GetMaterial();
    }

    // Fetch the scale values from the dialog, making sure that they don't get too close to 0.
    const float DialogScaleX = NonZero(m_SpinCtrlScaleX->GetValue());
    const float DialogScaleY = NonZero(m_SpinCtrlScaleY->GetValue());

    switch (ApplyMode)
    {
        case ApplyNormal:
        {
            SurfaceInfoT SI = Face->GetSurfaceInfo();

            // Apply current orientation values to the SI.
            // Note that this is equivalent to ObtainSurfaceInfo(Patch, ...), because always SI.TexCoordGenMode == PlaneProj.
            if (m_CurrentTexGenMode == PlaneProj)
            {
                // Our values were picked from a face or patch in PlaneProj mode.
                if (Detail & ApplyScaleX) SI.Scale[0] = 1.0 / (DialogScaleX * Mat->GetWidth());
                if (Detail & ApplyScaleY) SI.Scale[1] = 1.0 / (DialogScaleY * Mat->GetHeight());
            }
            else
            {
                // In this case, the scale values from our spin controls have different meaning
                // and are thus not very useful, so just keep whatever Face had.
                wxASSERT(m_CurrentTexGenMode == MatFit);
            }

            // The meaning of the translation values is the same for all m_CurrentTexGenMode.
            if (Detail & ApplyShiftX) SI.Trans[0] = m_SpinCtrlShiftX->GetValue() / Mat->GetWidth();
            if (Detail & ApplyShiftY) SI.Trans[1] = m_SpinCtrlShiftY->GetValue() / Mat->GetHeight();

            if (Detail & ApplyRotation)
            {
                SI.RotateUVAxes(m_SpinCtrlRotation->GetValue() - SI.Rotate);
                SI.Rotate = m_SpinCtrlRotation->GetValue();
            }

            return SI;
        }

        case ApplyViewAligned:
        {
            // Apply the current material view-aligned to the face.
            SurfaceInfoT SI = ObtainSurfaceInfo(Face, Mat, ApplyNormal, ApplyAll, MsgCounts, ViewWin3D);

            if (ViewWin3D)
            {
                // Augment the SI obtained from ApplyNormal.
                SI.UAxis = ViewWin3D->GetCamera().GetXAxis();
                SI.VAxis = ViewWin3D->GetCamera().GetZAxis();
            }

            return SI;
        }

        case ApplyEdgeAligned:
        {
            // This case is special: It ignores our dialog's settings quasi entirely,
            // using only the first selected face as the (required) reference face.
            if (m_SelectedFaces.Size() < 1)
            {
                if (!MsgCounts.NoCenterFace)
                {
                    wxMessageBox("No face is selected.\n\n"
                        "You must first select a face of a brush that acts as the \"center\".\n"
                        "Then apply this surface information to nearby faces in order to\n"
                        "have their common edges aligned.\n",
                        "Apply Edge Aligned");
                }

                MsgCounts.NoCenterFace++;

                return Face->GetSurfaceInfo();
            }

            const MapFaceT* RefFace = m_SelectedFaces[0].Face;

            if (RefFace == Face) return Face->GetSurfaceInfo();

            // Edge aligned material application works like wrapping a gift in wrapping paper.
            // See the user documentation at http://www.cafu.de/wiki/mapping:cawe:editingtools:editfaceprops for an overview.
            // The key idea is to rotate the surface information (that is, the texture space) of RefFace into Face,
            // around their common edge (the line of plane intersection).
            SurfaceInfoT SI = RefFace->GetSurfaceInfo();

            try
            {
                const Vector3fT OldOrigin =SI.UAxis*SI.Trans[0]/SI.Scale[0] + SI.VAxis*SI.Trans[1]/SI.Scale[1];
                const Vector3fT EdgeDir   =normalize(cross(RefFace->GetPlane().Normal, Face->GetPlane().Normal), 0.0001f);
                const float     Angle     =cf::math::AnglesfT::RadToDeg(acos(dot(RefFace->GetPlane().Normal, Face->GetPlane().Normal)));
                const MatrixT   EdgeRotMat=MatrixT::GetRotateMatrix(Angle, EdgeDir);

                SI.UAxis=EdgeRotMat.Mul0(SI.UAxis);
                SI.VAxis=EdgeRotMat.Mul0(SI.VAxis);

                const Vector3fT EdgeVertex=Plane3fT::GetIntersection(RefFace->GetPlane(), Face->GetPlane(), Plane3fT(EdgeDir, 0.0f));
                const MatrixT   OrigRotMat=MatrixT::GetTranslateMatrix(-EdgeVertex) * EdgeRotMat * MatrixT::GetTranslateMatrix(EdgeVertex);
                const Vector3fT NewOrigin =OrigRotMat.Mul1(OldOrigin);

                SI.Trans[0]=dot(SI.UAxis, NewOrigin) * SI.Scale[0];
                SI.Trans[1]=dot(SI.VAxis, NewOrigin) * SI.Scale[1];
            }
            catch (const DivisionByZeroE&)
            {
                // A DivisionByZeroE is thrown when the rotation of the texture coordinate system was not possible,
                // e.g. because the two faces were coplanar or parallel. Then in turn a rotation is not necessary anyway,
                // and so the exception is just the indication of a special (but stable) case, not an actual error,
                // and nothing needs to be done here.
            }

            SI.WrapTranslations();

            return SI;
        }

        case ApplyProjective:
        {
            // "Raw-copy" the last picked material 1:1 onto the face, including the u- and v-axes.
            SurfaceInfoT SI = ObtainSurfaceInfo(Face, Mat, ApplyNormal, ApplyAll, MsgCounts, ViewWin3D);

            // Augment the SI obtained from ApplyNormal.
            if (m_CurrentTexGenMode == PlaneProj)
            {
                // The previous pick brought good axes.
                SI.UAxis = m_CurrentUAxis;
                SI.VAxis = m_CurrentVAxis;
            }
            else
            {
                // Our m_CurrentUAxis and m_CurrentVAxis are of zero-length and thus invalid.
                // Whatever we do here, it is wrong in the general case and may confuse the
                // user, even if we provided some default/fallback axes or simply did nothing
                // (that is, keep the Face's original axes or figure out something for Patch).
                // Letting the user know about the problem seems to be the best compromise.
                wxASSERT(m_CurrentTexGenMode == MatFit);

                if (!MsgCounts.NoRefPlane)
                {
                    wxMessageBox("There is no reference plane available.\n\n"
                        "The current surface information was picked from a Bezier patch whose\n"
                        "material was set to \"fit\" onto its surface. In this exceptional case,\n"
                        "there is no reference plane available that is needed for projective\n"
                        "material application.\n\n"
                        "The material is therefore applied normally now (non-projective).\n"
                        "You may next wish to choose a different \"Right MB mode\" for applying\n"
                        "the current material, or pick a regular brush face. (The surface\n"
                        "orientation of a brush face provides the required reference plane.)",
                        "Apply Projective");
                }

                MsgCounts.NoRefPlane++;
            }

            return SI;
        }
    }

    return Face->GetSurfaceInfo();
}


/**
 * When a material is applied to the Patch instance, the following combinations of ApplyMode
 * and m_CurrentTexGenMode ("What was last picked into the dialog?") must be considered:
 *
 *                                    Apply to Patch instance using mode:
 *                            +--------+--------------+--------------+-----------+
 *                            | Normal | View Aligned | Edge Aligned | Projected |
 *                +-----------+--------+--------------+--------------+-----------+
 *   last picked  | PlaneProj |   ok   |      ok      |      -/-     |    ok     |
 *   m_CurrTexGM  | MatFit    |   ok   |      ok      |      -/-     |    -/-    |
 *                +-----------+--------+--------------+--------------+-----------+
 *
 * The combinations are the same as for faces (the table structure is the same), but here we
 * have no implementation for "Edge Aligned".
 * Note that while Face->GetSurfaceInfo().TexCoordGenMode is always PlaneProj, with Patch it
 * can be anything (PlaneProj, MatFit or Custom), which complicates the implementation.
 */
SurfaceInfoT EditSurfacePropsDialogT::ObtainSurfaceInfo(const MapBezierPatchT* Patch, EditorMaterialI* Mat, const ApplyModeT ApplyMode, const ApplyDetailT Detail, MsgCountsT& MsgCounts, ViewWindow3DT* ViewWin3D) const
{
    wxASSERT(ApplyMode == ApplyNormal || Detail == ApplyAll);

    if (!Mat)
    {
        // For convenience, allow the caller to pass NULL in place of Patch->GetMaterial().
        Mat = Patch->GetMaterial();
    }

    // Fetch the scale values from the dialog, making sure that they don't get too close to 0.
    const float DialogScaleX = NonZero(m_SpinCtrlScaleX->GetValue());
    const float DialogScaleY = NonZero(m_SpinCtrlScaleY->GetValue());

    switch (ApplyMode)
    {
        case ApplyNormal:
        {
            SurfaceInfoT SI = Patch->GetSurfaceInfo();

            if (SI.TexCoordGenMode == Custom)
            {
                // Get rid of the unmanageable mode Custom.
                // We assume that Detail == ApplyAll. If it is not, just ignore the problem.
                SI.TexCoordGenMode = m_CurrentTexGenMode;

                // The axes are good if m_CurrentTexGenMode == PlaneProj, ignored otherwise.
                SI.UAxis = m_CurrentUAxis;
                SI.VAxis = m_CurrentVAxis;
            }

            // Apply current orientation values to the SI.
            // Note that this is equivalent to ObtainSurfaceInfo(Face, ...), where always SI.TexCoordGenMode == PlaneProj.
            if (m_CurrentTexGenMode == SI.TexCoordGenMode)
            {
                // The tex-gen modes match, thus our picked values are compatible to those of the target Patch.
                if (Detail & ApplyScaleX) SI.Scale[0] = (SI.TexCoordGenMode == MatFit) ? DialogScaleX : 1.0 / (DialogScaleX * Mat->GetWidth());
                if (Detail & ApplyScaleY) SI.Scale[1] = (SI.TexCoordGenMode == MatFit) ? DialogScaleY : 1.0 / (DialogScaleY * Mat->GetHeight());
            }
            else
            {
                // In this case, the scale values from our spin controls have different meaning
                // and are thus not very useful, so just keep whatever Patch had.
                wxASSERT(m_CurrentTexGenMode != SI.TexCoordGenMode);
            }

            // The meaning of the translation values is the same for all m_CurrentTexGenMode.
            if (Detail & ApplyShiftX) SI.Trans[0] = m_SpinCtrlShiftX->GetValue() / Mat->GetWidth();
            if (Detail & ApplyShiftY) SI.Trans[1] = m_SpinCtrlShiftY->GetValue() / Mat->GetHeight();

            if (Detail & ApplyRotation)
            {
                SI.RotateUVAxes(m_SpinCtrlRotation->GetValue() - SI.Rotate);
                SI.Rotate = m_SpinCtrlRotation->GetValue();
            }

            return SI;
        }

        case ApplyViewAligned:
        {
            // Apply the current material view-aligned to the patch.
            SurfaceInfoT SI = ObtainSurfaceInfo(Patch, Mat, ApplyNormal, ApplyAll, MsgCounts, ViewWin3D);

            if (ViewWin3D)
            {
                // Augment the SI obtained from ApplyNormal.
                SI.UAxis = ViewWin3D->GetCamera().GetXAxis();
                SI.VAxis = ViewWin3D->GetCamera().GetZAxis();
            }

            if (m_CurrentTexGenMode == PlaneProj)
            {
                // No matter what the value of SI.TexCoordGenMode was (PlaneProj or MatFit),
                // it is set to PlaneProj and thus we (re-)assign our scale values accordingly.
                SI.TexCoordGenMode = PlaneProj;

                SI.Scale[0] = 1.0 / (DialogScaleX * Mat->GetWidth());
                SI.Scale[1] = 1.0 / (DialogScaleY * Mat->GetHeight());
            }
            else
            {
                if (SI.TexCoordGenMode == PlaneProj)
                {
                    wxASSERT(m_CurrentTexGenMode == MatFit && SI.TexCoordGenMode == PlaneProj);

                    // Our dialog's scale value are incompatible to those of Patch, and thus
                    // not useful. But with m_CurrentTexGenMode != SI.TexCoordGenMode, SI still
                    // has its original scale values of Patch, so we just continue using these.
                    // Thus, there is nothing else to do.
                }
                else
                {
                    wxASSERT(m_CurrentTexGenMode == MatFit && SI.TexCoordGenMode == MatFit);

                    // MatFit everywhere? Well... what else could we do but guess?
                    SI.TexCoordGenMode = PlaneProj;

                    SI.Scale[0] = 0.25f;
                    SI.Scale[1] = 0.25f;
                }
            }

            return SI;
        }

        case ApplyEdgeAligned:
        {
            if (!MsgCounts.NoEdgeAlign)
            {
                wxMessageBox("Edge aligned application onto Bezier patches is not implemented.\n\n"
                    "With Bezier patches, it is generally very difficult (if not impossible)\n"
                    "to find the relevant edge. We will revisit this later, but at this time,\n"
                    "edge aligned application to Bezier Patches is not possible.\n",
                    "Apply Edge Aligned");
            }

            MsgCounts.NoEdgeAlign++;

            // Here is an idea how this could be overcome (or rather, worked around):
            // Find the bounding-box of the Bezier patch, and build the relevant plane
            // from its two longest sides. Done.
            return Patch->GetSurfaceInfo();
        }

        case ApplyProjective:
        {
            // "Raw-copy" the last picked material 1:1 onto the patch, including the u- and v-axes.
            SurfaceInfoT SI = ObtainSurfaceInfo(Patch, Mat, ApplyNormal, ApplyAll, MsgCounts, ViewWin3D);

            // Augment the SI obtained from ApplyNormal.
            if (m_CurrentTexGenMode == PlaneProj)
            {
                // The previous pick brought good axes.
                SI.UAxis = m_CurrentUAxis;
                SI.VAxis = m_CurrentVAxis;

                // No matter what the value of SI.TexCoordGenMode was (PlaneProj or MatFit),
                // it is set to PlaneProj and thus we (re-)assign our scale values accordingly.
                SI.TexCoordGenMode = PlaneProj;

                SI.Scale[0] = 1.0 / (DialogScaleX * Mat->GetWidth());
                SI.Scale[1] = 1.0 / (DialogScaleY * Mat->GetHeight());
            }
            else
            {
                // Our m_CurrentUAxis and m_CurrentVAxis are of zero-length and thus invalid.
                // Whatever we do here, it is wrong in the general case and may confuse the
                // user, even if we provided some default/fallback axes or simply did nothing
                // (that is, keep the Face's original axes or figure out something for Patch).
                // Letting the user know about the problem seems to be the best compromise.
                wxASSERT(m_CurrentTexGenMode == MatFit);

                if (!MsgCounts.NoRefPlane)
                {
                    wxMessageBox("There is no reference plane available.\n\n"
                        "The current surface information was picked from a Bezier patch whose\n"
                        "material was set to \"fit\" onto its surface. In this exceptional case,\n"
                        "there is no reference plane available that is needed for projective\n"
                        "material application.\n\n"
                        "The material is therefore applied normally now (non-projective).\n"
                        "You may next wish to choose a different \"Right MB mode\" for applying\n"
                        "the current material, or pick a regular brush face. (The surface\n"
                        "orientation of a brush face provides the required reference plane.)",
                        "Apply Projective");
                }

                MsgCounts.NoRefPlane++;
            }

            return SI;
        }
    }

    return Patch->GetSurfaceInfo();
}


void EditSurfacePropsDialogT::UpdateVectorInfo()
{
    MaterialXInfo->SetLabel(wxString::Format("( %f | %f | %f )", m_CurrentUAxis.x, m_CurrentUAxis.y, m_CurrentUAxis.z));
    MaterialYInfo->SetLabel(wxString::Format("( %f | %f | %f )", m_CurrentVAxis.x, m_CurrentVAxis.y, m_CurrentVAxis.z));
}


void EditSurfacePropsDialogT::UpdateAfterSelChange()
{
    // It is certainly possible to implement this a bit more efficiently, e.g. by cutting the
    // loops short whenever both aligned and non-aligned faces have been found, but the code
    // as-is is short and easy to understand, and still in the same order of magnitude.
    unsigned int WorldAlignCount = 0;
    unsigned int FaceAlignCount  = 0;

    for (unsigned int FaceNr = 0; FaceNr < m_SelectedFaces.Size(); FaceNr++)
    {
        if (m_SelectedFaces[FaceNr].Face->IsUVSpaceWorldAligned()) WorldAlignCount++;
        if (m_SelectedFaces[FaceNr].Face->IsUVSpaceFaceAligned() ) FaceAlignCount++;
    }

    if (m_SelectedFaces.Size() == 0)
    {
        m_wrtWorldAxesText->Disable();
        m_wrtWorldAxesInfo->Disable();
        m_wrtWorldAxesButton->Disable();

        m_wrtWorldAxesInfo->SetLabel("none");
    }
    else
    {
        m_wrtWorldAxesText->Enable();
        m_wrtWorldAxesInfo->Enable();
        m_wrtWorldAxesButton->Enable();

        // Set the "Is aligned wrt. the world axes?" info text.
             if (WorldAlignCount ==                      0) m_wrtWorldAxesInfo->SetLabel("none");
        else if (WorldAlignCount == m_SelectedFaces.Size()) m_wrtWorldAxesInfo->SetLabel("all");
        else                                                m_wrtWorldAxesInfo->SetLabel("mixed");
    }

    if (m_SelectedFaces.Size() == 0)
    {
        m_wrtFacePlaneText->Disable();
        m_wrtFacePlaneInfo->Disable();
        m_wrtFacePlaneButton->Disable();

        m_wrtFacePlaneInfo->SetLabel("none");
    }
    else
    {
        m_wrtFacePlaneText->Enable();
        m_wrtFacePlaneInfo->Enable();
        m_wrtFacePlaneButton->Enable();

        // Set the "Is aligned wrt. the face plane?" info text.
             if (FaceAlignCount ==                      0) m_wrtFacePlaneInfo->SetLabel("none");
        else if (FaceAlignCount == m_SelectedFaces.Size()) m_wrtFacePlaneInfo->SetLabel("all");
        else                                               m_wrtFacePlaneInfo->SetLabel("mixed");
    }

    // Also deal with the "Treat multiple as one" checkbox. We better don't change its state
    // (checked/unchecked), because the user may wish to keep it even across selection changes.
    // Disabling it whenever the OnButtonAlign() handler cannot (fully) make use of it should
    // be enough to let the user know that some limitations apply.
    m_CheckBoxTreatMultipleAsOne->Enable(m_SelectedFaces.Size() > 1 && m_SelectedPatches.Size() == 0);
}


/**********************/
/*** Event Handlers ***/
/**********************/

void EditSurfacePropsDialogT::OnSpinCtrlValueChanged(wxSpinDoubleEvent& Event)
{
    ApplyDetailT Detail = ApplyNone;

    // Determine the proper value for Detail based on the spin control whose value changed.
    switch (Event.GetId())
    {
        case ID_SPINCTRL_SCALE_X:  Detail = ApplyScaleX;   break;
        case ID_SPINCTRL_SCALE_Y:  Detail = ApplyScaleY;   break;
        case ID_SPINCTRL_SHIFT_X:  Detail = ApplyShiftX;   break;
        case ID_SPINCTRL_SHIFT_Y:  Detail = ApplyShiftY;   break;
        case ID_SPINCTRL_ROTATION: Detail = ApplyRotation; break;
    }

    // The value in one of the spin controls changed, and thus the orientation of the material on the selected face(s)/patch(es).
    MsgCountsT        MsgCounts;
    ArrayT<CommandT*> SurfaceCommands;

    for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
    {
        EditorMaterialI*   Material = NULL;     // The material doesn't change.
        const SurfaceInfoT SI = ObtainSurfaceInfo(m_SelectedFaces[FaceNr].Face, Material, ApplyNormal, Detail, MsgCounts);

        SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, m_SelectedFaces[FaceNr].Brush, m_SelectedFaces[FaceNr].FaceIndex, SI, Material));
    }

    for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); PatchNr++)
    {
        EditorMaterialI*   Material = NULL;     // The material doesn't change.
        const SurfaceInfoT SI = ObtainSurfaceInfo(m_SelectedPatches[PatchNr], Material, ApplyNormal, Detail, MsgCounts);

        SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(*m_MapDoc, m_SelectedPatches[PatchNr], SI, Material));
    }

    if (SurfaceCommands.Size()>0)
    {
        wxString Name="";

        switch (Event.GetId())
        {
            case ID_SPINCTRL_SCALE_X:  Name="Apply scale x";  break;
            case ID_SPINCTRL_SCALE_Y:  Name="Apply scale y";  break;
            case ID_SPINCTRL_SHIFT_X:  Name="Apply shift x";  break;
            case ID_SPINCTRL_SHIFT_Y:  Name="Apply shift y";  break;
            case ID_SPINCTRL_ROTATION: Name="Apply rotation"; break;
        }

        CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, Name);

        m_MapDoc->CompatSubmitCommand(Macro);
    }
}


void EditSurfacePropsDialogT::OnButtonAlign(wxCommandEvent& Event)
{
    if (m_MapDoc==0) return;

    if (m_SelectedPatches.Size() > 0 && Event.GetId() != ID_BUTTON_ALIGN2FITFACE)
    {
        wxMessageBox("With Bezier patches, only \"Fit\" can be used.\n\n"
            "It is usually not possible with Bezier patches to compute the required\n"
            "reference point for the other alignment options. However, after \"Fit\"\n"
            "you can use the orientation controls to fine-tune the result.",
            "Fit material to surface");

        return;
    }

    if (m_SelectedFaces.Size()>0)
    {
        ArrayT<Vector3fT> CombinedVertices;
        ArrayT<CommandT*> SurfaceCommands;

        // If all the selected faces are to be treated "as one", prepare the array with the union of their vertices.
        if (m_CheckBoxTreatMultipleAsOne->IsChecked())
            for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
                CombinedVertices.PushBack(m_SelectedFaces[FaceNr].Face->GetVertices());

        for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        {
            const ArrayT<Vector3fT>& Vertices=m_CheckBoxTreatMultipleAsOne->IsChecked() ? CombinedVertices : m_SelectedFaces[FaceNr].Face->GetVertices();
            SurfaceInfoT             SI      =m_SelectedFaces[FaceNr].Face->GetSurfaceInfo();

            switch (Event.GetId())
            {
                case ID_BUTTON_ALIGN2TOP:     SI.AlignMaterial("top",    Vertices); break;
                case ID_BUTTON_ALIGN2BOTTOM:  SI.AlignMaterial("bottom", Vertices); break;
                case ID_BUTTON_ALIGN2LEFT:    SI.AlignMaterial("left",   Vertices); break;
                case ID_BUTTON_ALIGN2RIGHT:   SI.AlignMaterial("right",  Vertices); break;
                case ID_BUTTON_ALIGN2CENTER:  SI.AlignMaterial("center", Vertices); break;
                case ID_BUTTON_ALIGN2FITFACE: SI.AlignMaterial("fit",    Vertices); break;
            }

            SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, m_SelectedFaces[FaceNr].Brush, m_SelectedFaces[FaceNr].FaceIndex, SI, NULL));
        }

        m_MapDoc->CompatSubmitCommand(new CommandMacroT(SurfaceCommands, "Align material"));
    }

    if (m_SelectedPatches.Size() > 0)
    {
        wxASSERT(Event.GetId() == ID_BUTTON_ALIGN2FITFACE);

        ArrayT<CommandT*> SurfaceCommands;
        EditorMaterialI* Material = NULL;   // The material doesn't change.
        SurfaceInfoT SI;
        SI.TexCoordGenMode = MatFit;

        for (unsigned long PatchNr = 0; PatchNr < m_SelectedPatches.Size(); PatchNr++)
        {
            SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(*m_MapDoc, m_SelectedPatches[PatchNr], SI, Material));
        }

        m_MapDoc->CompatSubmitCommand(new CommandMacroT(SurfaceCommands, "Align material"));
    }
}


// The buttons "wrt. world axes" and "wrt. face plane" are (besides "Apply Projective") the
// only way to reset a face's u/v-axes. Both buttons are disabled by UpdateAfterSelChange()
// whenever no faces are selected.
void EditSurfacePropsDialogT::OnButtonAlignWrtAxes(wxCommandEvent& Event)
{
    wxASSERT(Event.GetId() == ID_BUTTON_ALIGN_WRT_WORLD ||
             Event.GetId() == ID_BUTTON_ALIGN_WRT_FACE);

    if (!m_MapDoc) return;

    ArrayT<CommandT*> SurfaceCommands;

    for (unsigned int FaceNr = 0; FaceNr < m_SelectedFaces.Size(); FaceNr++)
    {
        SurfaceInfoT SI = m_SelectedFaces[FaceNr].Face->GetSurfaceInfo();

        SI.ResetUVAxes(
            m_SelectedFaces[FaceNr].Face->GetPlane(),
            Event.GetId() == ID_BUTTON_ALIGN_WRT_FACE);

        SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(
            *m_MapDoc,
            m_SelectedFaces[FaceNr].Brush,
            m_SelectedFaces[FaceNr].FaceIndex,
            SI,
            NULL));
    }

    if (SurfaceCommands.Size() > 0)
    {
        CommandMacroT* Macro = new CommandMacroT(
            SurfaceCommands,
            Event.GetId() == ID_BUTTON_ALIGN_WRT_FACE
                ? "Align material wrt. face plane"
                : "Align material wrt. world axes");

        m_MapDoc->CompatSubmitCommand(Macro);
    }

    UpdateAfterSelChange();
}


void EditSurfacePropsDialogT::OnCheckBoxTreatMultipleAsOne(wxCommandEvent& Event)
{
    // Afaics, no need for any action here, other code will just query the checkbox's value.
}


// When another material has been selected, this handler updates the material preview bitmap
// sets the "Size: a x b" text, rearranges the contents of the ChoiceCurrentMat in MRU order
// and applies the chosen material to the currently selected faces and Bezier patches.
void EditSurfacePropsDialogT::OnSelChangeCurrentMat(wxCommandEvent& Event)
{
    const int Index = ChoiceCurrentMat->GetSelection();

    wxASSERT(Index != -1);
    if (Index == -1) return;

    EditorMaterialI* CurrentMaterial=(EditorMaterialI*)ChoiceCurrentMat->GetClientData(Index);

    if (Index!=0)
    {
        ChoiceCurrentMat->Delete(Index);
        ChoiceCurrentMat->Insert(CurrentMaterial->GetName(), 0, CurrentMaterial);
        ChoiceCurrentMat->SetSelection(0);
    }

    const int  w  =CurrentMaterial->GetWidth ();
    const int  h  =CurrentMaterial->GetHeight();
    const int  Max=w>h ? w : h;
    const bool Fit=(w<=PREVIEW_BITMAP_SIZE && h<=PREVIEW_BITMAP_SIZE);

    wxBitmap PreviewBitmap=Fit ? wxBitmap(CurrentMaterial->GetImage()) : wxBitmap(CurrentMaterial->GetImage().Scale(w*PREVIEW_BITMAP_SIZE/Max, h*PREVIEW_BITMAP_SIZE/Max));

    m_BitmapCurrentMat->SetBitmap(PreviewBitmap);
    m_BitmapCurrentMat->Refresh();

    StaticTextCurrentMatSize->SetLabel(wxString::Format("Size: %ix%i", CurrentMaterial->GetWidth(), CurrentMaterial->GetHeight()));
    m_MapDoc->GetGameConfig()->GetMatMan().SetDefaultMaterial(CurrentMaterial);

    assert(Event.GetExtraLong()==-1 || Event.GetExtraLong()==0);

    // Apply the new material to the selection (unless ExtraLong indicates that it is not desired).
    if (Event.GetExtraLong() != -1)
    {
        MsgCountsT        MsgCounts;
        ArrayT<CommandT*> SurfaceCommands;

        for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        {
            // SI must be recomputed as well, because the material's width and height may have changed.
            const SurfaceInfoT SI = ObtainSurfaceInfo(m_SelectedFaces[FaceNr].Face, CurrentMaterial, ApplyNormal, ApplyAll, MsgCounts);

            SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(
                *m_MapDoc,
                m_SelectedFaces[FaceNr].Brush,
                m_SelectedFaces[FaceNr].FaceIndex,
                SI,
                CurrentMaterial));
        }

        for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); PatchNr++)
        {
            // SI must be recomputed as well, because the material's width and height may have changed.
            const SurfaceInfoT SI = ObtainSurfaceInfo(m_SelectedPatches[PatchNr], CurrentMaterial, ApplyNormal, ApplyAll, MsgCounts);

            SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(
                *m_MapDoc,
                m_SelectedPatches[PatchNr],
                SI,
                CurrentMaterial));
        }

        if (SurfaceCommands.Size()>0)
        {
            CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Apply material");

            m_MapDoc->CompatSubmitCommand(Macro);
        }
    }
}


void EditSurfacePropsDialogT::OnButtonBrowseMats(wxCommandEvent& Event)
{
    int Index=ChoiceCurrentMat->GetSelection();

    MaterialBrowser::DialogT MatBrowser(this,
        m_MapDoc->GetAdapter(),
        MaterialBrowser::ConfigT().InitialMaterial(Index!=-1 ? (EditorMaterialI*)ChoiceCurrentMat->GetClientData(Index) : NULL));

    if (MatBrowser.ShowModal()!=wxID_OK) return;

    EditorMaterialI* Mat=MatBrowser.GetCurrentMaterial();
    if (Mat==NULL) return;

    Index=ChoiceCurrentMat->FindString(Mat->GetName());
    if (Index==-1)
    {
        // If there are already 10 or more items in the control, delete the last one.
        while (ChoiceCurrentMat->GetCount()>=10)
            ChoiceCurrentMat->Delete(ChoiceCurrentMat->GetCount()-1);

        // Material not found in list, add it now.
        ChoiceCurrentMat->Append(Mat->GetName(), Mat);
        Index=ChoiceCurrentMat->GetCount()-1;
    }

    ChoiceCurrentMat->SetSelection(Index);

    wxCommandEvent CE;
    OnSelChangeCurrentMat(CE);  // This will apply the material to the currently selected faces, bezier patches, etc.
}


void EditSurfacePropsDialogT::OnButtonReplaceMats(wxCommandEvent& Event)
{
    if (m_MapDoc)
    {
        ReplaceMaterialsDialogT ReplaceMatsDlg(m_MapDoc->GetSelection().Size()>0, *m_MapDoc, m_MapDoc->GetGameConfig()->GetMatMan().GetDefaultMaterial()->GetName());
        ReplaceMatsDlg.ShowModal();
    }
}


void EditSurfacePropsDialogT::OnCheckBoxHideSelMask(wxCommandEvent& Event)
{
    // Do nothing here, faces and bezierpatches read the selection state of this checkbox directly.

    // FIXME: Do we really need this? First of all its more of a temporary visual change, so a MODIFIED message
    // seems somewhat exaggerated. Second the selection mask is only rendered in the 3D view (which doesn't react
    // on observer message but update itself regulary on idle events).
    ArrayT<MapElementT*> UpdateObjects;

    for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        UpdateObjects.PushBack(m_SelectedFaces[FaceNr].Brush);

    for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); PatchNr++)
        UpdateObjects.PushBack(m_SelectedPatches[PatchNr]);

    m_MapDoc->UpdateAllObservers_Modified(UpdateObjects, MEMD_SURFACE_INFO_CHANGED);
}


void EditSurfacePropsDialogT::OnSelChangeRightMB(wxCommandEvent& Event)
{
    // Afaics, no need for any action here.
    // Those who are interested in the selected value of the ChoiceBox should query it directly.
}


ArrayT<EditorMaterialI*> EditSurfacePropsDialogT::GetMRUMaterials() const
{
    ArrayT<EditorMaterialI*> MRUMaterials;

    for (unsigned long i=0; i<ChoiceCurrentMat->GetCount(); i++)
        MRUMaterials.PushBack((EditorMaterialI*)ChoiceCurrentMat->GetClientData(i));

    return MRUMaterials;
}
