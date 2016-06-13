/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_REPARENT_PRIMITIVE_HPP_INCLUDED
#define CAFU_COMMAND_REPARENT_PRIMITIVE_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Templates/Pointer.hpp"


namespace MapEditor { class CompMapEntityT; }
class MapDocumentT;
class MapPrimitiveT;


namespace MapEditor
{
    /// A command to assign one or several map primitives to another entity.
    class CommandReparentPrimitiveT : public CommandT
    {
        public:

        /// The constructor for assigning a map primitive to another entity.
        CommandReparentPrimitiveT(MapDocumentT& MapDoc, MapPrimitiveT* Prim, IntrusivePtrT<CompMapEntityT> NewParent);

        /// The constructor for assigning several map primitives to another entity.
        CommandReparentPrimitiveT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& Prims, IntrusivePtrT<CompMapEntityT> NewParent);

        // Implementation of the CommandT interface.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        MapDocumentT&                           m_MapDoc;
        ArrayT<MapPrimitiveT*>                  m_Prims;
        ArrayT< IntrusivePtrT<CompMapEntityT> > m_OldParents;
        IntrusivePtrT<CompMapEntityT>           m_NewParent;
    };
}

#endif
