/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MORPH_PRIMITIVE_HPP_INCLUDED
#define CAFU_MORPH_PRIMITIVE_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"


class MapBezierPatchT;
class MapBrushT;
class MapPrimitiveT;
class Renderer2DT;
class Renderer3DT;
class wxPoint;
class MP_FaceT;


struct MP_PartT
{
    enum TypeT
    {
        TYPE_VERTEX,
        TYPE_EDGE
    };


    MP_PartT() : m_Selected(false)
    {
    }

    virtual ~MP_PartT() { }

    virtual TypeT     GetType() const=0;
    virtual Vector3fT GetPos()  const=0;

    bool m_Selected;   ///< Is this part/handle selected by the user?
};


class MP_VertexT : public MP_PartT
{
    public:

    TypeT     GetType() const { return TYPE_VERTEX; }
    Vector3fT GetPos()  const { return pos; }

    Vector3fT pos;
};


class MP_EdgeT : public MP_PartT
{
    public:

    MP_EdgeT();

    TypeT     GetType() const { return TYPE_EDGE; }
    Vector3fT GetPos()  const { return (Vertices[0]->pos + Vertices[1]->pos)*0.5f; }

    MP_VertexT* Vertices[2];    ///< The vertices of this edge (pointing into the MorphPrimT::m_Vertices array).
    MP_FaceT*   Faces[2];       ///< The faces this edge belongs to (pointing into the MorphPrimT::m_Faces array).
};


class MP_FaceT
{
    public:

    ArrayT<MP_EdgeT*> Edges;    ///< The edges of this face (pointing into the MorphPrimT::m_Edges array).
};


/// This is a helper class for the ToolMorphT ("edit vertices") tool.
/// It represents a map primitive (a brush or a bezier patch) that is currently being morphed by the tool.
///
/// This class is considered a \emph{helper} class, because the user code (i.e. the morph tool)
/// (currently) needs knowledge about the implementation of this class whenever it keeps pointers to
/// parts (vertices, edges) of the object that this class represents. Such pointers into the internal
/// structures may become invalidated by certain methods, and thus great care is required.
class MorphPrimT
{
    public:

    /// The constructor.
    /// @param MapPrim   The original brush or bezier patch that this MorphPrimT is associated with / attached to.
    /// Note that this MorphPrimT does not become the "owner" of the MapPrim pointer, e.g. it does not attempt to delete it in its dtor.
    /// That also means that this MorphPrimT should not live longer than the MapPrim object.
    MorphPrimT(const MapPrimitiveT* MapPrim);

    ~MorphPrimT();

    const MapPrimitiveT* GetMapPrim() const { return m_MapPrim; }
    bool                 IsModified() const { return m_Modified; }

    /// Returns a newly created instance matching the morphed map primitive, or NULL if reconstruction was not possible.
    /// It does not reset the modified-flag.
    MapPrimitiveT* GetMorphedMapPrim() const;

    /// Moves the selected handles by Delta.
    void MoveSelectedHandles(const Vector3fT& Delta);

    void Render(Renderer2DT& Renderer, bool RenderVertexHandles, bool RenderEdgeHandles) const;
    void Render(Renderer3DT& Renderer, bool RenderVertexHandles, bool RenderEdgeHandles) const;


    // Must store MP_VertexT*, not MP_VertexT, or else array growing renders all external pointers obsolete...
    ArrayT<MP_VertexT*> m_Vertices;
    ArrayT<MP_EdgeT*  > m_Edges;
    ArrayT<MP_FaceT*  > m_Faces;


    private:

    MorphPrimT(const MorphPrimT&);          ///< Use of the Copy Constructor    is not allowed.
    void operator = (const MorphPrimT&);    ///< Use of the Assignment Operator is not allowed.

    /// After a change of (or in) the m_Vertices array, this method computes the convex hull over them
    /// and updates (or rather, recomputes) all other member variables (the m_Edges and m_Faces).
    /// This method should only be called if the m_MapPrim member is of type MapBrushT,
    /// because it also modifies the m_Vertices array, which is not desired for the MapBezierPatchT type.
    void UpdateBrushFromVertices();
    void UpdatePatch();

    MP_VertexT* FindClosestVertex(const Vector3fT& Point) const;
    MP_EdgeT*   FindEdge(const MP_VertexT* v1, const MP_VertexT* v2) const;

    MP_VertexT* GetConnectionVertex(MP_EdgeT* Edge1, MP_EdgeT* Edge2) const;
    void RenderHandle(Renderer3DT& Renderer, const wxPoint& ClientPos, const float* color) const;


    const MapPrimitiveT* m_MapPrim;     ///< The "attached" source/reference map brush / bezier patch.
    bool                 m_Modified;    ///< Whether the MorphPrimT contains any modifications to the "attached" map brush/bezier patch.
    MapBezierPatchT*     m_RenderBP;    ///< If m_MapPrim is a Bezier patch, this is the current morphed clone that we use for rendering.
};

#endif
