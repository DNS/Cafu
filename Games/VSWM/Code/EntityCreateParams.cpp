#include "EntityCreateParams.hpp"

using namespace GAME_NAME;


EntityCreateParamsT::EntityCreateParamsT(
    const unsigned long                       ID_,
    const std::map<std::string, std::string>& Properties_,
    const cf::SceneGraph::GenericNodeT*       RootNode_,
    const cf::ClipSys::CollisionModelT*       CollisionModel_,
    const unsigned long                       WorldFileIndex_,
    cf::GameSys::GameWorldI*                  GameWorld_,
    const Vector3dT&                          Origin_)
    : ID(ID_),
      Properties(Properties_),
      RootNode(RootNode_),
      CollisionModel(CollisionModel_),
      WorldFileIndex(WorldFileIndex_),
      GameWorld(GameWorld_),
      Origin(Origin_)
{
}
