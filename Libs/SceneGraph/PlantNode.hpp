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

#ifndef _TREE_NODE_HPP_
#define _TREE_NODE_HPP_

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
