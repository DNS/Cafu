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

#ifndef CAFU_MAP_BRUSH_HPP_INCLUDED
#define CAFU_MAP_BRUSH_HPP_INCLUDED

#include "MapPrimitive.hpp"


class MapHelperT;


/// This class provides a graphical representation of an entity in the Map Editor.
class MapEntRepresT : public MapPrimitiveT
{
    public:

    /// The constructor.
    MapEntRepresT();

    /// The copy constructor.
    /// @param MapRepres   The entity representation to copy-construct the new instance from.
    MapEntRepresT(const MapEntRepresT& EntRepres);

    /// Called by the hosting entity instance whenever it has changed.
    void Update();


    // Implementations and overrides for base class methods.
    MapEntRepresT* Clone() const;
    void           Assign(const MapElementT* Elem);
    void           Render2D(Renderer2DT& Renderer) const;
    void           Render3D(Renderer3DT& Renderer) const;
    bool           IsTranslucent() const;
    BoundingBox3fT GetBB() const;
    bool           TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const;
    bool           TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;
    void           Save_cmap(std::ostream& OutFile, unsigned long EntRepNr, const MapDocumentT& MapDoc) const;
    wxString       GetDescription() const;

    // Implement the MapElementT transformation methods.
    void TrafoMove(const Vector3fT& Delta);
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles);
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale);
    void TrafoMirror(unsigned int NormalAxis, float Dist);
    void Transform(const MatrixT& Matrix);


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    MapHelperT* m_Helper;
};

#endif
