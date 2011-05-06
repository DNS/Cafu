/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "Camera.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "DialogReplaceMaterials.hpp"
#include "GameConfig.hpp"
#include "CommandHistory.hpp"
#include "ParentFrame.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "MapDocument.hpp"
#include "MapFace.hpp"
#include "MapBezierPatch.hpp"
#include "MapBrush.hpp"
#include "MapTerrain.hpp"
#include "EditorMaterial.hpp"
#include "EditorMaterialManager.hpp"
#include "Tool.hpp"
#include "ToolManager.hpp"
#include "ToolbarMaterials.hpp"

#include "MapCommands/UpdateSurface.hpp"
#include "MaterialBrowser/DocAccess.hpp"
#include "MaterialBrowser/MaterialBrowserDialog.hpp"

#include "wx/image.h"
#include "wx/confbase.h"

#if defined(_WIN32) && defined(_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


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
    EVT_CHECKBOX      (EditSurfacePropsDialogT::ID_CHECKBOX_ALIGN_WRT_WORLD,       EditSurfacePropsDialogT::OnCheckBoxAlignWorld)
    EVT_CHECKBOX      (EditSurfacePropsDialogT::ID_CHECKBOX_ALIGN_WRT_FACE,        EditSurfacePropsDialogT::OnCheckBoxAlignFace)
    EVT_CHECKBOX      (EditSurfacePropsDialogT::ID_CHECKBOX_TREAT_MULTIPLE_AS_ONE, EditSurfacePropsDialogT::OnCheckBoxTreatMultipleAsOne)
    EVT_CHOICE        (EditSurfacePropsDialogT::ID_CHOICE_CURRENT_MAT,             EditSurfacePropsDialogT::OnSelChangeCurrentMat)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_BROWSE_MATS,             EditSurfacePropsDialogT::OnButtonBrowseMats)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_REPLACE_MATS,            EditSurfacePropsDialogT::OnButtonReplaceMats)
    EVT_CHECKBOX      (EditSurfacePropsDialogT::ID_CHECKBOX_HIDE_SEL_MASK,         EditSurfacePropsDialogT::OnCheckBoxHideSelMask)
    EVT_CHOICE        (EditSurfacePropsDialogT::ID_CHOICE_RIGHT_MB_MODE,           EditSurfacePropsDialogT::OnSelChangeRightMB)
    EVT_BUTTON        (EditSurfacePropsDialogT::ID_BUTTON_APPLY_TO_ALL_SELECTED,   EditSurfacePropsDialogT::OnButtonApplyToAllSelected)
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
      CheckBoxAlignWrtWorld(NULL),
      CheckBoxAlignWrtFace(NULL),
      CheckBoxTreatMultipleAsOne(NULL),
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

    CheckBoxAlignWrtWorld=new wxCheckBox(this, ID_CHECKBOX_ALIGN_WRT_WORLD, wxT("wrt. World axes"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
    item25->Add(CheckBoxAlignWrtWorld, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    CheckBoxAlignWrtFace=new wxCheckBox(this, ID_CHECKBOX_ALIGN_WRT_FACE, wxT("wrt. Face plane"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE);
    item25->Add(CheckBoxAlignWrtFace, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    item25->Add( 15, 15, 0, wxALIGN_CENTER, 5 );

    CheckBoxTreatMultipleAsOne= new wxCheckBox(this, ID_CHECKBOX_TREAT_MULTIPLE_AS_ONE, wxT("Treat multiple as one"), wxDefaultPosition, wxDefaultSize, 0 );
    item25->Add(CheckBoxTreatMultipleAsOne, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item16->Add( item25, 0, wxGROW|wxALIGN_CENTER_HORIZONTAL, 5 );

    item0->Add( item16, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticBox *item55 = new wxStaticBox(this, -1, wxT("Tool Mode") );
    wxStaticBoxSizer *item54 = new wxStaticBoxSizer( item55, wxVERTICAL );

    CheckBoxHideSelMask= new wxCheckBox(this, ID_CHECKBOX_HIDE_SEL_MASK, wxT("Hide Selection Overlay"), wxDefaultPosition, wxDefaultSize, 0 );
    item54->Add(CheckBoxHideSelMask, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxFlexGridSizer *item57 = new wxFlexGridSizer( 3, 0, 0 );
    item57->AddGrowableCol( 1 );

    wxStaticText *item60 = new wxStaticText(this, -1, wxT("Right MB mode:"), wxDefaultPosition, wxDefaultSize, 0 );
    item57->Add( item60, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM, 5 );

    ChoiceRightMBMode= new wxChoice(this, ID_CHOICE_RIGHT_MB_MODE, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
    ChoiceRightMBMode->Append("Apply Normal"       , (void*)ApplyNormal     );
    ChoiceRightMBMode->Append("Apply View Aligned" , (void*)ApplyViewAligned);
    ChoiceRightMBMode->Append("Apply Edge Aligned" , (void*)ApplyEdgeAligned);
    ChoiceRightMBMode->Append("Apply Projective"   , (void*)ApplyProjective );
    ChoiceRightMBMode->SetSelection(0);
    item57->Add(ChoiceRightMBMode, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM, 5 );

    wxButton *item62 = new wxButton(this, ID_BUTTON_APPLY_TO_ALL_SELECTED, wxT("to all Sel."), wxDefaultPosition, wxSize(55, -1), 0 );
    item57->Add( item62, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    item54->Add( item57, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item54, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);


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


void EditSurfacePropsDialogT::NotifySubjectDies(SubjectT* Subject)
{
    wxASSERT(Subject==m_MapDoc);

    m_MapDoc=NULL;
}


bool EditSurfacePropsDialogT::Show(bool show)
{
    if (show)
    {
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
}


void EditSurfacePropsDialogT::ToggleClick(MapElementT* Object, unsigned long FaceIndex)
{
    if (Object==NULL) return;

    MapBrushT*       Brush=dynamic_cast<MapBrushT*>(Object);
    MapBezierPatchT* Patch=dynamic_cast<MapBezierPatchT*>(Object);

    if (Brush!=NULL)
    {
        if (FaceIndex==ALL_FACES)
        {
            // Solve the problem recursively.
            for (unsigned long FaceNr=0; FaceNr<Brush->GetFaces().Size(); FaceNr++)
                ToggleClick(Object, FaceNr);

            return;
        }

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


        unsigned long WorldAlignCount=0;
        unsigned long FaceAlignCount =0;

        for (FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        {
            if (m_SelectedFaces[FaceNr].Face->IsUVSpaceWorldAligned()) WorldAlignCount++;
            if (m_SelectedFaces[FaceNr].Face->IsUVSpaceFaceAligned() ) FaceAlignCount ++;
        }

        // Set the state of the "Is aligned wrt. the world axes?" checkbox.
             if (WorldAlignCount==                     0) CheckBoxAlignWrtWorld->Set3StateValue(wxCHK_UNCHECKED);
        else if (WorldAlignCount==m_SelectedFaces.Size()) CheckBoxAlignWrtWorld->Set3StateValue(wxCHK_CHECKED);
        else                                              CheckBoxAlignWrtWorld->Set3StateValue(wxCHK_UNDETERMINED);

        // Set the state of the "Is aligned wrt. the face plane?" checkbox.
             if (FaceAlignCount==                     0) CheckBoxAlignWrtFace->Set3StateValue(wxCHK_UNCHECKED);
        else if (FaceAlignCount==m_SelectedFaces.Size()) CheckBoxAlignWrtFace->Set3StateValue(wxCHK_CHECKED);
        else                                             CheckBoxAlignWrtFace->Set3StateValue(wxCHK_UNDETERMINED);
    }

    if (Patch!=NULL)
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


    ArrayT<MapElementT*> UpdateObjects;
    UpdateObjects.PushBack(Object);

    m_MapDoc->UpdateAllObservers_Modified(UpdateObjects, MEMD_SURFACE_INFO_CHANGED);
}


void EditSurfacePropsDialogT::ApplyClick(ViewWindow3DT& ViewWin3D, MapElementT* Object, unsigned long FaceIndex)
{
    if (Object==NULL) return;

    const RightMBClickModeT ApplyMode=(RightMBClickModeT)(unsigned long)ChoiceRightMBMode->GetClientData(ChoiceRightMBMode->GetSelection());

    MapBrushT*       Brush  =dynamic_cast<MapBrushT*>(Object);
    MapBezierPatchT* Patch  =dynamic_cast<MapBezierPatchT*>(Object);
    MapTerrainT*     Terrain=dynamic_cast<MapTerrainT*>(Object);

    if (ApplyMode==ApplyProjective && m_CurrentTexGenMode!=PlaneProj)   // Only apply projective if there is a projection plane.
    {
        wxMessageBox("When the texture-coordinate mode is Fit or Custom,\n"
            "there is no reference plane for projective material application.\n\n"
            "Please use another apply mode for the right mouse button,\n"
            "or pick a regular brush face in order to apply its material projectively.");
        return;
    }

    ArrayT<CommandT*> SurfaceCommands;

    if (Brush!=NULL)
    {
        if (FaceIndex==ALL_FACES) // Apply on all faces of Brush.
        {
            for (unsigned long FaceNr=0; FaceNr<Brush->GetFaces().Size(); FaceNr++)
            {
                EditorMaterialI* Material=NULL;
                SurfaceInfoT     SI=Brush->GetFaces()[FaceNr].GetSurfaceInfo();

                SetSurfaceInfo(&Brush->GetFaces()[FaceNr], SI, &Material, ApplyMode, ApplyAll, &ViewWin3D);
                SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, Brush, FaceNr, SI, Material));
            }
        }
        else // Just apply on face at FaceIndex.
        {
            EditorMaterialI* Material=NULL;
            SurfaceInfoT     SI=Brush->GetFaces()[FaceIndex].GetSurfaceInfo();

            SetSurfaceInfo(&Brush->GetFaces()[FaceIndex], SI, &Material, ApplyMode, ApplyAll, &ViewWin3D);
            SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, Brush, FaceIndex, SI, Material));
        }
    }

    if (Patch!=NULL)
    {
        EditorMaterialI* Material=NULL;
        SurfaceInfoT     SI=Patch->GetSurfaceInfo();

        SetSurfaceInfo(Patch, SI, &Material, ApplyMode, ApplyAll, &ViewWin3D);
        SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(*m_MapDoc, Patch, SI, Material));
    }

    if (Terrain!=NULL)
    {
        EditorMaterialI* Material=(EditorMaterialI*)ChoiceCurrentMat->GetClientData(ChoiceCurrentMat->GetSelection());

        SurfaceCommands.PushBack(new CommandUpdateSurfaceTerrainT(*m_MapDoc, Terrain, Material));
    }

    if (SurfaceCommands.Size()>0)
    {
        CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Right-click apply");

        m_MapDoc->GetHistory().SubmitCommand(Macro);
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

        m_CurrentTexGenMode=SI.TexCoordGenMode;
        m_CurrentUAxis=SI.UAxis;
        m_CurrentVAxis=SI.VAxis;

        UpdateVectorInfo();
    }

    if (Patch!=NULL)
    {
        const SurfaceInfoT& SI=Patch->GetSurfaceInfo();

        if (SI.TexCoordGenMode==Custom)
        {
            wxMessageBox("The texture information on this Bezier patch is in a custom format that cannot be picked for use in the dialog.\n"
                "You can fix the problem by assigning new texture information (using the right mouse button) to the patch first.");
            return;
        }

        m_SpinCtrlScaleX  ->SetValue((SI.TexCoordGenMode==MatFit) ? SI.Scale[0] : 1.0/SI.Scale[0]/Patch->GetMaterial()->GetWidth());
        m_SpinCtrlScaleY  ->SetValue((SI.TexCoordGenMode==MatFit) ? SI.Scale[1] : 1.0/SI.Scale[1]/Patch->GetMaterial()->GetHeight());
        m_SpinCtrlShiftX  ->SetValue(SI.Trans[0]*Patch->GetMaterial()->GetWidth());
        m_SpinCtrlShiftY  ->SetValue(SI.Trans[1]*Patch->GetMaterial()->GetHeight());
        m_SpinCtrlRotation->SetValue(SI.Rotate);
        m_TexGenModeInfo  ->SetLabel("");   // The proper value is set below.

        Material=Patch->GetMaterial();

        m_CurrentTexGenMode=SI.TexCoordGenMode;
        m_CurrentUAxis=SI.UAxis;
        m_CurrentVAxis=SI.VAxis;

        UpdateVectorInfo();

        switch (m_CurrentTexGenMode)
        {
            case Custom: m_TexGenModeInfo->SetLabel("Mode: Custom"); break;
            case MatFit: m_TexGenModeInfo->SetLabel("Mode: Fit");    break;
            default:     m_TexGenModeInfo->SetLabel("");             break;
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

void EditSurfacePropsDialogT::SetSurfaceInfo(const MapFaceT* Face, SurfaceInfoT& SI, EditorMaterialI** Material, const RightMBClickModeT ApplyMode, const ApplySettingT Setting, ViewWindow3DT* ViewWin3D) const
{
    // If this method is called without ApplyNormal as ApplyMode, the ApplySetting should have it's default value
    // (ApplyAll) since it isn't used anyway in those apply modes directly.
    // An important consequence is, that the ApplyMaterial flag is set in all the cases, so we can be sure that the
    // material will always be set, if the ApplyMode is != ApplyNormal.
    if (ApplyMode!=ApplyNormal) assert(Setting==ApplyAll);

    if (m_MapDoc==NULL) return;

    // Update the material (commonly done in all ApplyModes).
    const int CurrentMatIndex=ChoiceCurrentMat->GetSelection();

    if (CurrentMatIndex!=-1 && Face->GetMaterial()!=NULL)
        if (ApplyMode!=ApplyNormal || (Setting & ApplyMaterial))    // See comment above for rationale.
            if (wxStricmp(Face->GetMaterial()->GetName(), ((EditorMaterialI*)ChoiceCurrentMat->GetClientData(CurrentMatIndex))->GetName())!=0)
                *Material=(EditorMaterialI*)ChoiceCurrentMat->GetClientData(CurrentMatIndex);

    // Scale values currently stored in the dialog.
    float DialogScaleX=m_SpinCtrlScaleX->GetValue();
    float DialogScaleY=m_SpinCtrlScaleY->GetValue();

    // See that scale values are always 0.01 or greater if positive or -0.01 or smaller if negative.
    if(DialogScaleX>=0.0f && DialogScaleX < 0.01f) DialogScaleX= 0.01f;
    if(DialogScaleX< 0.0f && DialogScaleX >-0.01f) DialogScaleX=-0.01f;
    if(DialogScaleY>=0.0f && DialogScaleY < 0.01f) DialogScaleY= 0.01f;
    if(DialogScaleY< 0.0f && DialogScaleY <-0.01f) DialogScaleY=-0.01f;

    switch (ApplyMode)
    {
        case ApplyNormal:
        {
            // Apply the normal "orientation" settings depending on the choosen ApplySetting.
            if (Setting & ApplyScaleX) SI.Scale[0]=1.0/(DialogScaleX * Face->GetMaterial()->GetWidth());
            if (Setting & ApplyScaleY) SI.Scale[1]=1.0/(DialogScaleY * Face->GetMaterial()->GetHeight());
            if (Setting & ApplyShiftX) SI.Trans[0]=m_SpinCtrlShiftX->GetValue() / Face->GetMaterial()->GetWidth();
            if (Setting & ApplyShiftY) SI.Trans[1]=m_SpinCtrlShiftY->GetValue() / Face->GetMaterial()->GetHeight();
            if (Setting & ApplyRotation)
            {
                SI.RotateUVAxes(m_SpinCtrlRotation->GetValue() - Face->GetSurfaceInfo().Rotate);
                SI.Rotate=m_SpinCtrlRotation->GetValue();
            }

            break;
        }

        case ApplyViewAligned:
        {
            // Apply the current material view aligned to the face.
            if (ViewWin3D==NULL) break;

            SI.UAxis=ViewWin3D->GetCamera().GetXAxis();
            SI.VAxis=ViewWin3D->GetCamera().GetZAxis();

            // Set rotation to zero, so the whole rotation value from the dialog is applied below.
            SI.Rotate=0.0f;

            SetSurfaceInfo(Face, SI, Material, ApplyNormal, ApplySettingT(ApplyAll & ~ApplyMaterial), ViewWin3D);
            break;
        }

        case ApplyEdgeAligned:
        {
            // Take the last selected face as the required reference face.
            if (m_SelectedFaces.Size()<1) break;

            const MapFaceT* RefFace=m_SelectedFaces[m_SelectedFaces.Size()-1].Face;
            if (RefFace==Face) break;

            // Edge aligned material application works like wrapping a gift in wrapping paper.
            // See the user documentation at http://www.cafu.de/wiki/mapping:cawe:editingtools:editfaceprops for an overview.
            // The key idea is to rotate the surface information (that is, the texture space) of RefFace into Face,
            // around their common edge (the line of plane intersection).
            SI=RefFace->GetSurfaceInfo();

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
            break;
        }

        case ApplyProjective:
        {
            // "Raw-copy" the last picked material 1:1 onto the face, including the u- and v-axes.
            SI.UAxis=m_CurrentUAxis;
            SI.VAxis=m_CurrentVAxis;

            // Set rotation to zero, so the whole rotation value from the dialog is applied below.
            SI.Rotate=0.0f;

            SetSurfaceInfo(Face, SI, Material, ApplyNormal, ApplySettingT(ApplyAll & ~ApplyMaterial), ViewWin3D);
            break;
        }
    }
}


void EditSurfacePropsDialogT::SetSurfaceInfo(const MapBezierPatchT* Patch, SurfaceInfoT& SI, EditorMaterialI** Material, const RightMBClickModeT ApplyMode, const ApplySettingT Setting, ViewWindow3DT* ViewWin3D) const
{
    // If this method is called without ApplyNormal as ApplyMode, the ApplySetting should have it's default value
    // (ApplyAll) since it isn't used anyway in those apply modes directly.
    // An important consequence is, that the ApplyMaterial flag is set in all the cases, so we can be sure that the
    // material will always be set, if the ApplyMode is != ApplyNormal.
    if (ApplyMode!=ApplyNormal) assert(Setting==ApplyAll);

    if (m_MapDoc==NULL) return;

    // Update the material (commonly done in all ApplyModes).
    const int CurrentMatIndex=ChoiceCurrentMat->GetSelection();

    if (CurrentMatIndex!=-1 && Patch->GetMaterial()!=NULL && ApplyMode!=ApplyEdgeAligned) // Edge aligned doesn't work on bezier patches.
        if (ApplyMode!=ApplyNormal || (Setting & ApplyMaterial))    // See comment above for rationale.
            if (wxStricmp(Patch->GetMaterial()->GetName(), ((EditorMaterialI*)ChoiceCurrentMat->GetClientData(CurrentMatIndex))->GetName())!=0)
                *Material=(EditorMaterialI*)ChoiceCurrentMat->GetClientData(CurrentMatIndex);

    float DialogScaleX=m_SpinCtrlScaleX->GetValue();
    float DialogScaleY=m_SpinCtrlScaleY->GetValue();

    // See that scale values are always 0.01 or greater if positive or -0.01 or smaller if negative.
    if(DialogScaleX>=0.0f && DialogScaleX < 0.01f) DialogScaleX= 0.01f;
    if(DialogScaleX< 0.0f && DialogScaleX >-0.01f) DialogScaleX=-0.01f;
    if(DialogScaleY>=0.0f && DialogScaleY < 0.01f) DialogScaleY= 0.01f;
    if(DialogScaleY< 0.0f && DialogScaleY <-0.01f) DialogScaleY=-0.01f;

    switch (ApplyMode)
    {
        case ApplyNormal:
        {
            // Apply current orientation values to the patch.
            if (Setting & ApplyScaleX)   SI.Scale[0]=(SI.TexCoordGenMode==MatFit) ? DialogScaleX : 1.0/(DialogScaleX*Patch->GetMaterial()->GetWidth());
            if (Setting & ApplyScaleY)   SI.Scale[1]=(SI.TexCoordGenMode==MatFit) ? DialogScaleY : 1.0/(DialogScaleY*Patch->GetMaterial()->GetHeight());
            if (Setting & ApplyShiftX)   SI.Trans[0]=m_SpinCtrlShiftX->GetValue()/Patch->GetMaterial()->GetWidth();
            if (Setting & ApplyShiftY)   SI.Trans[1]=m_SpinCtrlShiftY->GetValue()/Patch->GetMaterial()->GetHeight();
            if (Setting & ApplyRotation)
            {
                SI.RotateUVAxes(m_SpinCtrlRotation->GetValue()-SI.Rotate);
                SI.Rotate=m_SpinCtrlRotation->GetValue();
            }

            break;
        }

        case ApplyViewAligned:
        {
            // Apply the current material view aligned to the patch.
            if (ViewWin3D==NULL) break;

            SI.UAxis=ViewWin3D->GetCamera().GetXAxis();
            SI.VAxis=ViewWin3D->GetCamera().GetZAxis();

            SI.TexCoordGenMode=PlaneProj;

            // Set rotation to zero, so the whole rotation value from the dialog is applied below.
            SI.Rotate=0.0f;

            SetSurfaceInfo(Patch, SI, Material, ApplyNormal, ApplySettingT(ApplyAll & ~ApplyMaterial), ViewWin3D);
            break;
        }

        case ApplyEdgeAligned:
        {
            wxMessageBox("Apply Edge Aligned is not available for bezier patches.");
            break;
        }

        case ApplyProjective:
        {
            // "Raw-copy" the last picked material 1:1 onto the patch, including the u- and v-axes.
            SI.UAxis=m_CurrentUAxis;
            SI.VAxis=m_CurrentVAxis;

            SI.TexCoordGenMode=PlaneProj;

            // Set rotation to zero, so the whole rotation value from the dialog is applied below.
            SI.Rotate=0.0f;

            SetSurfaceInfo(Patch, SI, Material, ApplyNormal, ApplySettingT(ApplyAll & ~ApplyMaterial), ViewWin3D);
            break;
        }
    }
}


void EditSurfacePropsDialogT::UpdateVectorInfo()
{
    MaterialXInfo->SetLabel(wxString::Format("( %f | %f | %f )", m_CurrentUAxis.x, m_CurrentUAxis.y, m_CurrentUAxis.z));
    MaterialYInfo->SetLabel(wxString::Format("( %f | %f | %f )", m_CurrentVAxis.x, m_CurrentVAxis.y, m_CurrentVAxis.z));
}


/**********************/
/*** Event Handlers ***/
/**********************/

void EditSurfacePropsDialogT::OnSpinCtrlValueChanged(wxSpinDoubleEvent& Event)
{
    ApplySettingT Setting=ApplyNone;

    // Choose apply setting depending on the changed spin ctrl.
    switch (Event.GetId())
    {
        case ID_SPINCTRL_SCALE_X:  Setting=ApplyScaleX;   break;
        case ID_SPINCTRL_SCALE_Y:  Setting=ApplyScaleY;   break;
        case ID_SPINCTRL_SHIFT_X:  Setting=ApplyShiftX;   break;
        case ID_SPINCTRL_SHIFT_Y:  Setting=ApplyShiftY;   break;
        case ID_SPINCTRL_ROTATION: Setting=ApplyRotation; break;
    }

    // The value in one of the spin controls changed, and thus the orientation of the material on the selected face(s)/patch(es).

    ArrayT<CommandT*> SurfaceCommands;

    for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
    {
        EditorMaterialI* Material=NULL;
        SurfaceInfoT     SI=m_SelectedFaces[FaceNr].Face->GetSurfaceInfo();

        SetSurfaceInfo(m_SelectedFaces[FaceNr].Face, SI, &Material, ApplyNormal, Setting);
        SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, m_SelectedFaces[FaceNr].Brush, m_SelectedFaces[FaceNr].FaceIndex, SI, Material));
    }

    for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); PatchNr++)
    {
        EditorMaterialI* Material=NULL;
        SurfaceInfoT     SI=m_SelectedPatches[PatchNr]->GetSurfaceInfo();

        SetSurfaceInfo(m_SelectedPatches[PatchNr], SI, &Material, ApplyNormal, Setting);
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

        m_MapDoc->GetHistory().SubmitCommand(Macro);
    }
}


void EditSurfacePropsDialogT::OnButtonAlign(wxCommandEvent& Event)
{
    if (m_MapDoc==0) return;

    if (m_SelectedFaces.Size()>0)
    {
        ArrayT<Vector3fT> CombinedVertices;
        ArrayT<CommandT*> SurfaceCommands;

        // If all the selected faces are to be treated "as one", prepare the array with the union of their vertices.
        if (CheckBoxTreatMultipleAsOne->IsChecked())
            for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
                CombinedVertices.PushBack(m_SelectedFaces[FaceNr].Face->GetVertices());

        for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        {
            const ArrayT<Vector3fT>& Vertices=CheckBoxTreatMultipleAsOne->IsChecked() ? CombinedVertices : m_SelectedFaces[FaceNr].Face->GetVertices();
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

        m_MapDoc->GetHistory().SubmitCommand(new CommandMacroT(SurfaceCommands, "Align Material"));
    }
    else if (m_SelectedPatches.Size()>0)
    {
        if (Event.GetId()!=ID_BUTTON_ALIGN2FITFACE)
        {
            wxMessageBox("With Bezier patches, only the Fit button is available for material alignment.\n");
            return;
        }

        ArrayT<CommandT*> SurfaceCommands;

        for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); ++PatchNr)
        {
            EditorMaterialI* Material=NULL;
            SurfaceInfoT     SI=m_SelectedPatches[PatchNr]->GetSurfaceInfo();

            SI.TexCoordGenMode=MatFit;

            SetSurfaceInfo(m_SelectedPatches[PatchNr], SI, &Material, ApplyNormal, ApplyAll);
            SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(*m_MapDoc, m_SelectedPatches[PatchNr], SI, Material));
        }

        CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Justify Material");

        m_MapDoc->GetHistory().SubmitCommand(Macro);
    }
}


// The OnCheckBoxAlign*() methods are the only way to reset a faces UV-axes (besides Apply Projective mode).
// As such, the checkboxes act both as indicators of the current state, as well as controls to change it.
void EditSurfacePropsDialogT::OnCheckBoxAlignWorld(wxCommandEvent& Event)
{
    if (m_MapDoc==0) return;

    if (m_SelectedFaces.Size()>0)
    {
        ArrayT<CommandT*> SurfaceCommands;

        for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        {
            SurfaceInfoT SI=m_SelectedFaces[FaceNr].Face->GetSurfaceInfo();

            SI.ResetUVAxes(m_SelectedFaces[FaceNr].Face->GetPlane(), false /*not FaceAligned*/);
            SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, m_SelectedFaces[FaceNr].Brush, m_SelectedFaces[FaceNr].FaceIndex, SI, NULL));
        }

        CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Align Material wrt. World Axes");

        m_MapDoc->GetHistory().SubmitCommand(Macro);
    }
}


// The OnCheckBoxAlign*() methods are the only way to reset a faces UV-axes (besides Apply Projective mode).
// As such, the checkboxes act both as indicators of the current state, as well as controls to change it.
void EditSurfacePropsDialogT::OnCheckBoxAlignFace(wxCommandEvent& Event)
{
    if (m_MapDoc==0) return;

    if (m_SelectedFaces.Size()>0)
    {
        ArrayT<CommandT*> SurfaceCommands;

        for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
        {
            SurfaceInfoT SI=m_SelectedFaces[FaceNr].Face->GetSurfaceInfo();

            SI.ResetUVAxes(m_SelectedFaces[FaceNr].Face->GetPlane(), true /*FaceAligned*/);
            SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, m_SelectedFaces[FaceNr].Brush, m_SelectedFaces[FaceNr].FaceIndex, SI, NULL));
        }

        CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Align Material wrt. Face Plane");

        m_MapDoc->GetHistory().SubmitCommand(Macro);
    }
}


