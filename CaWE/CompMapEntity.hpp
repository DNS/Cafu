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

#ifndef CAFU_MAPEDITOR_COMPONENT_MAP_ENTITY_HPP_INCLUDED
#define CAFU_MAPEDITOR_COMPONENT_MAP_ENTITY_HPP_INCLUDED

#include "EntProperty.hpp"
#include "GameSys/CompBase.hpp"
#include "GameSys/Entity.hpp"       // For GetMapEnt() only.
#include "Math3D/Angles.hpp"
#include "Math3D/BoundingBox.hpp"


class MapDocumentT;
class MapEntRepresT;
class MapPrimitiveT;
class wxProgressDialog;


namespace MapEditor
{
    /// This component houses the Map Editor specific parts of its entity.
    /// It is intended for use by the Map Editor application only, that is, as the "App" component of `cf::GameSys::EntityT`s.
    /// As such, it doesn't integrate with the TypeSys, and thus isn't available for scripting and whereever else we need
    /// the related meta-data.
    class CompMapEntityT : public cf::GameSys::ComponentBaseT
    {
        public:

        /// The constructor.
        CompMapEntityT(MapDocumentT& MapDoc);

        /// The copy constructor. It creates a new component as a copy of another component.
        ///
        /// Note that the new map entity is created without any primitives, that is, the primitives are
        /// intentionally *not* copied from `Comp`. This is because it is easy to call CopyPrimitives()
        /// with the new instance if the user code wants to have the map primitives copied as well,
        /// whereas the reverse would be very difficult to handle.
        ///
        /// (If primitives were copied per default, having them not copied when they are not wanted
        /// would require another parameter to the copy constructor -- which had to be passed through by
        /// the EntityT copy constructors and Clone() methods as well as by all Clone() methods in the
        /// ComponentBaseT hierarchy. This would be difficult to understand and bother large amounts of
        /// completely unrelated code.)
        ///
        /// @param Comp   The component to copy-construct this component from.
        CompMapEntityT(const CompMapEntityT& Comp);

        /// The destructor.
        ~CompMapEntityT();

        // Base class overrides.
        CompMapEntityT* Clone() const;
        const char* GetName() const { return "MapEntity"; }
        void Render() const;


        void Load_cmap   (TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr, unsigned int& cmapVersion);
        void Load_HL1_map(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
        void Load_HL2_vmf(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
        void Load_D3_map (TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
        void Save_cmap(const MapDocumentT& MapDoc, std::ostream& OutFile, unsigned long EntityNr, const BoundingBox3fT* Intersecting) const;


        MapDocumentT& GetDoc() const { return m_MapDoc; }

        bool IsWorld() const;

        const ArrayT<EntPropertyT>& GetProperties() const { return m_Properties; }
              ArrayT<EntPropertyT>& GetProperties()       { return m_Properties; }

        EntPropertyT*       FindProperty     (const wxString& Key, int* Index=NULL, bool Create=false); ///< Find the property.
        const EntPropertyT* FindProperty     (const wxString& Key, int* Index=NULL) const;              ///< Find the property.
        void                RemoveProperty(const wxString& Key);                                        ///< Remove this property.
        std::string         GetProperty(const wxString& Key, const char* Default="") const;             ///< Returns the value of this property, using the default if not found.
        std::string         GetAndRemove(const wxString& Key, const char* Default="");                  ///< Returns the value of this property, using the default if not found, and removes it.

        void CopyPrimitives(const CompMapEntityT& MapEnt);  ///< Creates a copy of each primitive in MapEnt and adds it to this instance.
        const ArrayT<MapPrimitiveT*>& GetPrimitives() const { return m_Primitives; }

        void AddPrim(MapPrimitiveT* Prim);
        void RemovePrim(MapPrimitiveT* Prim);

        MapEntRepresT* GetRepres() const { return m_Repres; }

        /// Returns the "overall" bounding-box of this entity.
        /// The returned bounding-box contains all elements (the representation and all primitives) of this entity.
        BoundingBox3fT GetElemsBB() const;


        private:

        MapDocumentT&          m_MapDoc;        ///< The document that contains, keeps and manages this entity.
        ArrayT<EntPropertyT>   m_Properties;    ///< Properties associated with this entity. Obsolete, kept so that we can import old or foreign map file formats.
        MapEntRepresT*         m_Repres;        ///< The graphical representation of this entity in the map.
        ArrayT<MapPrimitiveT*> m_Primitives;    ///< The primitive, atomic elements of this entity (brushes, patches, terrains, models, plants, ...).
    };


    inline IntrusivePtrT<CompMapEntityT> GetMapEnt(IntrusivePtrT<cf::GameSys::EntityT> Entity)
    {
        return dynamic_pointer_cast<CompMapEntityT>(Entity->GetApp());
    }
}

#endif
