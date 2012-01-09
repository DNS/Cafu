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

#include "ModifyModel.hpp"

#include "../MapModel.hpp"
#include "../MapDocument.hpp"
#include "../GameConfig.hpp"

#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/CollisionModel_base.hpp"


CommandModifyModelT::CommandModifyModelT(MapDocumentT& MapDoc, MapModelT* Model, const wxString& ModelFileName, const wxString& CollisionModelFileName, const wxString& Label, float Scale, int Sequence, float FrameTimeOff, float FrameTimeScale, bool Animated)
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
      m_NewSequence(Sequence),
      m_OldSequence(Model->m_SeqNumber),
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
    m_Model->m_SeqNumber        =m_NewSequence;
    m_Model->m_FrameNumber      =m_Model->m_FrameNumber-m_Model->m_FrameOffset+m_NewFrameTimeOff;
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
    m_Model->m_SeqNumber        =m_OldSequence;
    m_Model->m_FrameNumber      =m_Model->m_FrameNumber-m_Model->m_FrameOffset+m_OldFrameTimeOff;
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
