/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompMover.hpp"
#include "AllComponents.hpp"
#include "CompScript.hpp"
#include "Entity.hpp"
#include "World.hpp"
#include "ClipSys/ClipModel.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/Material.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::math;
using namespace cf::GameSys;


/******************************************/
/*** ComponentMoverT::VarDestActivatedT ***/
/******************************************/

ComponentMoverT::VarDestActivatedT::VarDestActivatedT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentMoverT::VarDestActivatedT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("move home");     Values.PushBack(DESTACT_MOVE_HOME);
    Strings.PushBack("reset timeout"); Values.PushBack(DESTACT_RESET_TIMEOUT);
    Strings.PushBack("ignore");        Values.PushBack(DESTACT_IGNORE);
}


/******************************************/
/*** ComponentMoverT::VarOtherEntitiesT ***/
/******************************************/

ComponentMoverT::VarOtherEntitiesT::VarOtherEntitiesT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentMoverT::VarOtherEntitiesT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("ignore");         Values.PushBack(OTHERENTS_IGNORE);
    Strings.PushBack("cannot push");    Values.PushBack(OTHERENTS_CANNOT_PUSH);
    Strings.PushBack("can push");       Values.PushBack(OTHERENTS_CAN_PUSH);
    Strings.PushBack("can force-push"); Values.PushBack(OTHERENTS_CAN_FORCE_PUSH);
}


/*************************************/
/*** ComponentMoverT::VarTrajFuncT ***/
/*************************************/

ComponentMoverT::VarTrajFuncT::VarTrajFuncT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentMoverT::VarTrajFuncT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("linear"); Values.PushBack(TRAJFUNC_LINEAR);
    Strings.PushBack("sine");   Values.PushBack(TRAJFUNC_SINE);
}


/***********************/
/*** ComponentMoverT ***/
/***********************/

const char* ComponentMoverT::DocClass =
    "This component controls the movement of one or more entities and implements the related effects.\n"
    "\n"
    "The component can handle a single entity, e.g. a moving platform or a lift, or several entities\n"
    "that act together as a team, e.g. the wings of a door.\n"
    "\n"
    "This component works in concert with a Script component, which must be present in the same entity.\n"
    "The Mover component queries the Script component for the desired spatial transformation of each\n"
    "team member, and notifies it whenever the movement is blocked by another entity (e.g. a player).\n"
    "It also implements the appropriate effects on other entities, e.g. their being pushed by moving\n"
    "parts, or their riding on top of them.";


const cf::TypeSys::VarsDocT ComponentMoverT::DocVars[] =
{
    { "moveDuration",  "The time in seconds that it takes to move each part from one endpoint to the other." },
    { "destActivated", "Describes the mover's behavior when it is activated at the \"dest\" position." },
    { "destTimeout",   "The timeout in seconds after which the parts move back to their \"home\" position. A negative value to disables the timeout." },
    { "otherEntities", "Describes the mover's behavior regarding other entities." },
    { "trajFunc",      "Describes the base function that is used to compute the mover's trajectory." },
    { "trajExp",       "The exponent that is applied to `trajFunc`." },
    { NULL, NULL }
};


ComponentMoverT::ComponentMoverT()
    : ComponentBaseT(),
      m_MoveDuration("moveDuration", 3.0f),
      m_DestActivated("destActivated", VarDestActivatedT::DESTACT_MOVE_HOME),
      m_DestTimeout("destTimeout", -1.0f),
      m_OtherEntities("otherEntities", VarOtherEntitiesT::OTHERENTS_CAN_PUSH),
      m_TrajFunc("trajFunc", VarTrajFuncT::TRAJFUNC_LINEAR),
      m_TrajExp("trajExp", 1.0f)
{
    GetMemberVars().Add(&m_MoveDuration);
    GetMemberVars().Add(&m_DestActivated);
    GetMemberVars().Add(&m_DestTimeout);
    GetMemberVars().Add(&m_OtherEntities);
    GetMemberVars().Add(&m_TrajFunc);
    GetMemberVars().Add(&m_TrajExp);
}


