/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
    void Transform(const Matrix4x4fT& Matrix, bool LockTexCoords) override;


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    MapEntRepresT(const MapEntRepresT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const MapEntRepresT&);     ///< Use of the Assignment Operator is not allowed.

    bool           IsPlayerPrototypeChild() const;
    BoundingBox3fT GetRepresBB() const;
};

#endif
