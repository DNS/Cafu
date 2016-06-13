/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAP_BRUSH_HPP_INCLUDED
#define CAFU_MAP_BRUSH_HPP_INCLUDED

#include "MapPrimitive.hpp"
#include "MapFace.hpp"
#include "Templates/Array.hpp"


class MapBrushT : public MapPrimitiveT
{
    public:

    /// Constructor for creating a MapBrushT from the convex hull over the given vertices.
    /// @param HullVertices   The vertices over whose convex hull this brush is built.
    /// @param Material       The material to be applied to the new faces.
    /// @param FaceAligned    Whether the u and v axes of the brush faces are initialized face aligned or world aligned.
    /// @param RefBrush       If non-NULL, the data of the faces of RefBrush is reused as much as possible for the faces of the new brush.
    ///                       This is very useful e.g. for the Edit Vertices tool, where edits should generally preserve face data.
    MapBrushT(const ArrayT<Vector3fT>& HullVertices, EditorMaterialI* Material, bool FaceAligned, const MapBrushT* RefBrush=NULL);

    /// Constructor for creating a MapBrushT from the intersection of the given planes.
    /// @param Planes        The planes whose intersection forms this brush.
    /// @param Material      The material to be applied to the new faces.
    /// @param FaceAligned   Whether the u and v axes of the brush faces are initialized face aligned or world aligned.
    MapBrushT(const ArrayT<Plane3fT>& Planes, EditorMaterialI* Material, bool FaceAligned);

    /// The copy constructor for copying a brush.
    /// @param Brush   The brush to copy-construct this brush from.
    MapBrushT(const MapBrushT& Brush);

    // Named constructors for loading brushes from map files.
    static MapBrushT* Create_cmap(TextParserT& TP, MapDocumentT& MapDoc, unsigned long EntityNr, unsigned long BrushNr, bool IgnoreGroups);   ///< EntityNr and BrushNr are provided by the caller, just for better error reporting.
    static MapBrushT* Create_D3_map(TextParserT& TP, const Vector3fT& Origin, unsigned long EntityNr, unsigned long PrimitiveNr, EditorMatManT& MatMan);    ///< EntityNr and PrimitiveNr are provided by the caller, just for better error reporting.
    static MapBrushT* Create_HL1_map(TextParserT& TP, unsigned long EntityNr, unsigned long BrushNr, EditorMatManT& MatMan);    ///< EntityNr and BrushNr are provided by the caller, just for better error reporting.
    static MapBrushT* Create_HL2_vmf(TextParserT& TP, EditorMatManT& MatMan);

    // Named constructors for obtaining stock brushes.
    static MapBrushT* CreateBlock   (const BoundingBox3fT& Box, EditorMaterialI* Material);                                  ///< Named constructor for creating a block    brush.
    static MapBrushT* CreateWedge   (const BoundingBox3fT& Box, EditorMaterialI* Material);                                  ///< Named constructor for creating a wedge    brush.
    static MapBrushT* CreateCylinder(const BoundingBox3fT& Box, const unsigned long NrOfSides, EditorMaterialI* Material);   ///< Named constructor for creating a cylinder brush.
    static MapBrushT* CreatePyramid (const BoundingBox3fT& Box, const unsigned long NrOfSides, EditorMaterialI* Material);   ///< Named constructor for creating a pyramid  brush.
    static MapBrushT* CreateSphere  (const BoundingBox3fT& Box, const unsigned long NrOfSides, EditorMaterialI* Material);   ///< Named constructor for creating a sphere   brush.


    // Implementations and overrides for base class methods.
    MapBrushT*     Clone() const override;
    void           Render2D(Renderer2DT& Renderer) const;
    void           Render3D(Renderer3DT& Renderer) const;
    bool           IsTranslucent() const;
    BoundingBox3fT GetBB() const;
    bool           TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const;
    bool           TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;
    void           Save_cmap(std::ostream& OutFile, unsigned long BrushNr, const MapDocumentT& MapDoc) const;
    wxString       GetDescription() const;

    // Implement the MapElementT transformation methods.
    TrafoMementoT* GetTrafoState() const override;
    void RestoreTrafoState(const TrafoMementoT* TM) override;
    void TrafoMove(const Vector3fT& Delta, bool LockTexCoords) override;
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords) override;
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords) override;
    void TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords) override;
    void Transform(const Matrix4x4fT& Matrix, bool LockTexCoords) override;

    /// This method splits the brush along the given plane and returns the front piece, the back piece, or both.
    /// @param Plane   The plane along which to split this brush.
    /// @param Front   The address of the pointer to the front piece, if the caller is interested in obtaining it (NULL otherwise).
    /// @param Back    The address of the pointer to the back  piece, if the caller is interested in obtaining it (NULL otherwise).
    void Split(const Plane3T<float>& Plane, MapBrushT** Front=NULL, MapBrushT** Back=NULL) const;

    /// Subtracts from this brush A the given volume B, Result = A \ B.
    /// @param B        The volume that is to be subtracted ("carved") from this brush A, defined as another MapBrushT.
    /// @param Result   The array in which the subtraction result is returned.
    /// @returns true when this brush A and the carver B intersected and thus the subtraction result is actually different from this brush,
    ///     and false otherwise (A and B didn't overlap / intersect at all). In the latter case, the caller should delete all brushes that
    ///     the method might have put into the Result array during the course of its computations.
    bool Subtract(const MapBrushT* B, ArrayT<MapBrushT*>& Result) const;

    /// Returns the faces of this brush.
    const ArrayT<MapFaceT>& GetFaces() const { return m_Faces; }
    ArrayT<MapFaceT>& GetFaces() { return m_Faces; }

    /// Returns whether this brush is valid. TODO: ctors should throw exceptions instead!
    bool IsValid() const { return m_Faces.Size()>=4; }

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    /// The default constructor for instantiating a map brush.
    /// It is in the "private" section so that only "named constructors" can access it.
    MapBrushT();

    /// A helper method for our constructors.
    /// Called when all faces of this brush are defined by their planes, this method
    /// computes their actual intersection and thus the vertices for each face.
    void CompleteFaceVertices();


    ArrayT<MapFaceT> m_Faces;   ///< The set of faces whose intersection forms this brush.
};

#endif
