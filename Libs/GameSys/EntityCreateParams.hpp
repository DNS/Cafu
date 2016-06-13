/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_ENTITY_CREATE_PARAMS_HPP_INCLUDED
#define CAFU_GAMESYS_ENTITY_CREATE_PARAMS_HPP_INCLUDED

#include "TypeSys.hpp"


namespace cf
{
    namespace GameSys
    {
        class WorldT;


        /// Creation parameters for a game entity.
        class EntityCreateParamsT : public cf::TypeSys::CreateParamsT
        {
            public:

            /// The constructor.
            /// @param World_   The world in which the entity should be created.
            EntityCreateParamsT(WorldT& World_);

            /// This method is used on the client to create entities with the ID sent from the server,
            /// not the automatically created ID that would otherwise (normally) be used.
            void ForceID(unsigned int ID);

            /// Returns the "forced" ID that is to be used for the new entity,
            /// or `UINT_MAX` if the normal ID should be used.
            unsigned int GetID() const { return m_ID; }


            WorldT& World;  ///< The world in which the entity should be created.


            private:

            unsigned int m_ID;
        };
    }
}

#endif
