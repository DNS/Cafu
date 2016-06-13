/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAPEDITOR_COMMAND_CHANGE_ENTITY_HIERARCHY_HPP_INCLUDED
#define CAFU_MAPEDITOR_COMMAND_CHANGE_ENTITY_HIERARCHY_HPP_INCLUDED

#include "../../CommandPattern.hpp"

#include "Math3D/Quaternion.hpp"
#include "Templates/Pointer.hpp"


class MapDocumentT;
namespace cf { namespace GameSys { class EntityT; } }


namespace MapEditor
{
    class CommandChangeEntityHierarchyT : public CommandT
    {
        public:

        CommandChangeEntityHierarchyT(MapDocumentT* MapDoc, IntrusivePtrT<cf::GameSys::EntityT> Entity, IntrusivePtrT<cf::GameSys::EntityT> NewParent, unsigned long NewPosition);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        MapDocumentT*                       m_MapDoc;
        IntrusivePtrT<cf::GameSys::EntityT> m_Entity;
        const Vector3fT                     m_OriginWS;
        const cf::math::QuaternionfT        m_QuatWS;
        IntrusivePtrT<cf::GameSys::EntityT> m_NewParent;
        unsigned long                       m_NewPosition;
        IntrusivePtrT<cf::GameSys::EntityT> m_OldParent;
        const unsigned long                 m_OldPosition;
        const std::string                   m_OldName;
    };
}

#endif
