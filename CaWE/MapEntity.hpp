/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_MAP_ENTITY_HPP_INCLUDED
#define CAFU_MAP_ENTITY_HPP_INCLUDED

#include "MapEntityBase.hpp"


class MapHelperT;


class MapEntityT : public MapEntityBaseT
{
    public:

    /// The default constructor.
    MapEntityT();

    /// The copy constructor for copying an entity.
    /// @param Entity   The entity to copy-construct this entity from.
    MapEntityT(const MapEntityT& Entity);

    /// The destructor.
    ~MapEntityT();


    // Implementations and overrides for base class methods.
    MapEntityT* Clone() const;
    void        Assign(const MapElementT* Elem);

    wxColour GetColor(bool ConsiderGroup=true) const;
    wxString GetDescription() const;

    void Render2D(Renderer2DT& Renderer) const;
    void Render3D(Renderer3DT& Renderer) const;
    BoundingBox3fT GetBB() const;

    bool TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const;
    bool TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;
    void TrafoMove(const Vector3fT& Delta);
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles);
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale);
    void TrafoMirror(unsigned int NormalAxis, float Dist);
    void Transform(const MatrixT& Matrix);

    void SetClass(const EntityClassT* NewClass);


    /// Checks if unique values are set and unique inside the world and changes/sets them if bool Repair is true (default).
    /// @return Properties that are flagged as unique, but haven't (or hadn't, if repaired) unique values.
    ArrayT<EntPropertyT> CheckUniqueValues(MapDocumentT& MapDoc, bool Repair=true);

    Vector3fT GetOrigin() const;
    void SetOrigin(const Vector3fT& Origin);


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    void UpdateHelpers();

    Vector3fT           m_Origin;
    ArrayT<MapHelperT*> m_Helpers;
};

#endif