void EditSurfacePropsDialogT::OnCheckBoxTreatMultipleAsOne(wxCommandEvent& Event)
{
    // Afaics, no need for any action here.
    // Those who are interested in the selected value of the CheckBox should query it directly.
    // The "Treat Multiple As One" flag is only needed for the Fit, T, B, L, R, C alignment buttons anyway!
}


// This looks at the current selection of the ChoiceCurrentMat,
// and updates the material preview bitmap and "Size: a x b" text accordingly.
// It also rearranges the contents of the ChoiceCurrentMat to reflect MRU behaviour.
void EditSurfacePropsDialogT::OnSelChangeCurrentMat(wxCommandEvent& Event)
{
    int Index=ChoiceCurrentMat->GetSelection();

    wxASSERT(Index!=-1);

    // if (Index==-1)
    // {
    //    BitmapCurrentMat->m_Bitmap=wxNullBitmap;
    //     BitmapCurrentMat->Refresh();
    //     StaticTextCurrentMatSize->SetLabel("(No mat. selected.)");
    // }
    // else
    {
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

        // Apply the new material to the selection.
        if (Event.GetExtraLong()!=-1) // Don't update selected faces/patches if not allowed (ExtraLong=-1).
        {
            ArrayT<CommandT*> SurfaceCommands;

            for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
            {
                EditorMaterialI* Material=NULL;
                SurfaceInfoT     SI=m_SelectedFaces[FaceNr].Face->GetSurfaceInfo();

                SetSurfaceInfo(m_SelectedFaces[FaceNr].Face, SI, &Material, ApplyNormal, ApplyMaterial);
                SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, m_SelectedFaces[FaceNr].Brush, m_SelectedFaces[FaceNr].FaceIndex, SI, Material));
            }

            for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); PatchNr++)
            {
                EditorMaterialI* Material=NULL;
                SurfaceInfoT     SI=m_SelectedPatches[PatchNr]->GetSurfaceInfo();

                SetSurfaceInfo(m_SelectedPatches[PatchNr], SI, &Material, ApplyNormal, ApplyMaterial);
                SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(*m_MapDoc, m_SelectedPatches[PatchNr], SI, Material));
            }

            if (SurfaceCommands.Size()>0)
            {
                CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Apply material to selection");

                m_MapDoc->GetHistory().SubmitCommand(Macro);
            }
        }
    }
}


