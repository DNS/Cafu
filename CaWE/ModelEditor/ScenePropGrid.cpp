/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ScenePropGrid.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "../Camera.hpp"
#include "../EditorMaterial.hpp"
#include "../GameConfig.hpp"
#include "../MapEditor/MapBrush.hpp"
#include "../Options.hpp"
#include "MaterialSystem/MapComposition.hpp"
#include "MaterialSystem/TextureMap.hpp"

#include "wx/confbase.h"
#include "wx/propgrid/advprops.h"


BEGIN_EVENT_TABLE(ModelEditor::ScenePropGridT, wxPropertyGridManager)
    EVT_PG_CHANGED(wxID_ANY, ModelEditor::ScenePropGridT::OnPropertyGridChanged)
END_EVENT_TABLE()


ModelEditor::ScenePropGridT::ScenePropGridT(ChildFrameT* Parent, const wxSize& Size)
    : wxPropertyGridManager(Parent, wxID_ANY, wxDefaultPosition, Size, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER), // | wxPG_DESCRIPTION
      m_BackgroundColor(wxColour(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/BackgroundColor", "rgb(0, 128, 255)"))),
      m_ShowOrigin(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/ShowOrigin", 1l)!=0),
      m_ShowGrid(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/ShowGrid", 0l)!=0),
      m_GridSpacing(Options.Grid.InitialSpacing),
      m_GroundPlane_Show(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_Show", 1l)!=0),
      m_GroundPlane_PosZ(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_PosZ", 0.0f)),
      m_GroundPlane_AutoZ(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_AutoZ", 1l)!=0),
      m_Model_ShowMesh(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/Model_ShowMesh", 1l)!=0),
      m_Model_ShowSkeleton(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/Model_ShowSkeleton", 0l)!=0),
      m_Model_ShowTriangleNormals(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/Model_ShowTriangleNormals", 0l)!=0),
      m_Model_ShowTangentSpace(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/Model_ShowTangentSpace", 0l)!=0),
      m_Model_DebugMaterial(0),
      m_AmbientLightColor(wxColour(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/AmbientLightColor", "rgb(96, 96, 96)"))),
      m_AmbientTexture(NULL),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false),
      m_AnimFrameNrProp(NULL),
      m_AnimSpeedProp(NULL),
      m_AnimLoopProp(NULL)
{
    UpdateAmbientTexture();

    SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);
    AddPage("Scene Setup");

    m_Parent->GetModelDoc()->RegisterObserver(this);
    RefreshPropGrid();
}


ModelEditor::ScenePropGridT::~ScenePropGridT()
{
    if (m_Parent->GetModelDoc())
        m_Parent->GetModelDoc()->UnregisterObserver(this);

    MatSys::TextureMapManager->FreeTextureMap(m_AmbientTexture);
}


void ModelEditor::ScenePropGridT::RefreshPropGrid()
{
    // if (m_GuiDocument==NULL) return;

    ClearPage(0);

    // "General" category.
    wxPGProperty* GeneralCat=Append(new wxPropertyCategory("General"));

    AppendIn(GeneralCat, new wxColourProperty("Background Color", wxPG_LABEL, m_BackgroundColor));
    AppendIn(GeneralCat, new wxBoolProperty("Show Origin", wxPG_LABEL, m_ShowOrigin));
    AppendIn(GeneralCat, new wxBoolProperty("Show Grid", wxPG_LABEL, m_ShowGrid));
    AppendIn(GeneralCat, new wxFloatProperty("Grid Spacing", wxPG_LABEL, m_GridSpacing));


    // "Camera" category.
    const CameraT& Camera=*m_Parent->GetModelDoc()->GetCameras()[0];
    wxPGProperty* CameraCat=Append(new wxPropertyCategory("Camera"));

    wxPGProperty* CameraPos=AppendIn(CameraCat, new wxStringProperty("Pos", "Camera.Pos", "<composed>"));
    wxPGProperty* CameraPosX=AppendIn(CameraPos, new wxFloatProperty("x", wxPG_LABEL, Camera.Pos.x)); SetPropertyTextColour(CameraPosX, wxColour(200, 0, 0)); // With wx2.9, change this into: CameraPosX->SetTextColour(wxColour(255, 0, 0));
    wxPGProperty* CameraPosY=AppendIn(CameraPos, new wxFloatProperty("y", wxPG_LABEL, Camera.Pos.y)); SetPropertyTextColour(CameraPosY, wxColour(0, 200, 0));
    wxPGProperty* CameraPosZ=AppendIn(CameraPos, new wxFloatProperty("z", wxPG_LABEL, Camera.Pos.z)); SetPropertyTextColour(CameraPosZ, wxColour(0, 0, 200));
    Collapse(CameraPos);

    wxPGProperty* CameraAngles=AppendIn(CameraCat, new wxStringProperty("Angles", "Camera.Angles", "<composed>"));
    AppendIn(CameraAngles, new wxFloatProperty("Pitch", wxPG_LABEL, Camera.Angles.pitch()));
 // AppendIn(CameraAngles, new wxFloatProperty("Roll", wxPG_LABEL, Camera.Angles.roll()));
    AppendIn(CameraAngles, new wxFloatProperty("Yaw", wxPG_LABEL, Camera.Angles.yaw()));
    Collapse(CameraAngles);

    wxPGProperty* CameraAdvanced=AppendIn(CameraCat, new wxStringProperty("Advanced", "Camera.Advanced", "<composed>"));
    AppendIn(CameraAdvanced, new wxFloatProperty("vertical FOV", wxPG_LABEL, Camera.VerticalFOV));
    AppendIn(CameraAdvanced, new wxFloatProperty("near plane dist", wxPG_LABEL, Camera.NearPlaneDist));
    AppendIn(CameraAdvanced, new wxFloatProperty("far plane dist", wxPG_LABEL, Camera.FarPlaneDist));
    Collapse(CameraAdvanced);


    // "Scene Elements" category.
    const MapBrushT* Ground=m_Parent->GetModelDoc()->GetGround();
    wxPGProperty* SceneElemsCat=Append(new wxPropertyCategory("Scene Elements"));

    wxPGProperty* GroundPlane=AppendIn(SceneElemsCat, new wxStringProperty("Ground Plane", wxPG_LABEL, "<composed>"));
    AppendIn(GroundPlane, new wxBoolProperty("Show", wxPG_LABEL, m_GroundPlane_Show));
    AppendIn(GroundPlane, new wxFloatProperty("Height (z-Pos)", wxPG_LABEL, m_GroundPlane_PosZ));
    AppendIn(GroundPlane, new wxBoolProperty("Auto Height", wxPG_LABEL, m_GroundPlane_AutoZ));
    AppendIn(GroundPlane, new wxStringProperty("Material", wxPG_LABEL, Ground->GetFaces()[0].GetMaterial()->GetName()));   // TODO: MaterialProperty

    wxPGProperty* ModelProps=AppendIn(SceneElemsCat, new wxStringProperty("Model", wxPG_LABEL, "<composed>"));
    AppendIn(ModelProps, new wxBoolProperty("Show Mesh", wxPG_LABEL, m_Model_ShowMesh));
    AppendIn(ModelProps, new wxBoolProperty("Show Skeleton", wxPG_LABEL, m_Model_ShowSkeleton));
    AppendIn(ModelProps, new wxBoolProperty("Show triangle normals", wxPG_LABEL, m_Model_ShowTriangleNormals));
    AppendIn(ModelProps, new wxBoolProperty("Show tangent-space", wxPG_LABEL, m_Model_ShowTangentSpace));
    const wxChar* DebugMaterialStrings[] = { wxT("normal/none"), wxT("plain (white)"), wxT("wire-frame"), NULL };
    const long    DebugMaterialIndices[] = { 0, 1, 2 };
    AppendIn(ModelProps, new wxEnumProperty("Debug material", wxPG_LABEL, DebugMaterialStrings, DebugMaterialIndices, m_Model_DebugMaterial));


    // "Anim Control" category.
    const ModelDocumentT::AnimStateT& Anim=m_Parent->GetModelDoc()->GetAnimState();

    wxPGProperty* AnimControlCat=Append(new wxPropertyCategory("Animation Control"));
    m_AnimFrameNrProp=AppendIn(AnimControlCat, new wxFloatProperty("Frame No.", wxPG_LABEL, Anim.LastStdAE->GetFrameNr()));
    m_AnimSpeedProp  =AppendIn(AnimControlCat, new wxFloatProperty("Speed", wxPG_LABEL, Anim.Speed));
    m_AnimLoopProp   =AppendIn(AnimControlCat, new wxBoolProperty("Loop", wxPG_LABEL, Anim.Loop));


    // "Light Sources" category.
    const ArrayT<ModelDocumentT::LightSourceT*>& LightSources=m_Parent->GetModelDoc()->GetLightSources();
    wxPGProperty* LightsCat=Append(new wxPropertyCategory("Light Sources"));

    AppendIn(LightsCat, new wxColourProperty("Ambient Light Color", wxPG_LABEL, m_AmbientLightColor));

    for (unsigned long LightNr=0; LightNr<LightSources.Size(); LightNr++)
    {
        const ModelDocumentT::LightSourceT& LS=*LightSources[LightNr];

        wxString      LightStr=wxString::Format("Light %lu", LightNr+1);
        wxPGProperty* LsProp  =AppendIn(LightsCat, new wxStringProperty(LightStr, wxPG_LABEL, "<composed>"));

        AppendIn(LsProp, new wxBoolProperty("On", wxPG_LABEL, LS.IsOn));
        AppendIn(LsProp, new wxBoolProperty("Cast Shadows", wxPG_LABEL, LS.CastShadows));

        wxPGProperty* LightPos=AppendIn(LsProp, new wxStringProperty("Pos", wxPG_LABEL, "<composed>"));
        wxPGProperty* LightPosX=AppendIn(LightPos, new wxFloatProperty("x", wxPG_LABEL, LS.Pos.x)); SetPropertyTextColour(LightPosX, wxColour(200, 0, 0)); // With wx2.9, change this into: LightPosX->SetTextColour(wxColour(255, 0, 0));
        wxPGProperty* LightPosY=AppendIn(LightPos, new wxFloatProperty("y", wxPG_LABEL, LS.Pos.y)); SetPropertyTextColour(LightPosY, wxColour(0, 200, 0));
        wxPGProperty* LightPosZ=AppendIn(LightPos, new wxFloatProperty("z", wxPG_LABEL, LS.Pos.z)); SetPropertyTextColour(LightPosZ, wxColour(0, 0, 200));
        Collapse(LightPos);

        AppendIn(LsProp, new wxFloatProperty("Radius", wxPG_LABEL, LS.Radius));
        AppendIn(LsProp, new wxColourProperty("Color", wxPG_LABEL, LS.Color));

        if (!LS.IsOn) Collapse(LsProp);
    }


    SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, true); // Use checkboxes instead of choice.

    RefreshGrid();
}


void ModelEditor::ScenePropGridT::OnPropertyGridChanged(wxPropertyGridEvent& Event)
{
    // Changing a property by pressing ENTER doesn't change the selection. In consequence the property refresh below does not result in
    // any change since selected properties are not updated (because the user could be in the process of editing a value).
    // Since the user is definitely finished editing this property we can safely clear the selection.
 // ClearSelection();

    ModelDocumentT::AnimStateT& AnimState=m_Parent->GetModelDoc()->GetAnimState();
    CameraT&            Camera    =*m_Parent->GetModelDoc()->GetCameras()[0];
    MapBrushT*          Ground    =m_Parent->GetModelDoc()->GetGround();
    const wxPGProperty* Prop      =Event.GetProperty();
    const wxString      PropName  =Prop->GetName();
    double              PropValueD=0.0;
    const float         PropValueF=Prop->GetValue().Convert(&PropValueD) ? float(PropValueD) : 0.0f;

         if (PropName=="Background Color")                m_BackgroundColor << Prop->GetValue();
    else if (PropName=="Show Origin")                     m_ShowOrigin=Prop->GetValue().GetBool();
    else if (PropName=="Show Grid")                       m_ShowGrid=Prop->GetValue().GetBool();
    else if (PropName=="Grid Spacing")                    m_GridSpacing=PropValueF;
    else if (PropName=="Camera.Pos.x")                    Camera.Pos.x=PropValueF;
    else if (PropName=="Camera.Pos.y")                    Camera.Pos.y=PropValueF;
    else if (PropName=="Camera.Pos.z")                    Camera.Pos.z=PropValueF;
    else if (PropName=="Camera.Angles.Pitch")             Camera.Angles.pitch()=PropValueF;
 // else if (PropName=="Camera.Angles.Roll")              Camera.Angles.roll()=PropValueF;
    else if (PropName=="Camera.Angles.Yaw")               Camera.Angles.yaw()=PropValueF;
    else if (PropName=="Camera.Advanced.vertical FOV")    Camera.VerticalFOV=PropValueF;
    else if (PropName=="Camera.Advanced.near plane dist") Camera.NearPlaneDist=PropValueF;
    else if (PropName=="Camera.Advanced.far plane dist")  Camera.FarPlaneDist=PropValueF;
    else if (PropName=="Ground Plane.Show")               m_GroundPlane_Show=Prop->GetValue().GetBool();
    else if (PropName=="Ground Plane.Height (z-Pos)")     m_GroundPlane_PosZ=PropValueF;
    else if (PropName=="Ground Plane.Auto Height")        m_GroundPlane_AutoZ=Prop->GetValue().GetBool();
    else if (PropName=="Ground Plane.Material")
    {
        EditorMaterialI* NewMat=m_Parent->GetModelDoc()->GetGameConfig()->GetMatMan().FindMaterial(Prop->GetValueAsString(), false /*CreateDummy*/);

        if (NewMat)
        {
            for (unsigned long FaceNr=0; FaceNr<Ground->GetFaces().Size(); FaceNr++)
                Ground->GetFaces()[FaceNr].SetMaterial(NewMat);
        }
    }
    else if (PropName=="Model.Show Mesh")             m_Model_ShowMesh           =Prop->GetValue().GetBool();
    else if (PropName=="Model.Show Skeleton")         m_Model_ShowSkeleton       =Prop->GetValue().GetBool();
    else if (PropName=="Model.Show triangle normals") m_Model_ShowTriangleNormals=Prop->GetValue().GetBool();
    else if (PropName=="Model.Show tangent-space")    m_Model_ShowTangentSpace   =Prop->GetValue().GetBool();
    else if (PropName=="Model.Debug material")        m_Model_DebugMaterial      =Prop->GetValue().GetInteger();
    else if (PropName=="Frame No.") { AnimState.LastStdAE->SetFrameNr(PropValueF); m_IsRecursiveSelfNotify=true; m_Parent->GetModelDoc()->UpdateAllObservers_AnimStateChanged(); m_IsRecursiveSelfNotify=false; }
    else if (PropName=="Speed")     { AnimState.Speed=PropValueF; m_IsRecursiveSelfNotify=true; m_Parent->GetModelDoc()->UpdateAllObservers_AnimStateChanged(); m_IsRecursiveSelfNotify=false; }
    else if (PropName=="Loop")      { AnimState.Loop =Prop->GetValue().GetBool(); m_IsRecursiveSelfNotify=true; m_Parent->GetModelDoc()->UpdateAllObservers_AnimStateChanged(); m_IsRecursiveSelfNotify=false; }
    else if (PropName=="Ambient Light Color")
    {
        m_AmbientLightColor << Prop->GetValue();
        UpdateAmbientTexture();
    }
    else if (PropName.StartsWith("Light "))
    {
        const ArrayT<ModelDocumentT::LightSourceT*>& LightSources=m_Parent->GetModelDoc()->GetLightSources();

        for (unsigned long LsNr=0; LsNr<LightSources.Size(); LsNr++)
        {
            const wxString                LightStr=wxString::Format("Light %lu", LsNr+1);
            ModelDocumentT::LightSourceT& LS      =*LightSources[LsNr];

                 if (PropName==LightStr+".On")           LS.IsOn=Prop->GetValue().GetBool();
            else if (PropName==LightStr+".Cast Shadows") LS.CastShadows=Prop->GetValue().GetBool();
            else if (PropName==LightStr+".Pos.x")        LS.Pos.x=PropValueF;
            else if (PropName==LightStr+".Pos.y")        LS.Pos.y=PropValueF;
            else if (PropName==LightStr+".Pos.z")        LS.Pos.z=PropValueF;
            else if (PropName==LightStr+".Radius")       LS.Radius=PropValueF;
            else if (PropName==LightStr+".Color")        LS.Color << Prop->GetValue();
        }
    }
    else
    {
        // Don't uncomment this:
        // Changing child properties (e.g. "Pos.x" to "5") also generates events for the composite parent (e.g. "Pos" to "(5, 0, 0)")!
        // wxMessageBox("Unknown property label \""+Name+"\".", "Warning", wxOK | wxICON_ERROR);
    }
}


void ModelEditor::ScenePropGridT::UpdateAmbientTexture()
{
    MatSys::TextureMapManager->FreeTextureMap(m_AmbientTexture);
    m_AmbientTexture=NULL;

    const unsigned char r=m_AmbientLightColor.Red();
    const unsigned char g=m_AmbientLightColor.Green();
    const unsigned char b=m_AmbientLightColor.Blue();

    char Data[]={ r, g, b, r, g, b, 0, 0,
                  r, g, b, r, g, b, 0, 0 };

    m_AmbientTexture=MatSys::TextureMapManager->GetTextureMap2D(Data, 2, 2, 3, true, MapCompositionT(MapCompositionT::Linear, MapCompositionT::Linear));
}


void ModelEditor::ScenePropGridT::Notify_AnimStateChanged(SubjectT* Subject)
{
    if (m_IsRecursiveSelfNotify) return;

    const ModelDocumentT::AnimStateT& AnimState=m_Parent->GetModelDoc()->GetAnimState();

    if (m_AnimFrameNrProp->GetValue().GetDouble()!=AnimState.LastStdAE->GetFrameNr())
        m_AnimFrameNrProp->SetValue(AnimState.LastStdAE->GetFrameNr());

    if (m_AnimSpeedProp->GetValue().GetDouble()!=AnimState.Speed)
        m_AnimSpeedProp->SetValue(AnimState.Speed);

    if (m_AnimLoopProp->GetValue().GetBool()!=AnimState.Loop)
        m_AnimLoopProp->SetValue(AnimState.Loop);
}


void ModelEditor::ScenePropGridT::Notify_SubjectDies(SubjectT* dyingSubject)
{
}
