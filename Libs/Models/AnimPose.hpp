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

    /// The instances of this struct parallel and augment the \c CafuModelT::MeshT instances in the related \c CafuModelT.
    struct MeshInfoT
    {
        struct TriangleT
        {
            Vector3fT Normal;       ///< The normal vector of this triangle, required for the shadow-silhouette determination.
        };

        struct VertexT
        {
            Vector3fT Pos;          ///< The spatial position of this vertex.
            Vector3fT Normal;       ///< The tangent-space normal   vector of this vertex.
            Vector3fT Tangent;      ///< The tangent-space tangent  vector of this vertex.
            Vector3fT BiNormal;     ///< The tangent-space binormal vector of this vertex.
        };

        ArrayT<TriangleT> Triangles;
        ArrayT<VertexT>   Vertices;
    };


    /// The constructor.
    AnimPoseT(const CafuModelT& Model, int SequNr=-1, float FrameNr=0.0f);

    /// The destructor.
    ~AnimPoseT();

    int GetSequNr() const { return m_SequNr; }

    /// @param SequNr   The number of the animation sequence to use, -1 for the bind pose.
    void SetSequNr(int SequNr);

    float GetFrameNr() const { return m_FrameNr; }

    /// @param FrameNr      The frame number in the animation sequence to render the model at.
    void SetFrameNr(float FrameNr);

    /// This method assigns a pose of a parent or "super" model that should be used when rendering this model.
    ///
    /// For example, a player model can act as the super model for a weapon,
    /// so that the skeleton of the weapon is copied from the player model
    /// in order to align the weapon with the hands of the player.
    ///
    /// @param SuperPose   The super model pose that should be used when rendering this model.
    void SetSuperPose(const AnimPoseT* SuperPose);

    /// Advances the pose in time.
    void Advance(float Time, bool ForceLoop=false);

    /// Call this if something in the related model has changed.
    void SetNeedsRecache() { m_NeedsRecache=true; }

    /// This method renders the model in this pose.
    /// @param SkinNr     The skin to render the model with, -1 for the default skin.
    /// @param LodDist    The distance to the camera for reducing the level-of-detail.
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

    /// Returns the set of transformation matrices (one per joint) at the given sequence and frame number.
    const ArrayT<MatrixT>& GetJointMatrices() const;

    /// Returns the transformation matrix for the joint with the given name, or \c NULL if there is no such joint.
    const MatrixT* GetJointMatrix(const std::string& JointName) const;

    /// Returns the mesh infos with additional data for each mesh in this pose.
    const ArrayT<MeshInfoT>& GetMeshInfos() const;

    /// Returns the MatSys meshes for this pose.
    const ArrayT<MatSys::MeshT>& GetDrawMeshes() const;

    /// Returns the bounding-box for this pose.
    const BoundingBox3fT& GetBB() const;


    private:

    AnimPoseT(const AnimPoseT&);                    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const AnimPoseT&);             ///< Use of the Assignment Operator is not allowed.

    void SyncDimensions() const;
    void UpdateData() const;
    void Recache() const;
    void NormalizeInput();

    const CafuModelT&             m_Model;          ///< The related model that this is a pose for.
    int                           m_SequNr;         ///< The animation sequence number at which we have computed the cache data.
    float                         m_FrameNr;        ///< The animation frame    number at which we have computed the cache data.
    const AnimPoseT*              m_SuperPose;
    AnimPoseT*                    m_DlodPose;       ///< The next pose in the chain of dlod poses matching the chain of dlod models.
 // ArrayT<...>                   m_Def;            ///< Array of { channel, sequence, framenr, (forceloop), blendweight } tuples.
 // bool                          m_DoCache;        ///< Cache the computed data? (Set to true by the user if he want to re-use this instance.)

    mutable bool                  m_NeedsRecache;   ///< wird auf 'true' gesetzt wann immer SetSequ(), SetFrameNr() oder AdvanceAll() o.ä. aufgerufen wird, übernimmt m_Draw_CachedDataAt*Nr Funktionalität.
    mutable ArrayT<MatrixT>       m_JointMatrices;  ///< The transformation matrices that represent the pose of the skeleton at the given animation sequence and frame number.
    mutable ArrayT<MeshInfoT>     m_MeshInfos;      ///< Additional data for each mesh in m_Model.
    mutable ArrayT<MatSys::MeshT> m_Draw_Meshes;    ///< The draw meshes resulting from the m_JointMatrices.
    mutable BoundingBox3fT        m_BoundingBox;    ///< The bounding-box for the model in this pose.
};

#endif
