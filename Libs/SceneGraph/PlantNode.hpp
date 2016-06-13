/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TREE_NODE_HPP_INCLUDED
#define CAFU_TREE_NODE_HPP_INCLUDED

#include "Node.hpp"
#include "Plants/Tree.hpp"


struct PlantDescriptionT;
class PlantDescrManT;


namespace cf
{
    namespace SceneGraph
    {
        class PlantNodeT : public GenericNodeT
        {
            public:

            /// The constructor.
            PlantNodeT();

            /// Constructor for creating a PlantNodeT from parameters.
            PlantNodeT(const PlantDescriptionT* PlantDescription, unsigned long RandomSeed, const Vector3dT& Position, const Vector3fT& Angles);

            /// Named constructor.
            static PlantNodeT* CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, LightMapManT& LMM, SHLMapManT& SMM, PlantDescrManT& PDM);

            /// The destructor.
            ~PlantNodeT();

            // The NodeT interface.
            void WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const;
            const BoundingBox3T<double>& GetBoundingBox() const;

         // void InitDrawing();
            bool IsOpaque() const { return false; };
            void DrawAmbientContrib(const Vector3dT& ViewerPos) const;
            //void DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const;
            //void DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const;
            void DrawTranslucentContrib(const Vector3dT& ViewerPos) const;


            private:

            PlantNodeT(const PlantNodeT&);       ///< Use of the Copy    Constructor is not allowed.
            void operator = (const PlantNodeT&); ///< Use of the Assignment Operator is not allowed.


            TreeT          m_Tree;
            unsigned long  m_RandomSeed;
            Vector3dT      m_Position;
            Vector3fT      m_Angles;
            std::string    m_DescrFileName;
            BoundingBox3dT m_Bounds;
        };
    }
}


#endif
