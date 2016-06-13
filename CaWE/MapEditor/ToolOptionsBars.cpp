/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolOptionsBars.hpp"
#include "MapBezierPatch.hpp"
#include "MapDocument.hpp"
#include "ToolClip.hpp"
#include "ToolMorph.hpp"

#include "../GameConfig.hpp"

#include "wx/statline.h"
#include "wx/filename.h"


OptionsBar_SelectionToolT::OptionsBar_SelectionToolT(wxWindow* Parent)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER)
{
    SetBackgroundColour(wxColour(153, 182, 221));

    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("When selecting:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    m_AutoGroupEntities = new wxCheckBox(this, ChildFrameT::ID_MENU_MAP_AUTO_GROUP_ENTITIES, wxT("Auto-Group Entities"), wxDefaultPosition, wxDefaultSize, 0 );
    m_AutoGroupEntities->SetToolTip( wxT("If checked, all primitives and children of an entity are selected as a group.") );
    item0->Add( m_AutoGroupEntities, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxStaticLine *item4 = new wxStaticLine(this, -1, wxDefaultPosition, wxSize(-1,20), wxLI_VERTICAL );
    item0->Add( item4, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxStaticText *item5 = new wxStaticText(this, -1, wxT("Current selection:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item5, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxButton *item6 = new wxButton(this, ChildFrameT::ID_MENU_SELECTION_GROUP, wxT("Group"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    item6->SetToolTip( wxT("Group the selected items.") );
    item0->Add( item6, 0, wxALIGN_CENTER|wxLEFT, 5 );

    wxButton *item8 = new wxButton(this, ChildFrameT::ID_MENU_SELECTION_HIDE, wxT("Hide"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    item8->SetToolTip( wxT("Hide the selected items in a new group.") );
    item0->Add( item8, 0, wxALIGN_CENTER|wxLEFT, 5 );

    wxButton *item9 = new wxButton(this, ChildFrameT::ID_MENU_SELECTION_HIDE_OTHER, wxT("Hide Other"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    item9->SetToolTip( wxT("Hide all unselected items in a new group (only those that are not in a group already).") );
    item0->Add( item9, 0, wxALIGN_CENTER|wxLEFT, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


OptionsBar_CameraToolT::OptionsBar_CameraToolT(wxWindow* Parent)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER)
{
    SetBackgroundColour(wxColour(153, 182, 221));

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


static OptionsBar_NewBrushToolT::BrushTypeInfoT BrushTypeInfos[]=
{
    // Name, Number, Min, Max.
    { "Block",        4, 4,   4 },
    { "Wedge",        3, 3,   3 },
    { "Cylinder",     8, 3,  32 },
    { "Pyramid",      4, 3,  32 },
    { "Sphere",       8, 3,  32 },
    { "Arch",         8, 3, 128 }
};


BEGIN_EVENT_TABLE(OptionsBar_NewBrushToolT, wxPanel)
    EVT_CHOICE  (OptionsBar_NewBrushToolT::ID_CHOICE_BRUSH_PRIMITIVES, OptionsBar_NewBrushToolT::OnSelChangeBrushPrimitives)
    EVT_SPINCTRL(OptionsBar_NewBrushToolT::ID_SPINCTRL_NR_OF_FACES,    OptionsBar_NewBrushToolT::OnSpinCtrlNrOfFaces)
END_EVENT_TABLE()


OptionsBar_NewBrushToolT::OptionsBar_NewBrushToolT(wxWindow* Parent)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER),
      m_BrushPrimitiveChoice(NULL),
      m_NrOfFacesSpinControl(NULL)
{
    SetBackgroundColour(wxColour(169, 221, 153));

    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("New brush shape:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER|wxLEFT, 5 );

    m_BrushPrimitiveChoice=new wxChoice(this, ID_CHOICE_BRUSH_PRIMITIVES, wxDefaultPosition, wxSize(100,-1), 0, NULL, 0 );
    {
        // Fill the list with brush primitives. This list does never change.
        for (unsigned long BPNr=0; BPNr<sizeof(BrushTypeInfos)/sizeof(BrushTypeInfos[0]); BPNr++)
            m_BrushPrimitiveChoice->Append(BrushTypeInfos[BPNr].Name);

        m_BrushPrimitiveChoice->SetSelection(0);
    }
    m_BrushPrimitiveChoice->SetToolTip("Select the geometric shape that is used for creating new brushes.");
    item0->Add(m_BrushPrimitiveChoice, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxStaticText *item3 = new wxStaticText(this, -1, wxT("with"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item3, 0, wxALIGN_CENTER, 5 );

    m_NrOfFacesSpinControl=new wxSpinCtrl(this, ID_SPINCTRL_NR_OF_FACES, wxT("0"), wxDefaultPosition, wxSize(50,-1), 0, 0, 100, 0 );
    m_NrOfFacesSpinControl->SetToolTip("When viewed from above, the new brush will get this many side faces.");
    item0->Add(m_NrOfFacesSpinControl, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxStaticText *item5 = new wxStaticText(this, -1, wxT("sides."), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item5, 0, wxALIGN_CENTER|wxRIGHT, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);

    wxCommandEvent CE;
    OnSelChangeBrushPrimitives(CE);   // This sets up the spin control (for the number of faces).
}


void OptionsBar_NewBrushToolT::OnSelChangeBrushPrimitives(wxCommandEvent& Event)
{
    const int SelectionNr=m_BrushPrimitiveChoice->GetSelection();
    if (SelectionNr==-1) return;

    m_NrOfFacesSpinControl->SetRange(BrushTypeInfos[SelectionNr].NrOfFacesMin, BrushTypeInfos[SelectionNr].NrOfFacesMax);
    m_NrOfFacesSpinControl->SetValue(BrushTypeInfos[SelectionNr].NrOfFaces);
    m_NrOfFacesSpinControl->Enable(BrushTypeInfos[SelectionNr].NrOfFacesMin!=BrushTypeInfos[SelectionNr].NrOfFacesMax);
}


void OptionsBar_NewBrushToolT::OnSpinCtrlNrOfFaces(wxSpinEvent& Event)
{
    const int SelectionNr=m_BrushPrimitiveChoice->GetSelection();
    if (SelectionNr==-1) return;

    BrushTypeInfoT& BTI=BrushTypeInfos[SelectionNr];

    BTI.NrOfFaces=m_NrOfFacesSpinControl->GetValue();

    if (BTI.NrOfFaces<BTI.NrOfFacesMin) { BTI.NrOfFaces=BTI.NrOfFacesMin; m_NrOfFacesSpinControl->SetValue(BTI.NrOfFaces); }
    if (BTI.NrOfFaces>BTI.NrOfFacesMax) { BTI.NrOfFaces=BTI.NrOfFacesMax; m_NrOfFacesSpinControl->SetValue(BTI.NrOfFaces); }
}


OptionsBar_NewEntityToolT::OptionsBar_NewEntityToolT(wxWindow* Parent, MapDocumentT& MapDoc)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER)
{
    SetBackgroundColour(wxColour(169, 221, 153));

    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item3_ = new wxStaticText(this, -1, wxT("Current selection:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item3_, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxButton *item4 = new wxButton(this, ChildFrameT::ID_MENU_SELECTION_ASSIGN_TO_ENTITY, wxT("Assign to Entity"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    item4->SetToolTip("Assigns the selected elements to a single (new or existing) entity.");
    item0->Add( item4, 0, wxALIGN_CENTER|wxLEFT, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


BEGIN_EVENT_TABLE(OptionsBar_NewBezierPatchToolT, wxPanel)
    EVT_CHOICE(OptionsBar_NewBezierPatchToolT::ID_PATCHTYPE, OptionsBar_NewBezierPatchToolT::OnPatchTypeChoice)
END_EVENT_TABLE()


OptionsBar_NewBezierPatchToolT::OptionsBar_NewBezierPatchToolT(wxWindow* Parent, MapDocumentT& MapDoc)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER),
      m_ChoicePatchType(NULL),
      m_SpinCtrlSubdivsHorz(NULL),
      m_SpinCtrlSubdivsVert(NULL),
      m_MapDoc(MapDoc),
      m_ChoicePatchResX(NULL),
      m_ChoicePatchResY(NULL)
{
    SetBackgroundColour(wxColour(169, 221, 153));

    wxBoxSizer *item0=new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item1=new wxStaticText(this, -1, wxT("New patch type:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER|wxLEFT, 5 );

    wxString PatchTypes[] =
    {
        "Flat",
        "Cylinder",
        "Open Box",
        "Half Cylinder",
        "Quarter Cylinder",
        "Edge Pipe",
        "Cone",
        "Sphere",
        "Convex Endcap",
        "Concave Endcap"
    };

    m_ChoicePatchType=new wxChoice(this, ID_PATCHTYPE, wxDefaultPosition, wxSize(105,-1), 10, PatchTypes, 0 );
    m_ChoicePatchType->SetToolTip("Newly created patches will be preformed to this type of geometry.");
    m_ChoicePatchType->SetSelection(0);
    item0->Add(m_ChoicePatchType, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxBoxSizer *item2 = new wxBoxSizer( wxVERTICAL );

    m_CheckConvex=new wxCheckBox( this, -1, wxT("with convex endcaps"), wxDefaultPosition, wxDefaultSize, 0 );
    m_CheckConvex->Enable(false);
    item2->Add( m_CheckConvex, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_CheckConcave=new wxCheckBox( this, -1, wxT("with concave endcaps"), wxDefaultPosition, wxDefaultSize, 0 );
    m_CheckConcave->Enable(false);
    item2->Add( m_CheckConcave, 0, wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item2, 0, wxALIGN_CENTER, 5 );

    wxStaticText *item3=new wxStaticText(this, -1, wxT("width:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item3, 0, wxALIGN_CENTER|wxLEFT, 5 );

    wxString PatchResolutions[] =
    {
        "3",
        "5",
        "7",
        "9",
        "11",
        "13",
        "15"
    };

    m_ChoicePatchResX=new wxChoice(this, -1, wxDefaultPosition, wxSize(60,-1), 7, PatchResolutions, 0 );
    m_ChoicePatchResX->SetToolTip("A new patch will be created with this many control points in x-direction.");
    m_ChoicePatchResX->SetSelection(0);
    item0->Add(m_ChoicePatchResX, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxStaticText *item5=new wxStaticText(this, -1, wxT("height:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item5, 0, wxALIGN_CENTER|wxLEFT, 5 );

    m_ChoicePatchResY=new wxChoice(this, -1, wxDefaultPosition, wxSize(60,-1), 7, PatchResolutions, 0 );
    m_ChoicePatchResY->SetToolTip("A new patch will be created with this many control points in y-direction.");
    m_ChoicePatchResY->SetSelection(0);
    item0->Add(m_ChoicePatchResY, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5);

    wxStaticText *item6=new wxStaticText(this, -1, wxT("Subdivs Horz:"), wxDefaultPosition, wxDefaultSize, 0);
    item0->Add(item6, 0, wxALIGN_CENTER|wxALL, 5);

    m_SpinCtrlSubdivsHorz=new wxSpinCtrl(this, ID_SUBDIVSHORZ, wxT("-1"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -1, 100, -1);
    item0->Add(m_SpinCtrlSubdivsHorz, 0, wxALIGN_CENTER|wxALL, 5);

    wxStaticText *item7=new wxStaticText(this, -1, wxT("Subdivs Vert:"), wxDefaultPosition, wxDefaultSize, 0);
    item0->Add(item7, 0, wxALIGN_CENTER|wxALL, 5);

    m_SpinCtrlSubdivsVert=new wxSpinCtrl(this, ID_SUBDIVSVERT, wxT("-1"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -1, 100, -1);
    item0->Add(m_SpinCtrlSubdivsVert, 0, wxALIGN_CENTER|wxALL, 5);

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


unsigned long OptionsBar_NewBezierPatchToolT::GetPatchResX() const
{
    unsigned long PatchResX=3;

    m_ChoicePatchResX->GetStringSelection().ToULong(&PatchResX);
    return PatchResX;
}


unsigned long OptionsBar_NewBezierPatchToolT::GetPatchResY() const
{
    unsigned long PatchResY=3;

    m_ChoicePatchResY->GetStringSelection().ToULong(&PatchResY);
    return PatchResY;
}


void OptionsBar_NewBezierPatchToolT::OnPatchTypeChoice(wxCommandEvent& Event)
{
    // Deactivate and activate toolbar elements, according to choosen bezier patch base form.
    switch(m_ChoicePatchType->GetSelection())
    {
        case 1: // Cylinder.
        case 3: // Half Cylinder.
        case 4: // Quarter Cylinder
        case 6: // Cone.
            m_CheckConvex    ->Enable(true);
            m_CheckConcave   ->Enable(true);
            m_ChoicePatchResX->Enable(false);
            m_ChoicePatchResY->Enable(true);
            break;

        case 2: // Open Box.
            m_CheckConvex    ->Enable(true);
            m_CheckConcave   ->Enable(false);
            m_ChoicePatchResX->Enable(false);
            m_ChoicePatchResY->Enable(true);
            break;

        case 5: // Edge Pipe.
            m_CheckConvex    ->Enable(true);
            m_CheckConcave   ->Enable(true);
            m_ChoicePatchResX->Enable(false);
            m_ChoicePatchResY->Enable(false);
            break;

        case 7: // Sphere.
        case 8: // Convex Endcap.
        case 9: // Concave Endcap.
            m_CheckConvex    ->Enable(false);
            m_CheckConcave   ->Enable(false);
            m_ChoicePatchResX->Enable(false);
            m_ChoicePatchResY->Enable(false);
            break;

        default: // Flat bezier patch.
            m_ChoicePatchResX->Enable(true);
            m_ChoicePatchResY->Enable(true);
            m_CheckConvex    ->Enable(false);
            m_CheckConcave   ->Enable(false);
            break;
    }
}


BEGIN_EVENT_TABLE(OptionsBar_NewTerrainToolT, wxPanel)
    EVT_BUTTON(OptionsBar_NewTerrainToolT::ID_BUTTON_BROWSE, OptionsBar_NewTerrainToolT::OnButtonBrowse)
END_EVENT_TABLE()


OptionsBar_NewTerrainToolT::OptionsBar_NewTerrainToolT(wxWindow* Parent, MapDocumentT& MapDoc)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER),
      m_ComboBoxHeightmapName(NULL),
      m_CheckBoxAddWallsAndCeil(NULL),
      m_CheckBoxAddFloor(NULL),
      m_MapDoc(MapDoc)
{
    SetBackgroundColour(wxColour(169, 221, 153));

    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("New terrain heightmap name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER|wxLEFT, 5 );

    m_ComboBoxHeightmapName=new wxComboBox(this, -1, wxT(""), wxDefaultPosition, wxSize(160,-1), 0, NULL, wxCB_DROPDOWN );
    m_ComboBoxHeightmapName->SetToolTip("Newly created terrains get their shape (height values) from this heightmap (an image file).");
    m_ComboBoxHeightmapName->Append("Terrains/BpRockB_hm1.png");    // Add this as an example name.
    item0->Add(m_ComboBoxHeightmapName, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxButton *item3 = new wxButton(this, ID_BUTTON_BROWSE, wxT("Browse"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    item3->SetToolTip("Browse your file system for heightmap files.");
    item0->Add( item3, 0, wxALIGN_CENTER|wxRIGHT, 5 );

    // No, the height is also inherent in the bounding volume in which terrains are created.
    // Reactivate this code if terrains are one day derived from bezier patches, which have no inherent height indication.
    //
    // wxStaticText *item4 = new wxStaticText(this, -1, wxT("height:"), wxDefaultPosition, wxDefaultSize, 0 );
    // item0->Add( item4, 0, wxALIGN_CENTER|wxLEFT, 5 );
    //
    // wxSpinCtrl *item5 = new wxSpinCtrl(this, -1, wxT("0"), wxDefaultPosition, wxSize(60,-1), 0, 1, 10000, 0 );
    // item5->SetToolTip("This is the maximum height of the new terrain, in world units.");
    // item0->Add( item5, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxBoxSizer *item7 = new wxBoxSizer( wxVERTICAL );

    m_CheckBoxAddWallsAndCeil=new wxCheckBox(this, -1, wxT("Add sky walls and ceiling"), wxDefaultPosition, wxDefaultSize, 0 );
    m_CheckBoxAddWallsAndCeil->SetValue(true);  // Check it per default so that users get some hint about how terrains are supposed to work.
    m_CheckBoxAddWallsAndCeil->SetToolTip( wxT("Add four walls and a ceiling brush around the terrain that have the sky material applied.") );
    item7->Add(m_CheckBoxAddWallsAndCeil, 0, wxALIGN_CENTER_VERTICAL, 5 );

    m_CheckBoxAddFloor=new wxCheckBox(this, -1, wxT("Add caulk floor"), wxDefaultPosition, wxDefaultSize, 0 );
    m_CheckBoxAddFloor->SetValue(true);     // Check it per default so that users get some hint about how terrains are supposed to work.
    m_CheckBoxAddFloor->SetToolTip( wxT("As worlds must be watertightly sealed, even outdoor terrains must on the \"inside\" of a world. Thus, some floor brush for terrains is required, and this checkbox adds one automatically.") );
    item7->Add(m_CheckBoxAddFloor, 0, wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item7, 0, wxALIGN_CENTER|wxLEFT, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


void OptionsBar_NewTerrainToolT::OnButtonBrowse(wxCommandEvent& Event)
{
    wxString HeightmapNameStr=wxFileSelector("Select a heightmap image", m_MapDoc.GetGameConfig()->ModDir+"/Terrains/", "", "", "All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (HeightmapNameStr!="")
    {
        wxFileName HeightmapName(HeightmapNameStr);

        HeightmapName.MakeRelativeTo(m_MapDoc.GetGameConfig()->ModDir);
        m_ComboBoxHeightmapName->SetValue(HeightmapName.GetFullPath());
    }
}


OptionsBar_NewDecalToolT::OptionsBar_NewDecalToolT(wxWindow* Parent)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER)
{
    SetBackgroundColour(wxColour(169, 221, 153));

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


OptionsBar_EditFacePropsToolT::OptionsBar_EditFacePropsToolT(wxWindow* Parent)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER)
{
    SetBackgroundColour(wxColour(221, 203, 153));

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


BEGIN_EVENT_TABLE(OptionsBar_ClipBrushesToolT, wxPanel)
    EVT_RADIOBUTTON(OptionsBar_ClipBrushesToolT::ID_RB_CLIPMODE_KEEP_FRONT, OptionsBar_ClipBrushesToolT::OnSelChangeClipMode)
    EVT_RADIOBUTTON(OptionsBar_ClipBrushesToolT::ID_RB_CLIPMODE_KEEP_BACK,  OptionsBar_ClipBrushesToolT::OnSelChangeClipMode)
    EVT_RADIOBUTTON(OptionsBar_ClipBrushesToolT::ID_RB_CLIPMODE_KEEP_BOTH,  OptionsBar_ClipBrushesToolT::OnSelChangeClipMode)
END_EVENT_TABLE()


OptionsBar_ClipBrushesToolT::OptionsBar_ClipBrushesToolT(wxWindow* Parent, ToolClipT& ToolClipBrushes)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER),
      m_ToolClipBrushes(ToolClipBrushes),
      m_RB_ClipModeKeepFront(NULL),
      m_RB_ClipModeKeepBack(NULL),
      m_RB_ClipModeKeepBoth(NULL)
{
    SetBackgroundColour(wxColour(221, 203, 153));

    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("Keep:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    m_RB_ClipModeKeepFront=new wxRadioButton(this, ID_RB_CLIPMODE_KEEP_FRONT, wxT("Front"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_RB_ClipModeKeepFront->SetToolTip("Keep only the front part of the clipped brush. (The back part is thrown away.)");
    item0->Add(m_RB_ClipModeKeepFront, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    m_RB_ClipModeKeepBack=new wxRadioButton(this, ID_RB_CLIPMODE_KEEP_BACK, wxT("Back"), wxDefaultPosition, wxDefaultSize, 0 );
    m_RB_ClipModeKeepBack->SetToolTip("Keep only the back part of the clipped brush. (The front part is thrown away.)");
    item0->Add(m_RB_ClipModeKeepBack, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    m_RB_ClipModeKeepBoth=new wxRadioButton(this, ID_RB_CLIPMODE_KEEP_BOTH, wxT("Both"), wxDefaultPosition, wxDefaultSize, 0 );
    m_RB_ClipModeKeepBoth->SetToolTip("Keep both the front and back parts of the clipped brush.");
    item0->Add(m_RB_ClipModeKeepBoth, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);

    m_RB_ClipModeKeepBoth->SetValue(true);

    // Don't do anything else here. Who says that the clip tool is active when this ctor runs, after all?!
    // On the other hand, we assume that the tool is already fully constructed when we get here, and it
    // really should be able to deal with a call to NoteClipModeChanged() even if inactive.
}


OptionsBar_ClipBrushesToolT::ClipModeT OptionsBar_ClipBrushesToolT::GetClipMode() const
{
    if (m_RB_ClipModeKeepFront->GetValue()) return KeepFront;
    if (m_RB_ClipModeKeepBack ->GetValue()) return KeepBack;

    return KeepBoth;
}


void OptionsBar_ClipBrushesToolT::CycleClipMode()
{
         if (m_RB_ClipModeKeepFront->GetValue()) m_RB_ClipModeKeepBack ->SetValue(true);
    else if (m_RB_ClipModeKeepBack ->GetValue()) m_RB_ClipModeKeepBoth ->SetValue(true);
    else                                         m_RB_ClipModeKeepFront->SetValue(true);

    wxCommandEvent CE;
    OnSelChangeClipMode(CE);
}


void OptionsBar_ClipBrushesToolT::OnSelChangeClipMode(wxCommandEvent& CE)
{
    m_ToolClipBrushes.NoteClipModeChanged();
}


BEGIN_EVENT_TABLE(OptionsBar_EditVerticesToolT, wxPanel)
    EVT_RADIOBUTTON(OptionsBar_EditVerticesToolT::ID_RB_EDITMODE_VERTICES, OptionsBar_EditVerticesToolT::OnSelChangeEditMode)
    EVT_RADIOBUTTON(OptionsBar_EditVerticesToolT::ID_RB_EDITMODE_EDGES,    OptionsBar_EditVerticesToolT::OnSelChangeEditMode)
    EVT_RADIOBUTTON(OptionsBar_EditVerticesToolT::ID_RB_EDITMODE_BOTH,     OptionsBar_EditVerticesToolT::OnSelChangeEditMode)
    EVT_BUTTON     (OptionsBar_EditVerticesToolT::ID_BUTTON_INSERT_VERTEX, OptionsBar_EditVerticesToolT::OnButtonInsertVertex)
END_EVENT_TABLE()


OptionsBar_EditVerticesToolT::OptionsBar_EditVerticesToolT(wxWindow* Parent, ToolMorphT& ToolEditVertices)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxSUNKEN_BORDER),
      m_ToolEditVertices(ToolEditVertices),
      m_RB_EditModeVertices(NULL),
      m_RB_EditModeEdges(NULL),
      m_RB_EditModeBoth(NULL)
{
    SetBackgroundColour(wxColour(221, 203, 153));

    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("Edit:"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add( item1, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    m_RB_EditModeVertices=new wxRadioButton(this, ID_RB_EDITMODE_VERTICES, wxT("Vertices"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    m_RB_EditModeVertices->SetToolTip("Only show the vertices for editing.");
    item0->Add(m_RB_EditModeVertices, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    m_RB_EditModeEdges=new wxRadioButton(this, ID_RB_EDITMODE_EDGES, wxT("Edges"), wxDefaultPosition, wxDefaultSize, 0 );
    m_RB_EditModeEdges->SetToolTip("Only show the edges for editing.");
    item0->Add(m_RB_EditModeEdges, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    m_RB_EditModeBoth=new wxRadioButton(this, ID_RB_EDITMODE_BOTH, wxT("Both"), wxDefaultPosition, wxDefaultSize, 0 );
    m_RB_EditModeBoth->SetToolTip("Show both the vertices and the edges for editing.");
    item0->Add(m_RB_EditModeBoth, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    item0->Add( 10, 10, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item5 = new wxButton(this, ID_BUTTON_INSERT_VERTEX, wxT("Insert Vertex"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->SetToolTip("Insert another vertex at the center of the brush that is being morphed. This vertex can then be dragged to become a part of the convex hull.");
    item0->Add( item5, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);

    m_RB_EditModeBoth->SetValue(true);

    // Don't do anything else here. Who says that the edit vertices tool is active when this ctor runs, after all?!
    // On the other hand, we assume that the tool is already fully constructed when we get here, and it
    // really should be able to deal with a call to NoteEditModeChanged() even if inactive.
}


OptionsBar_EditVerticesToolT::EditModeT OptionsBar_EditVerticesToolT::GetEditMode() const
{
    if (m_RB_EditModeVertices->GetValue()) return EditVertices;
    if (m_RB_EditModeEdges   ->GetValue()) return EditEdges;

    return EditBoth;
}


bool OptionsBar_EditVerticesToolT::IsEditingVertices() const { return m_RB_EditModeVertices->GetValue() || m_RB_EditModeBoth->GetValue(); }
bool OptionsBar_EditVerticesToolT::IsEditingEdges()    const { return m_RB_EditModeEdges   ->GetValue() || m_RB_EditModeBoth->GetValue(); }


void OptionsBar_EditVerticesToolT::CycleEditMode()
{
         if (m_RB_EditModeVertices->GetValue()) m_RB_EditModeEdges   ->SetValue(true);
    else if (m_RB_EditModeEdges   ->GetValue()) m_RB_EditModeBoth    ->SetValue(true);
    else                                        m_RB_EditModeVertices->SetValue(true);

    wxCommandEvent CE;
    OnSelChangeEditMode(CE);
}


void OptionsBar_EditVerticesToolT::OnSelChangeEditMode(wxCommandEvent& CE)
{
    m_ToolEditVertices.NoteEditModeChanged();
}


void OptionsBar_EditVerticesToolT::OnButtonInsertVertex(wxCommandEvent& Event)
{
    m_ToolEditVertices.InsertVertex();
}
