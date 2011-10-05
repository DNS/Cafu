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

#ifndef _MODELEDITOR_MODEL_DOCUMENT_HPP_
#define _MODELEDITOR_MODEL_DOCUMENT_HPP_

#include "ObserverPattern.hpp"
#include "ElementTypes.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Vector3.hpp"
#include "Models/AnimPose.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


class CafuModelT;
class CameraT;
class GameConfigT;
class EditorMaterialI;
class MapBrushT;
namespace cf { namespace GuiSys { class GuiImplT; } }


namespace ModelEditor
{
    class ModelDocumentT : public SubjectT
    {
        public:

        class LightSourceT
        {
            public:

            LightSourceT(bool IsOn_, bool CastShadows_, const Vector3fT& Pos_, float Radius_, const wxColour& Color_)
                : IsOn(IsOn_), CastShadows(CastShadows_), Pos(Pos_), Radius(Radius_), Color(Color_) { }

            bool      IsOn;         ///< Whether this light source is currently on / active / being used.
            bool      CastShadows;  ///< Whether this light source casts shadows.
            Vector3fT Pos;          ///< The light sources position in world space.
            float     Radius;       ///< The light sources radius in world space.
            wxColour  Color;        ///< The light sources color (used for both the diffuse and specular component).
        };

        class AnimStateT
        {
            public:

            AnimStateT(const CafuModelT& Model)
                : Pose(Model), Speed(1.0f), Loop(true) { }

            AnimPoseT Pose;     ///< The current pose of the model, as defined by sequence and frame number.
            float     Speed;    ///< The speed (relative to clock time) with which the animation is advanced, usually 0 for stop or 1 for playback.
            bool      Loop;     ///< When playing the sequence, loop automatically when its end has been reached?
        };

        class SubmodelT
        {
            public:

            SubmodelT(const wxString& fn, CafuModelT* sm, const ArrayT<unsigned int>& jm);
            ~SubmodelT();

            const wxString&             GetFilename() const { return m_Filename; }
            const CafuModelT*           GetSubmodel() const { return m_Submodel; }
            AnimPoseT&                  GetPose() { return m_Pose; }
            const ArrayT<unsigned int>& GetJointsMap() const { return m_JointsMap; }


            private:

            wxString             m_Filename;    ///< The filename of the submodel.
            CafuModelT*          m_Submodel;    ///< The submodel that is shown with the main model.
            AnimPoseT            m_Pose;        ///< The pose of the submodel.
            ArrayT<unsigned int> m_JointsMap;   ///< Describes how the joints of the m_Submodel map to the joints of the m_Model super model.

            SubmodelT(const SubmodelT&);        ///< Use of the Copy    Constructor is not allowed.
            void operator = (const SubmodelT&); ///< Use of the Assignment Operator is not allowed.
        };


        /// The constructor.
        /// @throws   ModelT::LoadError if the model could not be loaded or imported.
        ModelDocumentT(GameConfigT* GameConfig, const wxString& FileName);

        /// The destructor.
        ~ModelDocumentT();

        const CafuModelT*               GetModel() const           { return m_Model; }
        const ArrayT<unsigned int>&     GetSelection(ModelElementTypeT Type) const { wxASSERT(Type<6); return m_Selection[Type]; }
        const BoundingBox3fT&           GetSequenceBB() const      { return m_SequenceBB; }
        int                             GetSelSkinNr() const;      ///< Return the index number of the currently selected skin, or -1 when no skin (that is, the default skin) is selected.
        wxString                        GetSelSkinString() const;  ///< Returns a string representation for the currently selected skin.
        const ArrayT<EditorMaterialI*>& GetEditorMaterials() const { return m_EditorMaterials; }
        const AnimStateT&               GetAnimState() const       { return m_AnimState; }
        const ArrayT<SubmodelT*>&       GetSubmodels() const       { return m_Submodels; }
        const cf::GuiSys::GuiImplT*     GetGui() const             { return m_Gui; }
        const MapBrushT*                GetGround() const          { return m_Ground; }
        const ArrayT<CameraT*>&         GetCameras() const         { return m_Cameras; }
        const ArrayT<LightSourceT*>&    GetLightSources() const    { return m_LightSources; }
        const GameConfigT*              GetGameConfig() const      { return m_GameConfig; }

        CafuModelT*      GetModel()      { return m_Model; }
        void             SetSelection(ModelElementTypeT Type, const ArrayT<unsigned int>& NewSel);
        AnimStateT&      GetAnimState()  { return m_AnimState; }
        void             LoadSubmodel(const wxString& FileName);
        void             UnloadSubmodel(unsigned long SubmodelNr);
        MapBrushT*       GetGround()     { return m_Ground; }
        GameConfigT*     GetGameConfig() { return m_GameConfig; }

        ArrayT<unsigned int> GetSelection_NextAnimSequ() const;  ///< Returns the suggested selection set for activating the next animation sequence.
        ArrayT<unsigned int> GetSelection_PrevAnimSequ() const;  ///< Returns the suggested selection set for activating the previous animation sequence.
        void                 AdvanceTime(float Time);
        void                 SetAnimSpeed(float NewSpeed);


        private:

        ModelDocumentT(const ModelDocumentT&);      ///< Use of the Copy    Constructor is not allowed.
        void operator = (const ModelDocumentT&);    ///< Use of the Assignment Operator is not allowed.

        static CafuModelT* LoadModel(const wxString& FileName);

        CafuModelT*              m_Model;           ///< The model that is being edited.
        ArrayT<unsigned int>     m_Selection[6];    ///< The selected joints, meshes, animations, channels, skins and GUI fixtures.
        BoundingBox3fT           m_SequenceBB;      ///< The bounding-box encompassing all frames of the currently selected animation sequence(s).
        ArrayT<EditorMaterialI*> m_EditorMaterials; ///< One editor material for each material in the model (its material manager).
        AnimStateT               m_AnimState;       ///< The current state of the model animation.
        ArrayT<SubmodelT*>       m_Submodels;       ///< The submodels that are shown with the main model.
        cf::GuiSys::GuiImplT*    m_Gui;             ///< The GUI that is rendered where the model has GUI fixtures.
        MapBrushT*               m_Ground;          ///< The ground brush.
        ArrayT<CameraT*>         m_Cameras;         ///< The cameras in the scene (used by the 3D views for rendering), there is always at least one.
        ArrayT<LightSourceT*>    m_LightSources;    ///< The light sources that exist in the scene.
        GameConfigT*             m_GameConfig;      ///< The game configuration that the model is used with.
    };
}

#endif
