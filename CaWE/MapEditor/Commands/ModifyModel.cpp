/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ModifyModel.hpp"

#include "../MapModel.hpp"
#include "../MapDocument.hpp"

#include "../../GameConfig.hpp"

#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/CollisionModel_base.hpp"


CommandModifyModelT::CommandModifyModelT(MapDocumentT& MapDoc, MapModelT* Model, const wxString& ModelFileName,
    const wxString& CollisionModelFileName, const wxString& Label, float Scale,
    IntrusivePtrT<AnimExprStandardT> AnimExpr, float FrameTimeOff, float FrameTimeScale, bool Animated)
    : m_MapDoc(MapDoc),
      m_Model(Model),
      m_NewModelFileName(ModelFileName),
      m_OldModelFileName(Model->m_ModelFileName),
      m_NewCollModelFileName(CollisionModelFileName),
      m_OldCollModelFileName(Model->m_CollModelFileName),
      m_NewLabel(Label),
      m_OldLabel(Model->m_Label),
      m_NewScale(Scale),
      m_OldScale(Model->m_Scale),
      m_NewAnimExpr(AnimExpr),
      m_OldAnimExpr(Model->m_AnimExpr),
      m_NewFrameTimeOff(FrameTimeOff),
      m_OldFrameTimeOff(Model->m_FrameOffset),
      m_NewFrameTimeScale(FrameTimeScale),
      m_OldFrameTimeScale(Model->m_FrameTimeScale),
      m_NewAnimated(Animated),
      m_OldAnimated(Model->m_Animated)
{
}


bool CommandModifyModelT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Build observer notification parameters.
    ArrayT<MapElementT*>   MapElements;
    ArrayT<BoundingBox3fT> OldBounds;

    MapElements.PushBack(m_Model);
    OldBounds.PushBack(m_Model->GetBB());

    m_Model->m_ModelFileName    =m_NewModelFileName;
    m_Model->m_Model            =m_MapDoc.GetGameConfig()->GetModel(m_NewModelFileName);
    m_Model->m_CollModelFileName=m_NewCollModelFileName;
    m_Model->m_Label            =m_NewLabel;
    m_Model->m_Scale            =m_NewScale;
    m_Model->m_AnimExpr         =m_NewAnimExpr;
    m_Model->m_FrameOffset      =m_NewFrameTimeOff,
    m_Model->m_FrameTimeScale   =m_NewFrameTimeScale,
    m_Model->m_Animated         =m_NewAnimated;

    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED, OldBounds);

    m_Done=true;
    return true;
}


void CommandModifyModelT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Build observer notification parameters.
    ArrayT<MapElementT*>   MapElements;
    ArrayT<BoundingBox3fT> OldBounds;

    MapElements.PushBack(m_Model);
    OldBounds.PushBack(m_Model->GetBB());

    m_Model->m_ModelFileName    =m_OldModelFileName;
    m_Model->m_Model            =m_MapDoc.GetGameConfig()->GetModel(m_OldModelFileName);
    m_Model->m_CollModelFileName=m_OldCollModelFileName;
    m_Model->m_Label            =m_OldLabel;
    m_Model->m_Scale            =m_OldScale;
    m_Model->m_AnimExpr         =m_OldAnimExpr;
    m_Model->m_FrameOffset      =m_OldFrameTimeOff,
    m_Model->m_FrameTimeScale   =m_OldFrameTimeScale,
    m_Model->m_Animated         =m_OldAnimated;

    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED, OldBounds);

    m_Done=false;
}


wxString CommandModifyModelT::GetName() const
{
    return "Modify model";
}
