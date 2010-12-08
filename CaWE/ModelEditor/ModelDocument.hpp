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

#ifndef _MODELEDITOR_MODEL_DOCUMENT_HPP_
#define _MODELEDITOR_MODEL_DOCUMENT_HPP_

#include "../Camera.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


class CafuModelT;
class GameConfigT;
class MapBrushT;


namespace ModelEditor
{
    class ModelDocumentT
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

        class ModelAnimationT
        {
            public:

            ModelAnimationT()
                : SequNr(-1), FrameNr(0.0f), Speed(1.0f), Loop(true) { }

            int   SequNr;   ///< The animation sequence number, -1 is "base (or bind) pose", no animation data.
            float FrameNr;  ///< The current frame number.
            float Speed;    ///< The speed (relative to clock time) with which the animation is advanced, usually 0 for stop or 1 for playback.
            bool  Loop;     ///< When playing the sequence, loop automatically when its end has been reached?
        };


        /// The constructor.
        /// @throws   ModelT::LoadError if the model could not be loaded or imported.
        ModelDocumentT(GameConfigT* GameConfig, const wxString& ModelFileName="");

        /// The destructor.
        ~ModelDocumentT();

        void AdvanceTime(float Time);
        void SetNextAnimSequ();
        void SetPrevAnimSequ();
        void SetAnimSpeed(float NewSpeed);

        const CafuModelT*            GetModel() const        { return m_Model; }
        const ModelAnimationT&       GetAnim() const         { return m_Anim; }
        const MapBrushT*             GetGround() const       { return m_Ground; }
        const ArrayT<CameraT*>&      GetCameras() const      { return m_Cameras; }
        const ArrayT<LightSourceT*>& GetLightSources() const { return m_LightSources; }
        const GameConfigT*           GetGameConfig() const   { return m_GameConfig; }

        ModelAnimationT& GetAnim()       { return m_Anim; }
        MapBrushT*       GetGround()     { return m_Ground; }
        GameConfigT*     GetGameConfig() { return m_GameConfig; }


        private:

        ModelDocumentT(const ModelDocumentT&);      ///< Use of the Copy    Constructor is not allowed.
        void operator = (const ModelDocumentT&);    ///< Use of the Assignment Operator is not allowed.

        CafuModelT*           m_Model;          ///< The model that is being edited.
        ModelAnimationT       m_Anim;           ///< The state of our model animation.
        MapBrushT*            m_Ground;         ///< The ground brush.
        ArrayT<CameraT*>      m_Cameras;        ///< The cameras in the scene (used by the 3D views for rendering), there is always at least one.
        ArrayT<LightSourceT*> m_LightSources;   ///< The light sources that exist in the scene.
        GameConfigT*          m_GameConfig;     ///< The game configuration that the model is used with.
    };
}

#endif
