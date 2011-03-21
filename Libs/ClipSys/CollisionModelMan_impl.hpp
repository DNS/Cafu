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

#ifndef _CF_CLIPSYS_COLLISION_MODEL_MANAGER_IMPL_HPP_
#define _CF_CLIPSYS_COLLISION_MODEL_MANAGER_IMPL_HPP_

#include "CollisionModelMan.hpp"


namespace cf
{
    namespace ClipSys
    {
        /// This class provides an implementation of the CollModelManI interface.
        class CollModelManImplT : public CollModelManI
        {
            public:

            CollModelManImplT();
            ~CollModelManImplT();

            // The CollisionModelManI interface.
            const CollisionModelT* GetCM(const std::string& FileName);
         // const CollisionModelT* GetCM(std::istream& InFile, SceneGraph::aux::PoolT& Pool, const ArrayT<CollisionModelStaticT::TerrainRefT>& Terrains);
            const CollisionModelT* GetCM(unsigned long Width, unsigned long Height, const ArrayT<Vector3dT>& Mesh, MaterialT* Material);
            const CollisionModelT* GetCM(const BoundingBox3T<double>& BB, MaterialT* Material);
            const CollisionModelT* GetCM(const CollisionModelT* CollisionModel);
            const std::string& GetFileName(const CollisionModelT* CollisionModel) const;
            void FreeCM(const CollisionModelT* CollisionModel);
            unsigned long GetUniqueCMCount() const;


            private:

            struct cmInfoT
            {
                const CollisionModelT* Instance;
                std::string            FileName;
             // unsigned long          ContentsHash;
                unsigned long          RefCount;
                bool                   NoDelete;
            };

            ArrayT<cmInfoT> cmInfos;
        };
    }
}

#endif
