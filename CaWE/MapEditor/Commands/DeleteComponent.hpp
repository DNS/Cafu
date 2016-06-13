/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAPEDITOR_COMMAND_DELETE_COMPONENT_HPP_INCLUDED
#define CAFU_MAPEDITOR_COMMAND_DELETE_COMPONENT_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GameSys { class ComponentBaseT; } }
namespace cf { namespace GameSys { class EntityT; } }
class MapDocumentT;


namespace MapEditor
{
    class CommandDeleteComponentT : public CommandT
    {
        public:

        CommandDeleteComponentT(MapDocumentT* MapDocument, IntrusivePtrT<cf::GameSys::EntityT> Entity, unsigned long Index);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        MapDocumentT*                              m_MapDocument;
        IntrusivePtrT<cf::GameSys::EntityT>        m_Entity;
        IntrusivePtrT<cf::GameSys::ComponentBaseT> m_Component;
        const unsigned long                        m_Index;
    };
}

#endif
