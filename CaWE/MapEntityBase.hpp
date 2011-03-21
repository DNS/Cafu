/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _MAP_ENTITY_BASE_HPP_
#define _MAP_ENTITY_BASE_HPP_

#include "EntProperty.hpp"
#include "MapElement.hpp"


class EntityClassT;
class MapPrimitiveT;
class wxProgressDialog;


class MapEntityBaseT : public MapElementT
{
    public:

    /// The default constructor.
    MapEntityBaseT(const wxColour& Color);

    /// The copy constructor for copying a base entity.
    /// @param Ent   The base entity to copy-construct this base entity from.
    MapEntityBaseT(const MapEntityBaseT& Ent);

    /// The destructor.
    ~MapEntityBaseT();

    // Implementations and overrides for base class methods.
 // MapEntityBaseT* Clone() const;
    void            Assign(const MapElementT* Elem);

    void Load_cmap   (TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Load_HL1_map(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Load_HL2_vmf(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Load_D3_map (TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr);
    void Save_cmap(const MapDocumentT& MapDoc, std::ostream& OutFile, unsigned long EntityNr, const BoundingBox3fT* Intersecting) const;


    virtual void SetClass(const EntityClassT* NewClass);
    const EntityClassT* GetClass() const { return m_Class; }

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


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    const EntityClassT*    m_Class;         ///< The "entity class" of this entity.
    ArrayT<EntPropertyT>   m_Properties;    ///< The concrete, instantiated properties for this entity, according to its entity class.
    ArrayT<MapPrimitiveT*> m_Primitives;    ///< The primitive, atomic elements of this entity (brushes, patches, terrains, models, plants, ...).
};

#endif
