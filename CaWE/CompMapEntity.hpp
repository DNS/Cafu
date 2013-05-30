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
#include "MapEntityBase.hpp"
#include "GameSys/CompBase.hpp"
#include "GameSys/Entity.hpp"       // For GetMapEnt() only.
#include "Math3D/Angles.hpp"
#include "Math3D/BoundingBox.hpp"


class EntityClassT;
class MapDocumentT;
class MapEntRepresT;
class MapPrimitiveT;
class wxProgressDialog;


namespace MapEditor
{
    inline IntrusivePtrT<CompMapEntityT> GetMapEnt(IntrusivePtrT<cf::GameSys::EntityT> Entity)
    {
        return dynamic_pointer_cast<CompMapEntityT>(Entity->GetApp());
    }


    /// This component houses the Map Editor specific parts of its entity.
    /// It is intended for use by the Map Editor application only, that is, as the "App" component of `cf::GameSys::EntityT`s.
    /// As such, it doesn't integrate with the TypeSys, and thus isn't available for scripting and whereever else we need
    /// the related meta-data.
    class CompMapEntityT : public cf::GameSys::ComponentBaseT
    {
        public:

        /// The constructor.
        CompMapEntityT(MapDocumentT& MapDoc);

        /// The copy constructor.
        /// @param Comp   The component to create a copy of.
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


        MapDocumentT& GetDoc() const { return m_MapEntity->m_MapDoc; }

        bool IsWorld() const;

        void SetClass(const EntityClassT* NewClass);
        const EntityClassT* GetClass() const { return m_MapEntity->m_Class; }

        Vector3fT GetOrigin() const;
        void SetOrigin(const Vector3fT& Origin);

        const ArrayT<EntPropertyT>& GetProperties() const { return m_MapEntity->m_Properties; }
              ArrayT<EntPropertyT>& GetProperties()       { return m_MapEntity->m_Properties; }

        EntPropertyT*       FindProperty     (const wxString& Key, int* Index=NULL, bool Create=false); ///< Find the property.
        const EntPropertyT* FindProperty     (const wxString& Key, int* Index=NULL) const;              ///< Find the property.
        int                 FindPropertyIndex(const wxString& Key) const;                               ///< Get the index of the property.

        void               SetAngles(const cf::math::AnglesfT& Angles);
        cf::math::AnglesfT GetAngles() const;

        const ArrayT<MapPrimitiveT*>& GetPrimitives() const { return m_MapEntity->m_Primitives; }

        void AddPrim(MapPrimitiveT* Prim);
        void RemovePrim(MapPrimitiveT* Prim);

        MapEntRepresT* GetRepres() const { return m_MapEntity->m_Repres; }

        /// Returns the "overall" bounding-box of this entity.
        /// The returned bounding-box contains all elements (the representation and all primitives) of this entity.
        BoundingBox3fT GetElemsBB() const;

        /// Checks if unique values are set and unique inside the world and changes/sets them if bool Repair is true (default).
        /// @return Properties that are flagged as unique, but haven't (or hadn't, if repaired) unique values.
        ArrayT<EntPropertyT> CheckUniqueValues(bool Repair=true);


        private:

        MapEntityBaseT* m_MapEntity;
    };
}

#endif
