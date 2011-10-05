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

#ifndef _CAFU_MODEL_ANIM_POSE_HPP_
#define _CAFU_MODEL_ANIM_POSE_HPP_

#include "Templates/Array.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix.hpp"
#include "Model.hpp"    // For ModelT::TraceResultT


class CafuModelT;
// class TraceResultT;


/// This class describes a specific pose of an associated model.
///
/// A pose is defined by a set of animation sequences at given frame numbers for given channels,
/// whose combined application to the model yields all data that is relevant for further actions
/// on the model in that pose, such as rendering, tracing rays, collision detection, etc.
///
/// The data that is derived from the set of input tuples (sequence, frame number, channel)
/// is cached within the pose instance. It comprises:
///   - the transformation matrices for each joint in the skeleton,
///   - the MatSys render meshes for each mesh,
///   - tangent space vectors for each vertex in each mesh.
/// In essence, this "externalizes" all data that is specific to a pose from
/// the representation of a model (the \c CafuModelT instance).
/// As a consequence, AnimPoseT instances should be shared whenever there are multiple users
/// (such as "static detail model" entity instances) that all show the same model in the same
/// pose.
class AnimPoseT
{
    public:

    /// This struct describes information about a parent or "super" model whose skeleton pose
    /// should be used when rendering this model. For example, a player model can act as the
    /// super model for a weapon, so that the skeleton of the weapon is copied from the
    /// player model in order to align the weapon with the hands of the player.
    struct SuperT
    {
        /// The constructor.
        /// @param Matrices_   The draw matrices of the super model.
        /// @param Map_        Describes how our joints map to the joints of the super model.
        ///                    If <tt>Map_[i]</tt> is not a valid index into \c Matrices_, then our joint \c i has no match in the skeleton of the super model.
        SuperT(const ArrayT<MatrixT>& Matrices_, const ArrayT<unsigned int>& Map_) : Matrices(Matrices_), Map(Map_) { }

        /// Has our joint \c JointNr a correspondence in the super model?
        bool HasMatrix(unsigned long JointNr) const { return Map[JointNr] < Matrices.Size(); }

        /// For our joint \c JointNr, return the corresponding matrix from the super model.
        /// Only call this if HasMatrix(JointNr) returns \c true.
        const MatrixT& GetMatrix(unsigned long JointNr) const { return Matrices[Map[JointNr]]; }

        const ArrayT<MatrixT>&      Matrices;   ///< The draw matrices of the super model.
        const ArrayT<unsigned int>& Map;        ///< Describes how our joints map to the joints of the super model. If <tt>Map[i]</tt> is not a valid index into \c Matrices, then our joint \c i has no match in the skeleton of the super model.
    };


    AnimPoseT(const CafuModelT& Model, int SequNr=-1, float FrameNr=0.0f);

    /// @param SequenceNr   The number of the animation sequence to use, -1 for the bind pose.
    void SetSequNr(int SequNr);

    float GetFrameNr() const { return m_FrameNr; }

    /// @param FrameNr      The frame number in the animation sequence to render to model at.
    void SetFrameNr(float FrameNr);

    /// @param Super        Information about a parent or "super" model whose skeleton pose should be used when rendering this model.
    void SetSuper(const SuperT* Super);

    /// Advances the pose in time.
    void Advance(float Time, bool ForceLoop=false);

    /// Call this if something in the related model has changed.
    void SetNeedsRecache() { m_NeedsRecache=true; }

    /// This method renders the model in this pose.
    /// @param SkinNr     The skin to render the model with, -1 for the default skin.
    /// @param LodDist    The distance to the camera for reducing the level-of-detail (currently unused).
    void Draw(int SkinNr, float LodDist) const;

    /// Traces a ray against this model in this pose, and returns whether it was hit.
    /// The ray for the trace is defined by RayOrigin + RayDir*Fraction, where Fraction is a scalar >= 0.
    ///
    /// @param SkinNr      The skin to use for the trace, use -1 for the default skin.
    /// @param RayOrigin   The point in model space where the ray starts.
    /// @param RayDir      A unit vector in model space that describes the direction the ray extends to.
    /// @param Result      If the model was hit, this struct contains additional details of the hit.
    ///
    /// @returns true if the ray hit the model, false otherwise. When the model was hit, additional details are returned via the Result parameter.
    bool TraceRay(int SkinNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, ModelT::TraceResultT& Result) const;

    /// Considers the given triangle in the given mesh, and returns the vertex that the given point P is closest to in this pose.
    /// The returned number is the index of the vertex in the mesh, \emph{not} the (0, 1 or 2) index in the triangle.
    unsigned int FindClosestVertex(unsigned int MeshNr, unsigned int TriNr, const Vector3fT& P) const;

    /// Returns the spatial position of the given vertex in the specified mesh.
    const Vector3fT& GetVertexPos(unsigned int MeshNr, unsigned int VertexNr) const;

    /// This method returns the set of transformation matrices (one per joint) at the given sequence and frame number.
    const ArrayT<MatrixT>& GetJointMatrices() const;

    /// This method returns the MatSys meshes for this pose.
    const ArrayT<MatSys::MeshT>& GetDrawMeshes() const;

    /// This method returns the bounding-box for this pose.
    const BoundingBox3fT& GetBB() const;


    private:

    /// The instances of this struct parallel and augment the \c CafuModelT::MeshT instances in the related \c CafuModelT.
    struct MeshT
    {
        struct TriangleT
        {
            Vector3fT Draw_Normal;      ///< The normal vector of this triangle, required for the shadow-silhouette determination.
        };

        struct VertexT
        {
            Vector3fT Draw_Pos;         ///< The spatial position of this vertex.
            Vector3fT Draw_Normal;      ///< The tangent-space normal   vector of this vertex.
            Vector3fT Draw_Tangent;     ///< The tangent-space tangent  vector of this vertex.
            Vector3fT Draw_BiNormal;    ///< The tangent-space binormal vector of this vertex.
        };

        ArrayT<TriangleT> Triangles;
        ArrayT<VertexT>   Vertices;
    };


    void NormalizeInput();
    void SyncDimensions() const;
    void UpdateData() const;
    void Recache() const;

    const CafuModelT&     m_Model;
    int                   m_SequNr;             ///< The animation sequence number at which we have computed the cache data.
    float                 m_FrameNr;            ///< The animation frame    number at which we have computed the cache data.
    const SuperT*         m_Super;
 // ArrayT<...>           m_Def;                ///< Array of { channel, sequence, framenr, (forceloop), blendweight } tuples.
 // bool                  m_DoCache;            ///< Cache the computed data? (Set to true by the user if he want to re-use this instance.)

    mutable bool                  m_NeedsRecache;       ///< wird auf 'true' gesetzt wann immer SetSequ(), SetFrameNr() oder AdvanceAll() o.ä. aufgerufen wird, übernimmt m_Draw_CachedDataAt*Nr Funktionalität.
    mutable ArrayT<MatrixT>       m_JointMatrices;      ///< The transformation matrices that represent the pose of the skeleton at the given animation sequence and frame number.
    mutable ArrayT<MeshT>         m_Meshes;
    mutable ArrayT<MatSys::MeshT> m_Draw_Meshes;        ///< The draw meshes resulting from m_JointMatrices.
    mutable BoundingBox3fT        m_BoundingBox;
};

#endif
