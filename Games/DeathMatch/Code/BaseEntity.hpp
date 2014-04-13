/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_BASE_ENTITY_HPP_INCLUDED
#define CAFU_BASE_ENTITY_HPP_INCLUDED

#include "../../GameEntity.hpp"
#include "ClipSys/ClipModel.hpp"
#include "GameSys/Entity.hpp"

#include <map>

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


class PhysicsWorldT;
struct lua_State;
namespace cf { namespace ClipSys { class CollisionModelT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }


namespace GAME_NAME
{
    class EntityCreateParamsT;
    class ApproxBaseT;


    /// The TypeInfoTs of all BaseEntityT derived classes must register with this TypeInfoManT instance.
    cf::TypeSys::TypeInfoManT& GetBaseEntTIM();


    // This structure describes each entitys state and is transmitted from the server to the clients over the network.
    struct EntityStateT
    {
        char           StateOfExistance;        // For entity defined state machines, e.g. "specator, dead, alive, ...".
        char           PlayerName[64];          // If it is a human player, this is its name. Usually unused otherwise.
     // ArrayT<char>   PlayerName;

        char           Health;                  // Health.
        char           Armor;                   // Armor.
        unsigned long  HaveItems;               // Bit field, entity can carry 32 different items.
        unsigned long  HaveWeapons;             // Bit field, entity can carry 32 different weapons.
        char           ActiveWeaponSlot;        // Index into HaveWeapons, HaveAmmoInWeapons, and for determining the weapon model index.
        char           ActiveWeaponSequNr;      // The weapon sequ. that WE see (the LOCAL clients VIEW weapon model). Could (and should in the future) also be used for other clients PLAYER weapon model, but currently is not.
        float          ActiveWeaponFrameNr;     // Respectively, this is the frame number of the current weapon sequence.
        unsigned short HaveAmmo[16];            // Entity can carry 16 different types of ammo (weapon independent). This is the amount of each.
        unsigned char  HaveAmmoInWeapons[32];   // Entity can carry ammo in each of the 32 weapons. This is the amount of each.

        EntityStateT(char StateOfExistance_,
                     char Health_, char Armor_, unsigned long HaveItems_, unsigned long HaveWeapons_,
                     char ActiveWeaponSlot_, char ActiveWeaponSequNr_, float ActiveWeaponFrameNr_);
    };


    // This class describes "base entities", the most central component in game<-->engine communication.
    class BaseEntityT : public GameEntityI
    {
        public:

        const unsigned long ID;             // The unique ID of this entity.
        const std::map<std::string, std::string> Properties;    ///< The properties of this entities from the map file.
        unsigned long       ParentID;       // The 'ID' of the entity that created us.
     // ID[]                ChildrenIDs;    // The entities that we have created (e.g. the rockets that a human player fired).

        IntrusivePtrT<cf::GameSys::EntityT> m_Entity;       ///< The associated entity in the cf::GameSys::WorldT.
        cf::GameSys::GameWorldI*            GameWorld;      ///< Pointer to the game world implementation.
        const cf::ClipSys::CollisionModelT* CollisionModel; ///< The collision model of this entity, NULL for none.
        cf::ClipSys::ClipModelT             ClipModel;      ///< The clip model of this entity. Note that clip models can take NULL collision model pointers, so that the ClipModel instance is always non-NULL and available.


        /// The destructor.
        virtual ~BaseEntityT();

        /// The implementation calls DoSerialize(), that derived classes override to add their own data.
        void Serialize(cf::Network::OutStreamT& Stream) const /*final override*/;

        /// The implementation calls DoDeserialize(), that derived classes override to read their own data.
        /// It also calls ProcessEvent() (overridden by derived classes) for any received events.
        void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting) /*final override*/;

        // Implement GameEntityI base class methods.
        virtual void NotifyLeaveMap() { }
        virtual unsigned long GetID() const { return ID; }
        virtual cf::GameSys::GameWorldI* GetGameWorld() const { return GameWorld; }
        virtual const BoundingBox3dT& GetDimensions() const { return m_Dimensions; }
        virtual void GetBodyOrientation(unsigned short& h, unsigned short& p, unsigned short& b) const { h=0; p=0; b=0; /*TODO: this method is called from obsolete methods only...*/ }


        /// This SERVER-SIDE function is used to notify this entity that it was touched by another entity.
        /// 'Entity' is the entity that touches this one, and is usually the entity from which the call is made.
        /// Calls are only made from within other entities 'Think()' functions.
        /// (Unfortunately, this function cannot be declared as "protected": see "C++ FAQs, 2nd edition" by Cline, Lomow, Girou, pages 249f.)
        virtual void NotifyTouchedBy(BaseEntityT* Entity);

        /// This SERVER-SIDE method is called whenever another entity walked into one of our trigger volumes (trigger brushes).
        /// It is called once per frame as long as the other entity stays within our trigger volume.
        ///
        /// @param Activator   The entity that walked into our trigger volume and thus caused the call of this method.
        ///
        /// Note that most entity classes just do nothing in their implementation of this method, usually because they never have trigger brushes anyway.
        /// Entity classes that are very likely to provide an implementation though are EntTriggerT (of course!) and the items like weapons,
        /// so they can e.g. detect being picked up.
        /// Calls to this method normally come from within the Activator->Think() method, because it are the activators themselves that detect
        /// that they entered a trigger volume (it are not the entities with the triggers that detect that something entered their volume).
        /// (Unfortunately, this function cannot be declared as "protected": see "C++ FAQs, 2nd edition" by Cline, Lomow, Girou, pages 249f.)
        virtual void OnTrigger(BaseEntityT* Activator);