void EditSurfacePropsDialogT::OnButtonBrowseMats(wxCommandEvent& Event)
{
    int Index=ChoiceCurrentMat->GetSelection();

    MaterialBrowser::DialogT MatBrowser(this,
        MaterialBrowser::MapDocAccessT(*m_MapDoc),
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


void EditSurfacePropsDialogT::OnButtonApplyToAllSelected(wxCommandEvent& Event)
{
    const RightMBClickModeT ApplyMode=(RightMBClickModeT)(unsigned long)ChoiceRightMBMode->GetClientData(ChoiceRightMBMode->GetSelection());

    if (ApplyMode==ApplyProjective && m_CurrentTexGenMode!=PlaneProj)   // Only apply projective, if there is an projection plane.
    {
        wxMessageBox("When the texture-coordinate mode is Fit or Custom,\n"
            "there is no reference plane for projective material application.\n\n"
            "Please use another apply mode for the right mouse button,\n"
            "or pick a regular brush face in order to apply its material projectively.");
        return;
    }

    ArrayT<CommandT*> SurfaceCommands;

    for (unsigned long FaceNr=0; FaceNr<m_SelectedFaces.Size(); FaceNr++)
    {
        EditorMaterialI* Material=NULL;
        SurfaceInfoT     SI=m_SelectedFaces[FaceNr].Face->GetSurfaceInfo();

        SetSurfaceInfo(m_SelectedFaces[FaceNr].Face, SI, &Material, ApplyMode, ApplyAll);
        SurfaceCommands.PushBack(new CommandUpdateSurfaceFaceT(*m_MapDoc, m_SelectedFaces[FaceNr].Brush, m_SelectedFaces[FaceNr].FaceIndex, SI, Material));
    }

    for (unsigned long PatchNr=0; PatchNr<m_SelectedPatches.Size(); PatchNr++)
    {
        EditorMaterialI* Material=NULL;
        SurfaceInfoT     SI=m_SelectedPatches[PatchNr]->GetSurfaceInfo();

        SetSurfaceInfo(m_SelectedPatches[PatchNr], SI, &Material, ApplyMode, ApplyAll);
        SurfaceCommands.PushBack(new CommandUpdateSurfaceBezierPatchT(*m_MapDoc, m_SelectedPatches[PatchNr], SI, Material));
    }

    if (SurfaceCommands.Size()>0)
    {
        CommandMacroT* Macro=new CommandMacroT(SurfaceCommands, "Apply to all selected");

        m_MapDoc->GetHistory().SubmitCommand(Macro);
    }
}


ArrayT<EditorMaterialI*> EditSurfacePropsDialogT::GetMRUMaterials() const
{
    ArrayT<EditorMaterialI*> MRUMaterials;

    for (unsigned long i=0; i<ChoiceCurrentMat->GetCount(); i++)
        MRUMaterials.PushBack((EditorMaterialI*)ChoiceCurrentMat->GetClientData(i));

    return MRUMaterials;
}
