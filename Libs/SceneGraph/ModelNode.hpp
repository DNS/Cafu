/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODEL_NODE_HPP_INCLUDED
#define CAFU_MODEL_NODE_HPP_INCLUDED

#include "Node.hpp"
#include "Models/AnimExpr.hpp"
#include "Util/Util.hpp"


class CafuModelT;
class ModelManagerT;


namespace cf
{
    namespace SceneGraph
    {
        class ModelNodeT : public GenericNodeT
        {
            public:

            /// Constructor for creating a ModelNodeT from parameters.
            ModelNodeT(const CafuModelT* Model, const std::string& Label, const Vector3fT& Origin, const Vector3fT& Angles, float Scale=1.0f, int SeqNumber=0, float FrameOffset=0.0f, float FrameTimeScale=1.0f, bool Animate=false);

            /// Named constructor.
            static ModelNodeT* CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, ModelManagerT& ModelMan);

            /// The destructor.
            ~ModelNodeT();

            // The NodeT interface.
            void WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const;
            const BoundingBox3T<double>& GetBoundingBox() const;

            //void InitDrawing();
            bool IsOpaque() const { return true; }
            void DrawAmbientContrib(const Vector3dT& ViewerPos) const;
            //void DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const;
            //void DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const;
            //void DrawTranslucentContrib(const Vector3dT& ViewerPos) const;


            private:

            /// The constructor.
            ModelNodeT();

            ModelNodeT(const ModelNodeT&);       ///< Use of the Copy    Constructor is not allowed.
            void operator = (const ModelNodeT&); ///< Use of the Assignment Operator is not allowed.

            const CafuModelT* m_Model;
            std::string       m_Label;
            Vector3fT         m_Origin;
            Vector3fT         m_Angles;
            float             m_Scale;
            mutable IntrusivePtrT<AnimExprStandardT> m_AnimExpr;    // FIXME: See comment below. The anim expression is updated inside the constant DrawAmbientContrib() method...
            float             m_FrameOffset;
            float             m_FrameTimeScale;
            bool              m_Animate;

            // TODO/FIXME Unfortunately a mutable timer is necessary in order to get the frame time (done via a non const method in TimerT) on each
            // DrawAmbientContrib call (which is a const method).
            // Further every model has its own timer object to calculate its animation frame. This "problem" occurs for the first time in the scene graph
            // since all other scene graph objects are rendered statically and don't change over time.
            // In general we should evaluate timer usage throughout the engine and aim for a global timer object that is shared by all classes if they
            // have time dependent functionality (or to pass a FrameTime parameter to the scene graph draw methods).
            mutable TimerT m_Timer;
        };
    }
}

#endif