        /// This SERVER-SIDE function is used to have this entity take damage.
        /// 'Entity' is the entitiy that causes the damage (i.e., who fired a shot). It is usually the entity from which the call is made.
        /// 'Amount' is the amount of damage that was caused, and is usually subtracted from this entitys health.
        /// 'ImpactDir' is the direction from which we were hit.
        /// Calls are only made from within other entities 'Think()' functions.
        /// (Unfortunately, this function cannot be declared as "protected": see "C++ FAQs, 2nd edition" by Cline, Lomow, Girou, pages 249f.)
        virtual void TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir);

        /// This SERVER-SIDE function is used for posting an event of the given type.
        /// The event is automatically sent from the entity instance on the server to the entity instances
        /// on the clients, and causes a matching call to ProcessEvent() there.
        /// The meaning of the event type is specific to the concrete entity class.
        /// Note that events are fully predictable: they work well even in the presence of client prediction.
        void PostEvent(unsigned int EventType) { m_EventsCount[EventType]++; }

        // Implement SERVER-SIDE GameEntityI base class methods.
        virtual void ProcessConfigString(const void* ConfigData, const char* ConfigString) /*override*/;
        virtual void Think(float FrameTime, unsigned long ServerFrameNr) /*override*/;


        /// This CLIENT-SIDE function is called to process events on the client.
        /// Events that have been posted via PostEvent() on the server (or in client prediction) are eventually
        /// received in Deserialize(), which automatically calls this method.
        /// In the call, Deserialize() indicates the number of new events since the last call (at least one,
        /// or it wouldn't call ProcessEvent() at all). This way, each event can be processed exactly once.
        ///
        /// TODO: Move into private section below "DoDeserialize()" ??
        virtual void ProcessEvent(unsigned int EventType, unsigned int NumEvents);

        // Implement CLIENT-SIDE GameEntityI base class methods.
        virtual bool GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const /*override*/;
        virtual void Draw(bool FirstPersonView, float LodDist) const /*override*/;
        virtual void Interpolate(float FrameTime) /*final override*/;
        virtual void PostDraw(float FrameTime, bool FirstPersonView) /*override*/;


        /// Returns the proper type info for this entity.
        virtual const cf::TypeSys::TypeInfoT* GetType() const;
        static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
        static const cf::TypeSys::TypeInfoT TypeInfo;     ///< The type info object for (objects/instances of) this class.


        // Methods provided to be called from the map/entity Lua scripts.
        static int GetName(lua_State* L);


        protected:

        /// Protected constructor such that only concrete entities can call this for creating a BaseEntityT, but nobody else.
        /// Concrete entities are created in the GameI::CreateGameEntity() method for both the server- and client-side.
        BaseEntityT(const EntityCreateParamsT& Params, const BoundingBox3dT& Dimensions, const unsigned int NUM_EVENT_TYPES);

        /// Concrete entities call this method in their constructors in order to have us automatically interpolate
        /// the value that has been specified with the Interp instance.
        /// Interpolation occurs on the client only: it is advanced in the video frames between server updates.
        /// Each update received from the server (applied in DoDeserialize()) automatically updates the reference value
        /// for the interpolation in the next video frames.
        void Register(ApproxBaseT* Interp);

        BoundingBox3dT m_Dimensions;    ///< The bounding box of this entity (relative to the origin).


        private:

        BaseEntityT(const BaseEntityT&);        ///< Use of the Copy    Constructor is not allowed.
        void operator = (const BaseEntityT&);   ///< Use of the Assignment Operator is not allowed.

        /// Derived classes override this method in order to write their state into the given stream.
        /// The method itself is automatically called from Serialize(), see Serialize() for more details.
        ///
        /// (This follows the "Non-Virtual Interface Idiom" as described by Scott Meyers in
        /// "Effective C++, 3rd Edition", item 35 ("Consider alternatives to virtual functions.").)
        virtual void DoSerialize(cf::Network::OutStreamT& Stream) const { }

        /// Derived classes override this method in order to read their state from the given stream.
        /// The method itself is automatically called from Deserialize(), see Deserialize() for more details.
        ///
        /// (This follows the "Non-Virtual Interface Idiom" as described by Scott Meyers in
        /// "Effective C++, 3rd Edition", item 35 ("Consider alternatives to virtual functions.").)
        virtual void DoDeserialize(cf::Network::InStreamT& Stream) { }

        ArrayT<uint32_t>     m_EventsCount;     ///< A counter for each event type for the number of its occurrences. Serialized (and deserialized) normally along with the entity state.
        ArrayT<uint32_t>     m_EventsRef;       ///< A reference counter for each event type for the number of processed occurrences. Never serialized (or deserialized), never reset, strictly growing.

        ArrayT<ApproxBaseT*> m_Interpolators;   ///< The interpolators for advancing values in the client video frames between server updates. The values that are interpolated are specified by calls to Register() by the derived entity class.
    };
}

#endif
