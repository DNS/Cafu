/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_SCENE_PROPGRID_HPP_INCLUDED
#define CAFU_MODELEDITOR_SCENE_PROPGRID_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "wx/wx.h"
#include "wx/propgrid/manager.h"


namespace MatSys { class TextureMapI; }


namespace ModelEditor
{
    class ChildFrameT;


    class ScenePropGridT : public wxPropertyGridManager, public ObserverT
    {
        public:

        ScenePropGridT(ChildFrameT* Parent, const wxSize& Size);
        ~ScenePropGridT();

        void RefreshPropGrid();

        // ObserverT implementation.
        void Notify_AnimStateChanged(SubjectT* Subject);
        void Notify_SubjectDies(SubjectT* dyingSubject);

        wxColour             m_BackgroundColor;
        bool                 m_ShowOrigin;
        bool                 m_ShowGrid;
        float                m_GridSpacing;
        bool                 m_GroundPlane_Show;
        float                m_GroundPlane_PosZ;
        bool                 m_GroundPlane_AutoZ;
        bool                 m_Model_ShowMesh;
        bool                 m_Model_ShowSkeleton;
        bool                 m_Model_ShowTriangleNormals;
        bool                 m_Model_ShowTangentSpace;
        unsigned int         m_Model_DebugMaterial;   ///< 0 for "normal/none", 1 for "plain (white)", 2 for "wire-frame"
        wxColour             m_AmbientLightColor;
        MatSys::TextureMapI* m_AmbientTexture;    ///< A uniform 2x2 texture colored in the ambient light color, used as lightmap for the ground plane.


        private:

        void OnPropertyGridChanged(wxPropertyGridEvent& Event);
        void UpdateAmbientTexture();

        ChildFrameT*  m_Parent;
        bool          m_IsRecursiveSelfNotify;
        wxPGProperty* m_AnimFrameNrProp;
        wxPGProperty* m_AnimSpeedProp;
        wxPGProperty* m_AnimLoopProp;

        DECLARE_EVENT_TABLE()
    };
}

#endif
