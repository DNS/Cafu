/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_COLLISION_MODEL_MANAGER_IMPL_HPP_INCLUDED
#define CAFU_CLIPSYS_COLLISION_MODEL_MANAGER_IMPL_HPP_INCLUDED

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
            const CollisionModelT* GetCM(const std::string& FileName) override;
         // const CollisionModelT* GetCM(std::istream& InFile, SceneGraph::aux::PoolT& Pool, const ArrayT<CollisionModelStaticT::TerrainRefT>& Terrains) override;
            const CollisionModelT* GetCM(unsigned long Width, unsigned long Height, const ArrayT<Vector3dT>& Mesh, MaterialT* Material, const double MIN_NODE_SIZE) override;
            const CollisionModelT* GetCM(const BoundingBox3T<double>& BB, MaterialT* Material) override;
            const CollisionModelT* GetCM(const CollisionModelT* CollisionModel) override;
            const std::string& GetFileName(const CollisionModelT* CollisionModel) const override;
            void FreeCM(const CollisionModelT* CollisionModel) override;
            unsigned long GetUniqueCMCount() const override;


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