ComponentMoverT::ComponentMoverT(const ComponentMoverT& Comp)
    : ComponentBaseT(Comp),
      m_MoveDuration(Comp.m_MoveDuration),
      m_DestActivated(Comp.m_DestActivated),
      m_DestTimeout(Comp.m_DestTimeout),
      m_OtherEntities(Comp.m_OtherEntities),
      m_TrajFunc(Comp.m_TrajFunc),
      m_TrajExp(Comp.m_TrajExp)
{
    GetMemberVars().Add(&m_MoveDuration);
    GetMemberVars().Add(&m_DestActivated);
    GetMemberVars().Add(&m_DestTimeout);
    GetMemberVars().Add(&m_OtherEntities);
    GetMemberVars().Add(&m_TrajFunc);
    GetMemberVars().Add(&m_TrajExp);
}


ComponentMoverT* ComponentMoverT::Clone() const
{
    return new ComponentMoverT(*this);
}


namespace
{
    enum EntityPosCatT
    {
        POSCAT_CLEAR,
        POSCAT_IN_SOLID,
        POSCAT_STANDING_ON
    };


    void GetClipModels(IntrusivePtrT<EntityT> Ent, ArrayT<const cf::ClipSys::ClipModelT*>& ClipModels)
    {
        ClipModels.Overwrite();

        // We know that Basics and Transform have no collision models, but App may have.
        if (Ent->GetApp() != NULL && Ent->GetApp()->GetClipModel() != NULL)
            ClipModels.PushBack(Ent->GetApp()->GetClipModel());

        for (unsigned int CompNr = 0; CompNr < Ent->GetComponents().Size(); CompNr++)
            if (Ent->GetComponents()[CompNr]->GetClipModel())
                ClipModels.PushBack(Ent->GetComponents()[CompNr]->GetClipModel());
    }


    ArrayT< IntrusivePtrT<EntityT> > GetEntitiesInBB(const WorldT& World, const BoundingBox3dT& BB)
    {
        ArrayT<cf::ClipSys::ClipModelT*> ClipModels;
        ArrayT< IntrusivePtrT<EntityT> > Entities;

        World.GetClipWorld()->GetClipModelsFromBB(ClipModels, MaterialT::Clip_AllBlocking, BB);

        for (unsigned int i = 0; i < ClipModels.Size(); i++)
            Entities.PushBack(ClipModels[i]->GetOwner()->GetEntity());

        // Remove possible duplicates or "overlaps".
        for (unsigned int i = 0; i < Entities.Size(); i++)
            for (unsigned int j = i + 1; j < Entities.Size(); j++)
            {
                if (Entities[i]->Has(Entities[j]))
                {
                    Entities.RemoveAt(j);
                    j--;
                    continue;
                }

                if (Entities[j]->Has(Entities[i]))
                {
                    Entities.RemoveAt(i);
                    i--;
                    break;
                }
            }

        return Entities;
    }


    EntityPosCatT GetEntityPosCat(IntrusivePtrT<EntityT> Ent, const ArrayT<const cf::ClipSys::ClipModelT*>& PartClipModels)
    {
        // Declared static in order to minimize memory allocs.
        static ArrayT<const cf::ClipSys::ClipModelT*> NearbyClipModels;

        // The clip models must be fetched each time anew in oder to have their updated Transform values being accounted for!
        GetClipModels(Ent, NearbyClipModels);     // TODO: recurse / account for the children as well!!

        EntityPosCatT PosCat = POSCAT_CLEAR;

        for (unsigned int cmNr = 0; cmNr < NearbyClipModels.Size(); cmNr++)
        {
            const cf::ClipSys::ClipModelT* NearbyCM = NearbyClipModels[cmNr];
            cf::ClipSys::TraceResultT      Trace(1.0);
            cf::ClipSys::ClipModelT*       StandingOn = NULL;

            // Note: Tracing NearbyCM's "absolute bounding-box" here can be problematic.
            // It would be better to trace the true clip model or at least its convex hull.
            Ent->GetWorld().GetClipWorld()->TraceConvexSolid(
                cf::ClipSys::TraceBoxT(NearbyCM->GetAbsoluteBB()), Vector3dT(), Vector3dT(0.0, 0.0, -0.1), MaterialT::Clip_AllBlocking, NearbyCM, Trace, &StandingOn);

            if (Trace.StartSolid)
            {
                return POSCAT_IN_SOLID;
            }

            // Are we standing on one of the PartClipModels?
            // Take steep slopes into account, similar to ComponentPlayerPhysicsT::CategorizePosition().
            if (StandingOn && Trace.Fraction < 1.0 && Trace.ImpactNormal.z >= 0.7 && PartClipModels.Find(StandingOn) != -1)
            {
                PosCat = POSCAT_STANDING_ON;
            }
        }

        return PosCat;
    }


