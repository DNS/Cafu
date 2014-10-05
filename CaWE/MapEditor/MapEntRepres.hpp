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

    /// The copy constructor.
    ///
    /// When a MapEntRepresT is copied, the entity that it represents is copied as well.
    /// (Only the entity itself is copied, but not its primitives.)
    ///
    /// The new entity instance is held by the new MapEntRepresT, and can be obtained via GetParent() as expected.
    /// As a result, the new entity has *two* representations:
    /// This one (that holds it), and the one that is held by it (in its CompMapEntityT component).
    /// It is OK to delete this instance and to keep the created entity.
    ///
    /// @param EntRepres   The entity representation to copy-construct the new instance from.
    MapEntRepresT(const MapEntRepresT& EntRepres);


    // Implementations and overrides for base class methods.
    MapEntRepresT* Clone() const;
    void           Assign(const MapElementT* Elem);

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
    void TrafoMove(const Vector3fT& Delta) override;
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles) override;
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale) override;
    void TrafoMirror(unsigned int NormalAxis, float Dist) override;
    void Transform(const MatrixT& Matrix) override;


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    BoundingBox3fT GetRepresBB() const;

    IntrusivePtrT<cf::GameSys::EntityT> m_Cloned;   ///< If the MapEntRepresT has been cloned, the related (cloned) entity instance is stored here. Note that we can *not* get rid of this variable by turning m_Parent into an IntrusivePtrT<>: Doing so would create a cycle and cause entity instances to never get freed, leaking memory.
};

#endif
