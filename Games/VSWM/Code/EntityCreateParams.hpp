#include "TypeSys.hpp"

#include <map>
#include "Math3D/Vector3.hpp"

namespace cf { namespace ClipSys { class CollisionModelT; } }
namespace cf { namespace GameSys { class GameWorldI; } }
namespace cf { namespace SceneGraph { class GenericNodeT; } }


class EntityCreateParamsT : public cf::TypeSys::CreateParamsT
{
    public:

    EntityCreateParamsT(
        const unsigned long                       ID_,
        const std::map<std::string, std::string>& Properties_,
        const cf::SceneGraph::GenericNodeT*       RootNode_,
        const cf::ClipSys::CollisionModelT*       CollisionModel_,
        const unsigned long                       WorldFileIndex_,
        cf::GameSys::GameWorldI*                  GameWorld_,
        const Vector3dT&                          Origin_);

    const unsigned long                       ID;
    const std::map<std::string, std::string>& Properties;
    const cf::SceneGraph::GenericNodeT*       RootNode;
    const cf::ClipSys::CollisionModelT*       CollisionModel;
    const unsigned long                       WorldFileIndex;
    cf::GameSys::GameWorldI*                  GameWorld;
    const Vector3dT&                          Origin;
};