    // This class helps with storing the original transforms of the entities that are
    // affected by a move, and with restoring them if the move fails (is blocked).
    // Note that for restoring the original transforms after a blocked move,
    // storing the originals in parent-space is enough (world-space is not needed).
    class OriginalTransformsT
    {
        public:

        OriginalTransformsT()
            : m_IsMoveSuccessful(false)
        {
        }

        ~OriginalTransformsT()
        {
            // If the move was not successful, restore each affected entity's transform.
            if (!m_IsMoveSuccessful)
            {
                for (std::map<EntityT*, Vector3fT>::const_iterator It = m_OrigsPS.begin(); It != m_OrigsPS.end(); It++)
                    It->first->GetTransform()->SetOriginPS(It->second);

                for (std::map<EntityT*, QuaternionfT>::const_iterator It = m_QuatsPS.begin(); It != m_QuatsPS.end(); It++)
                    It->first->GetTransform()->SetQuatPS(It->second);

            }
        }

        void StoreOrigin(IntrusivePtrT<EntityT> Ent)
        {
            if (m_OrigsPS.find(Ent.get()) == m_OrigsPS.end())
                m_OrigsPS[Ent.get()] = Ent->GetTransform()->GetOriginPS();
        }

        void StoreQuat(IntrusivePtrT<EntityT> Ent)
        {
            if (m_QuatsPS.find(Ent.get()) == m_QuatsPS.end())
                m_QuatsPS[Ent.get()] = Ent->GetTransform()->GetQuatPS();
        }

#if 0
        void Restore(IntrusivePtrT<EntityT> Ent)
        {
            // Restore Ent's origin.
            {
                std::map<EntityT*, Vector3fT>::const_iterator It = m_OrigsPS.find(Ent.get());

                if (It != m_OrigsPS.end())
                    It->first->GetTransform()->SetOriginPS(It->second);
            }

            // Restore Ent's orientation.
            {
                std::map<EntityT*, QuaternionfT>::const_iterator It = m_QuatsPS.find(Ent.get());

                if (It != m_QuatsPS.end())
                    It->first->GetTransform()->SetQuatPS(It->second);
            }
        }
#endif

        void SetMoveSuccessful()
        {
            m_IsMoveSuccessful = true;
        }


        private:

        std::map<EntityT*, Vector3fT>    m_OrigsPS;
        std::map<EntityT*, QuaternionfT> m_QuatsPS;
        bool                             m_IsMoveSuccessful;
    };
}


