/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompGameEntity.hpp"
#include "World.hpp"
#include "ClipSys/ClipModel.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "GameSys/World.hpp"
#include "SceneGraph/BspTreeNode.hpp"


/*
 * Note that as an alternative to using entity IDs as indices into WorldT::m_StaticEntityData[] as has been
 * introduced in the revisions of 2013-07-26, it might be possible to let this class have a reference to the
 * WorldT::m_StaticEntityData[] array, add an `unsigned int` member for the world file index, and to
 * serialize/deserialize this index as part of the component. (`ComponentBaseT::Serialize()` would have to
 * call a virtual `DoSerialize()` method that we had to override here.)
 * This would liberate the entity ID from the requirements of double-purpose use, and would allow us to
 * actually *copy* entities with associated world file information. But is it worth the effort?
 * See the revisions of 2013-07-26 for additional details.
 */

CompGameEntityT::CompGameEntityT(StaticEntityDataT* SED)
    : m_StaticEntityData(SED ? SED : new StaticEntityDataT()),
      m_DeleteSED(SED == NULL),
      m_ClipModel(NULL),
      m_ClipPrevOrigin(),
      m_ClipPrevQuat()
{
}


CompGameEntityT::CompGameEntityT(const CompGameEntityT& Comp)
    : m_StaticEntityData(new StaticEntityDataT()),
      m_DeleteSED(true),
      m_ClipModel(NULL),
      m_ClipPrevOrigin(),
      m_ClipPrevQuat()
{
    // The problem with copying CompGameEntityT instances is that the
    // m_StaticEntityData cannot be copied -- see the /*...*/ comment above.
    // Therefore, make sure that only such instances are copied that use
    // "dummy" StaticEntityDataT's anyway.
    assert(Comp.m_StaticEntityData->m_BspTree == NULL || Comp.m_StaticEntityData->m_BspTree->Nodes.Size() == 0);
    assert(Comp.m_StaticEntityData->m_CollModel == NULL);
}


CompGameEntityT::~CompGameEntityT()
{
    delete m_ClipModel;
    m_ClipModel = NULL;

    if (m_DeleteSED)
        delete m_StaticEntityData;
}


CompGameEntityT* CompGameEntityT::Clone() const
{
    return new CompGameEntityT(*this);
}


void CompGameEntityT::UpdateDependencies(cf::GameSys::EntityT* Entity)
{
    if (GetEntity() != Entity)
    {
        delete m_ClipModel;
        m_ClipModel = NULL;
    }

    ComponentBaseT::UpdateDependencies(Entity);

    UpdateClipModel();
}


BoundingBox3fT CompGameEntityT::GetCullingBB() const
{
    if (m_StaticEntityData->m_BspTree)
        return m_StaticEntityData->m_BspTree->GetBoundingBox().AsBoxOfFloat();

    return BoundingBox3fT();
}


void CompGameEntityT::DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    // Deserialization may have updated our origin or orientation,
    // so we have to update the clip model.
    UpdateClipModel();
}


void CompGameEntityT::DoServerFrame(float t)
{
    // TODO:
    // This should actually be in some PostThink() method, so that we can be sure that
    // all behaviour and physics scripts (that possibly alter the origin and orientation)
    // have already been run when we update the clip model.
    // (Same is true for the clip model in the ComponentCollisionModelT class.)
    UpdateClipModel();
}


void CompGameEntityT::UpdateClipModel()
{
    const bool IsNewClipModel = (m_ClipModel == NULL);

    if (!m_ClipModel)
    {
        if (!m_StaticEntityData->m_CollModel) return;
        if (!GetEntity()) return;
        if (GetEntity()->GetID() == 0) return;  // The world's collision model is already handled in the clip world!
        if (!GetEntity()->GetWorld().GetClipWorld()) return;

        m_ClipModel = new cf::ClipSys::ClipModelT(*GetEntity()->GetWorld().GetClipWorld());

        m_ClipModel->SetCollisionModel(m_StaticEntityData->m_CollModel);
        m_ClipModel->SetOwner(this);
    }

    // Has the origin or orientation changed since we last registered clip model? If so, re-register!
    const Vector3fT              o = GetEntity()->GetTransform()->GetOriginWS();
    const cf::math::QuaternionfT q = GetEntity()->GetTransform()->GetQuatWS();

    if (IsNewClipModel || o != m_ClipPrevOrigin || q != m_ClipPrevQuat)
    {
        m_ClipModel->SetOrigin(o.AsVectorOfDouble());
        m_ClipModel->SetOrientation(cf::math::Matrix3x3dT(cf::math::QuaterniondT(q.x, q.y, q.z, q.w)));
        m_ClipModel->Register();

        m_ClipPrevOrigin = o;
        m_ClipPrevQuat   = q;
    }
}
