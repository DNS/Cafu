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

#ifndef CAFU_MAP_ENTITY_BASE_HPP_INCLUDED
#define CAFU_MAP_ENTITY_BASE_HPP_INCLUDED

#include "EntProperty.hpp"
#include "Math3D/Angles.hpp"
#include "Math3D/BoundingBox.hpp"


class EntityClassT;
class MapDocumentT;
class MapEntRepresT;
class MapPrimitiveT;
class wxProgressDialog;


class MapEntityBaseT
{
    public:

    /// The default constructor.
    MapEntityBaseT(MapDocumentT& MapDoc);

    /// The copy constructor for copying a base entity.
    /// @param Ent         The base entity to copy-construct this base entity from.
    /// @param CopyPrims   Whether the primitives of `Ent` should be copied into the new entity as well.
    ///                    If `false`, the new entity will be created without any primitives.
    MapEntityBaseT(const MapEntityBaseT& Ent, bool CopyPrims=true);

    /// The destructor.
    ~MapEntityBaseT();

    // Implementations and overrides for base class methods.
    void Load_cmap   (TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Load_HL1_map(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Load_HL2_vmf(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Load_D3_map (TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Save_cmap(const MapDocumentT& MapDoc, std::ostream& OutFile, unsigned long EntityNr, const BoundingBox3fT* Intersecting) const;


    MapDocumentT& GetDoc() const { return m_MapDoc; }

    bool IsWorld() const;

    virtual void SetClass(const EntityClassT* NewClass);
    const EntityClassT* GetClass() const { return m_Class; }

    Vector3fT GetOrigin() const;
    void SetOrigin(const Vector3fT& Origin);

    const ArrayT<EntPropertyT>& GetProperties() const { return m_Properties; }
          ArrayT<EntPropertyT>& GetProperties()       { return m_Properties; }

    EntPropertyT*       FindProperty     (const wxString& Key, int* Index=NULL, bool Create=false); ///< Find the property.
    const EntPropertyT* FindProperty     (const wxString& Key, int* Index=NULL) const;              ///< Find the property.
    int                 FindPropertyIndex(const wxString& Key) const;                               ///< Get the index of the property.

    virtual void               SetAngles(const cf::math::AnglesfT& Angles);
    virtual cf::math::AnglesfT GetAngles() const;

    const ArrayT<MapPrimitiveT*>& GetPrimitives() const { return m_Primitives; }

    void AddPrim(MapPrimitiveT* Prim);
    void RemovePrim(MapPrimitiveT* Prim);

    MapEntRepresT* GetRepres() const { return m_Repres; }

    /// Returns the "overall" bounding-box of this entity.
    /// The returned bounding-box contains all elements (the representation and all primitives) of this entity.
    BoundingBox3fT GetElemsBB() const;

    /// Checks if unique values are set and unique inside the world and changes/sets them if bool Repair is true (default).
    /// @return Properties that are flagged as unique, but haven't (or hadn't, if repaired) unique values.
    ArrayT<EntPropertyT> CheckUniqueValues(bool Repair=true);


    private:

    friend class MapEntRepresT;             ///< For mutual construction and destruction.

    /// The copy constructor that is used when a MapEntRepresT is copied.
    MapEntityBaseT(const MapEntityBaseT& Ent, MapEntRepresT* Repres);

    MapDocumentT&          m_MapDoc;        ///< The document that contains, keeps and manages this world.
    const EntityClassT*    m_Class;         ///< The "entity class" of this entity.
    Vector3fT              m_Origin;        ///< The origin of this entity.
    ArrayT<EntPropertyT>   m_Properties;    ///< The concrete, instantiated properties for this entity, according to its entity class.
    MapEntRepresT*         m_Repres;        ///< The graphical representation of this entity in the map.
    ArrayT<MapPrimitiveT*> m_Primitives;    ///< The primitive, atomic elements of this entity (brushes, patches, terrains, models, plants, ...).
};

#endif
