/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_ALL_COMPONENTS_HPP_INCLUDED
#define CAFU_GAMESYS_ALL_COMPONENTS_HPP_INCLUDED

#include "TypeSys.hpp"


namespace cf
{
    namespace GameSys
    {
        /// All classes in the ComponentBaseT hierarchy must register their TypeInfoT
        /// members with this TypeInfoManT instance.
        cf::TypeSys::TypeInfoManT& GetComponentTIM();

        /// Returns whether the given component type is fundamental to its parent entity.
        /// Fundamental components are explicit C++ members of class EntityT. Users cannot delete them in the Map
        /// Editor or via script, and cannot add new ones in the Map Editor. (It is possible to add additional
        /// ones via script, but not useful.)
        /// Note that most components are not fundamental, but "custom": Users can freely add or delete them in
        /// the Map Editor or via scripts.
        bool IsFundamental(const cf::TypeSys::TypeInfoT* CompType);
    }
}

#endif
