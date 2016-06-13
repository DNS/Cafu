/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMPONENT_GAME_ENTITY_HPP_INCLUDED
#define CAFU_COMPONENT_GAME_ENTITY_HPP_INCLUDED

#include "GameSys/CompBase.hpp"
#include "GameSys/Entity.hpp"       // For GetGameEnt() only.


class StaticEntityDataT;


/// This component houses the "engine-specific" parts of its entity.
/// It is intended for use by the implementing applications only (map compilers, engine), that is,
/// as the "App" component of `cf::GameSys::EntityT`s. It is not intended for use in game scripts.
/// As such, it doesn't integrate with the TypeSys, and thus isn't available for scripting and
/// whereever else we need the related meta-data.
class CompGameEntityT : public cf::GameSys::ComponentBaseT
{
    public:

    /// The constructor.
    CompGameEntityT(StaticEntityDataT* SED = NULL);

    /// The copy constructor. It creates a new component as a copy of another component.
    /// @param Comp   The component to copy-construct this component from.
    CompGameEntityT(const CompGameEntityT& Comp);

    /// The destructor.
    ~CompGameEntityT();

    const StaticEntityDataT* GetStaticEntityData() const { return m_StaticEntityData; }
    StaticEntityDataT* GetStaticEntityData() { return m_StaticEntityData; }

    // Base class overrides.
    CompGameEntityT* Clone() const;
    const char* GetName() const { return "GameEntity"; }
    void UpdateDependencies(cf::GameSys::EntityT* Entity);
    BoundingBox3fT GetCullingBB() const;
    const cf::ClipSys::ClipModelT* GetClipModel() override { UpdateClipModel(); return m_ClipModel; }


    private:

    void DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting) override;
    void DoServerFrame(float t) override;
    void UpdateClipModel();

    StaticEntityDataT*       m_StaticEntityData;
    const bool               m_DeleteSED;

    cf::ClipSys::ClipModelT* m_ClipModel;       ///< The clip model for the m_StaticEntityData->m_CollModel (the collision model made of the map primitives), NULL for none.
    Vector3fT                m_ClipPrevOrigin;
    cf::math::QuaternionfT   m_ClipPrevQuat;
};


inline IntrusivePtrT<CompGameEntityT> GetGameEnt(IntrusivePtrT<cf::GameSys::EntityT> Entity)
{
    return dynamic_pointer_cast<CompGameEntityT>(Entity->GetApp());
}

#endif