// TODO: Support rotations and update entity "heading"
// TODO: "Sweep" vs. "discrete steps"
bool ComponentMoverT::HandleMove(float t) const
{
    EntityT* MoverEnt = GetEntity();

    if (!MoverEnt) return false;

    IntrusivePtrT<ComponentScriptT> Script =
        dynamic_pointer_cast<ComponentScriptT>(MoverEnt->GetComponent("Script"));

    if (Script == NULL) return false;

    OriginalTransformsT OriginalTransforms;

    // Declared here in order to minimize memory allocs.
    ArrayT<const cf::ClipSys::ClipModelT*> PartClipModels;

    for (unsigned int PartNr = 1; PartNr < 128; PartNr++)   // PartNr is used as Lua array index, so start at 1.
    {
        int       PartID = 0;
        Vector3fT Offset;

        Script->CallLuaMethod("GetMove", 0, "if>ifff", PartNr, t, &PartID, &Offset.x, &Offset.y, &Offset.z);

        // PartID is normally 0 because GetMove() returned `false` or `nil`,
        // but as a side effect, this also excludes the world (root) entity from being moved.
        if (PartID <= 0) break;

        IntrusivePtrT<EntityT> Part = MoverEnt->FindID(PartID);

        if (Part == NULL) Part = MoverEnt->GetRoot()->FindID(PartID);
        if (Part == NULL) break;

        if (Part->GetBasics()->IsStatic())
        {
            // Parts should not be static, because CaBSP moves the brushes of static entities
            // into the world. That is, unless Part comes with explicit Model and CollisionModel
            // components, moving Parts that are static likely won't work as expected.
            Console->Warning(cf::va("Mover \"%s\" attempts to move part \"%s\", which is static (as per its Basics component).\n",
                MoverEnt->GetBasics()->GetEntityName().c_str(),
                Part->GetBasics()->GetEntityName().c_str()));
        }

        // Special case: If we are supposed to ignore other entities, only Part needs to be moved.
        // The move is unconditional and cannot fail (thus, even OriginalTransforms can be ignored).
        if (m_OtherEntities.Get() == VarOtherEntitiesT::OTHERENTS_IGNORE)
        {
            if (Offset != Vector3fT(0, 0, 0))
                Part->GetTransform()->SetOriginWS(Part->GetTransform()->GetOriginWS() + Offset);

            // if (Angles != Angles3fT(0, 0, 0))
            //     ...;

            continue;
        }


        // The code below works in three steps:
        //
        //   1. At the beginning, everything is at its initial (original) position.
        //   2. The `Part` entity is moved.
        //   3. Any nearby entities are examined and possibly moved.
        //
        // Note that only after step 2 it is properly possible to determine all entities that
        // are possibly affected by Part's move, because only then the bounding-box that covers
        // the entire move is known. (We could try to anticipate Part's move, i.e. transform
        // its bounding-box before Part's transform itself, and use that bounding-box to find
        // the entities. But alas, while not complicated, this is an extra step, and Part must
        // subsequently be transformed anyway.)
        //
        // It would still be helpful to determine all entities that are standing on Part
        // *before* step 2, as such entities ride on Part and therefore must be moved along
        // with it later, and moving Part may pull the rug out from under an entity that was
        // standing on it and thus may be left behind. While this is possible, it is remarkably
        // difficult to properly "merge" the entities (and their PosCats) of "before step 2"
        // with those of "after step 2".
        //
        // Fortunately, it turns out that examining the situation "before step 2" is not
        // necessary, because we can draw proper conclusions from the situation "after step 2"
        // and "after step 3", which must be examined anyway. This is the idea on which the
        // following code is based.


        // Begin with the bounding-box of Part before the move (in world-space).
        // When done below, the bounding-box will eventually cover the entire transform.
        BoundingBox3dT PartMoveBB;

        GetClipModels(Part, PartClipModels);     // TODO: recurse / account for the children as well!!

        for (unsigned int i = 0; i < PartClipModels.Size(); i++)
            PartMoveBB += PartClipModels[i]->GetAbsoluteBB();


        // Transform the Part entity.
        if (Offset != Vector3fT(0, 0, 0))
        {
            IntrusivePtrT<ComponentTransformT> PartTrafo = Part->GetTransform();

            OriginalTransforms.StoreOrigin(Part);
            PartTrafo->SetOriginWS(PartTrafo->GetOriginWS() + Offset);
        }

        // if (Angles != Angles3fT(0, 0, 0))
        // {
        //     ...;
        // }


        // Now grow PartMoveBB to cover the entire transform.
        // We must fetch the clip models again in oder to have their updated Transform values being accounted for!
        GetClipModels(Part, PartClipModels);     // TODO: recurse / account for the children as well!!

        for (unsigned int i = 0; i < PartClipModels.Size(); i++)
            PartMoveBB += PartClipModels[i]->GetAbsoluteBB();

        if (!PartMoveBB.IsInited()) continue;


        // Determine all nearby (possibly pushed) entities (including those riding on top, but not including Part or any of its children).
        const ArrayT< IntrusivePtrT<EntityT> > NearbyEntities = GetEntitiesInBB(MoverEnt->GetWorld(), PartMoveBB);

        for (unsigned int EntNr = 0; EntNr < NearbyEntities.Size(); EntNr++)
        {
            IntrusivePtrT<EntityT> Ent = NearbyEntities[EntNr];

            // Part never interacts with the world entity.
            assert(Ent->GetID() != 0);
            assert(Ent->GetRoot() != Ent);

            // Part never interacts with itself or any of its children.
            if (Part->Has(Ent)) continue;

            // Part never interacts with any of its parents (we might handle the
            // pushing of a parent as a special case, but for now let's just ignore it).
            if (Ent->Has(Part)) continue;

            // Part never interacts with MoverEnt or any of its children
            // (which are typically the siblings and team members of Part).
            if (MoverEnt->Has(Ent)) continue;

         // if (Ent is not something we want to push) continue;

            // Determine the PosCat after Part but before Ent has been moved.
            const EntityPosCatT OldPosCat = GetEntityPosCat(Ent, PartClipModels);

            // Note that we cannot write
            //     if (OldPosCat == POSCAT_CLEAR) continue;
            // here, because moving Part may have pulled the rug out from under Ent, that is,
            // Ent may have been standing on Part before Part was moved, and became clear only
            // due to the move. Therefore, we move Ent even if it is found to be clear. If this
            // move "restores" the standing on Part, the move was proper, otherwise Ent's move
            // is undone.

            if (OldPosCat == POSCAT_IN_SOLID && m_OtherEntities.Get() == VarOtherEntitiesT::OTHERENTS_CANNOT_PUSH)
            {
                // The move is blocked and thus cannot be completed. The transform of each entity that has already
                // been moved will automatically be restored by the destructor of the OriginalTransforms instance.
                return false;
            }

            // Transform the Ent entity.
            IntrusivePtrT<ComponentTransformT> EntTrafo = Ent->GetTransform();

            const Vector3fT    EntOriginPS = EntTrafo->GetOriginPS();
            const QuaternionfT EntQuatPS   = EntTrafo->GetQuatPS();

            if (Offset != Vector3fT(0, 0, 0))
            {
                OriginalTransforms.StoreOrigin(Ent);
                EntTrafo->SetOriginWS(EntTrafo->GetOriginWS() + Offset);
            }

            // if (Angles != Angles3fT(0, 0, 0))
            // {
            //     ...;
            // }

            // Determine the PosCat after both Part and Ent have been moved.
            const EntityPosCatT NewPosCat = GetEntityPosCat(Ent, PartClipModels);

            if (NewPosCat == POSCAT_STANDING_ON)
            {
                // Moving Ent was ok!
                continue;
            }

            if (NewPosCat == POSCAT_CLEAR && OldPosCat == POSCAT_IN_SOLID)
            {
                // Moving Ent was ok!
                continue;
            }

            if (NewPosCat == POSCAT_IN_SOLID && OldPosCat == POSCAT_IN_SOLID)
            {
                if (m_OtherEntities.Get() == VarOtherEntitiesT::OTHERENTS_CAN_FORCE_PUSH)
                {
                    // Moving Ent was ok!
                    continue;
                }

                // The move is blocked and thus cannot be completed. The transform of each entity that has already
                // been moved will automatically be restored by the destructor of the OriginalTransforms instance.
                return false;
            }

            // At this point, only four cases are left:
            //
            //   - After Part was moved, Ent was found to be clear, and moving Ent either left it
            //     clear or got it stuck in solid (but did not restore its "standing on Part").
            //
            //   - After Part was moved, Ent was found to be standing on Part, and moving Ent
            //     either made it clear again (Part's move put some rug under Ent's feet that
            //     wasn't there before) or got Ent stuck in solid.
            //
            // Therefore, we restore Ent at the previous, valid position now, and leave it there.
            // Note that restoring cannot rely on the data kept in OriginalTransforms, because it
            // only keeps Ent's very initial position, whereas in a multi-part move, Ent may have
            // already undergone a valid move in a previous iteraton by another part.
            EntTrafo->SetOriginPS(EntOriginPS);
            EntTrafo->SetQuatPS(EntQuatPS);
        }
    }

    // The move completed successfully, so tell the OriginalTransformsT instance
    // that the transforms of the affected entities should *not* be restored.
    OriginalTransforms.SetMoveSuccessful();
    return true;
}


static const cf::TypeSys::MethsDocT META_HandleMove =
{
    "HandleMove",
    "This is the main method of this component: [...]\n"
    "@param FrameTime   The time across which the parts are moved.",
    "", "(number FrameTime)"
};

int ComponentMoverT::HandleMove(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentMoverT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentMoverT> >(1);

    const float FrameTime = float(luaL_checknumber(LuaState, 2));
    const bool  Success   = Comp->HandleMove(FrameTime);

    lua_pushboolean(LuaState, Success);
    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentMoverT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "target component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentMoverT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentMoverT();
}

const luaL_Reg ComponentMoverT::MethodsList[] =
{
    { "HandleMove", HandleMove },
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentMoverT::DocMethods[] =
{
    META_HandleMove,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentMoverT::TypeInfo(GetComponentTIM(), "GameSys::ComponentMoverT", "GameSys::ComponentBaseT", ComponentMoverT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
