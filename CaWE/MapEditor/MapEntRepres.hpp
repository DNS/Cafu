/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_MAP_ENTITY_REPRES_HPP_INCLUDED
#define CAFU_MAP_ENTITY_REPRES_HPP_INCLUDED

#include "MapElement.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GameSys { class EntityT; } }


/// This class provides a graphical representation of an entity in the Map Editor.
/// Contrary to all other MapElementT-derived classes, the parent entity of a MapEntRepresT can never be `NULL`,
/// because a representation cannot exist without the object that it represents.
class MapEntRepresT : public MapElementT
{
    public:

    /// The constructor.
    MapEntRepresT(MapEditor::CompMapEntityT* Parent);

    // Implementations and overrides for base class methods.
    wxColour       GetColor(bool ConsiderGroup=true) const;
    wxString       GetDescription() const;

    void           Render2D(Renderer2DT& Renderer) const;
    void           Render3D(Renderer3DT& Renderer) const;
    bool           IsTranslucent() const;
    BoundingBox3fT GetBB() const;
    bool           TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const;
    bool           TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;

    // Implement the MapElementT transformation methods.
    TrafoMementoT* GetTrafoState() const override;
    void RestoreTrafoState(const TrafoMementoT* TM) override;
    void TrafoMove(const Vector3fT& Delta, bool LockTexCoords) override;
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords) override;
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords) override;
    void TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords) override;
    void Transform(const MatrixT& Matrix, bool LockTexCoords) override;


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    MapEntRepresT(const MapEntRepresT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const MapEntRepresT&);     ///< Use of the Assignment Operator is not allowed.

    BoundingBox3fT GetRepresBB() const;
};

#endif
