/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Node.hpp"
#include "_aux.hpp"
#include "BezierPatchNode.hpp"
#include "BspTreeNode.hpp"
#include "FaceNode.hpp"
#include "TerrainNode.hpp"
#include "PlantNode.hpp"
#include "ModelNode.hpp"

using namespace cf::SceneGraph;


GenericNodeT* GenericNodeT::CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool,
    LightMapManT& LMM, SHLMapManT& SMM, PlantDescrManT& PDM, const ArrayT<const TerrainT*>& ShTe, ModelManagerT& ModelMan)
{
    std::string ClassName=aux::ReadString(InFile);

    // C++ FAQ 36.8, http://www.parashift.com/c++-faq-lite/serialization.html#faq-36.8 for more details
    // on how to improve this. I.e. the problem is that this is very inflexible - new classes derived from NodeT cannot be
    // added without altering the code here. Rather have them register their class names and pointer to named ctor in a std::map,
    // as suggested in the FAQ.
         if (ClassName=="BP"     ) return BezierPatchNodeT::CreateFromFile_cw(InFile, Pool, LMM, SMM);  // Named ctors.
    else if (ClassName=="BspTree") return BspTreeNodeT::CreateFromFile_cw(InFile, Pool, LMM, SMM, PDM, ShTe, ModelMan);
    else if (ClassName=="Face"   ) return FaceNodeT::CreateFromFile_cw(InFile, Pool, LMM, SMM);
    else if (ClassName=="Terrain") return TerrainNodeT::CreateFromFile_cw(InFile, Pool, LMM, SMM, ShTe);
    else if (ClassName=="Plant"  ) return PlantNodeT::CreateFromFile_cw(InFile, Pool, LMM, SMM, PDM);
    else if (ClassName=="Model"  ) return ModelNodeT::CreateFromFile_cw(InFile, Pool, ModelMan);

    return NULL;
}
