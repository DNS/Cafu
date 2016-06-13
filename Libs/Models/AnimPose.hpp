/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODEL_ANIM_POSE_HPP_INCLUDED
#define CAFU_MODEL_ANIM_POSE_HPP_INCLUDED

#include "AnimExpr.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix.hpp"


class CafuModelT;
class MaterialT;


/// This class describes a specific pose of an associated model.
///
/// A pose is defined (or "configured") by an AnimExpressionT instance
/// whose application to the model yields all data that is relevant for further actions
/// on the model in that pose, such as rendering, tracing rays, collision detection, etc.
///
/// The data that is derived from the anim expression is cached within the pose instance.
/// It comprises:
///   - the transformation matrices for each joint in the skeleton,
///   - the MatSys render meshes for each mesh,
///   - tangent space vectors for each vertex in each mesh.
/// In essence, this "externalizes" all data that is specific to a pose from
/// the representation of a model (the \c CafuModelT instance).
/// As a consequence, AnimPoseT instances should be shared whenever there are multiple users
/// (such as "static detail model" entity instances) that all show the same model in the same pose.
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

    /// This class describes the result of tracing a ray or a bounding box against the model.
    struct TraceResultT
    {
        /// The constructor.
        TraceResultT(float Fraction_=0.0f) : Fraction(Fraction_), Material(NULL), MeshNr(-1), TriNr(-1) { }

        float            Fraction;  ///< The scalar along RayDir at which the hit occurred (RayOrigin + RayDir*Fraction).
        Vector3fT        Normal;    ///< This is the normal vector of the hit surface.
        const MaterialT* Material;  ///< The material at the point of impact. Can be NULL, e.g. when an edge (i.e. a bevel plane) was hit or the material is not available.
        unsigned int     MeshNr;    ///< The number of the hit mesh. Can be -1 (that is, \emph{larger} then the number of meshes in the model) if the hit mesh cannot be determined.
        unsigned int     TriNr;     ///< The number of the hit triangle in the hit mesh. Can be -1 (that is, \emph{larger} then the number of triangles in the mesh) if the hit triangle cannot be determined.
    };


    /// The constructor.
    AnimPoseT(const CafuModelT& Model, IntrusivePtrT<AnimExpressionT> AnimExpr);

    /// The destructor.
    ~AnimPoseT();

    /// Returns the current anim expression of this pose.
    IntrusivePtrT<AnimExpressionT> GetAnimExpr() const { return m_AnimExpr; }

    /// Sets a new anim expression to use for this pose.
    ///
    /// \param AnimExpr   The new anim expression to use for this pose.
    void SetAnimExpr(IntrusivePtrT<AnimExpressionT> AnimExpr);

    /// This method assigns a pose of a parent or "super" model that should be used when rendering this model.
    ///
    /// For example, a player model can act as the super model for a weapon,
    /// so that the skeleton of the weapon is copied from the player model
    /// in order to align the weapon with the hands of the player.
    ///
    /// @param SuperPose   The super model pose that should be used when rendering this model.
    void SetSuperPose(const AnimPoseT* SuperPose);

    /// Call this if something in the related model has changed.
    void SetNeedsRecache() { m_CachedAE=NULL; }

    /// This method renders the model in this pose.
    /// The current MatSys model-view matrix determines the position and orientation.
    ///
    /// @param SkinNr     The skin to render the model with, -1 for the default skin.
    /// @param LodDist    The distance to the camera for reducing the level-of-detail.
    void Draw(int SkinNr, float LodDist) const;

    /// Returns the origin, x-axis and y-axis vectors for the given Gui fixture.
    /// The return value indicates whether the vectors could successfully be returned in the reference parameters
    /// (the method can fail if e.g. the model has no such Gui fixture or the Gui fixture is improperly specified).
    bool GetGuiPlane(unsigned int GFNr, Vector3fT& Origin, Vector3fT& AxisX, Vector3fT& AxisY) const;

    /// Traces a ray against this model in this pose, and returns whether it was hit.
    /// The ray for the trace is defined by RayOrigin + RayDir*Fraction, where Fraction is a scalar >= 0.
    ///
    /// @param SkinNr      The skin to use for the trace, use -1 for the default skin.
    /// @param RayOrigin   The point in model space where the ray starts.
    /// @param RayDir      A unit vector in model space that describes the direction the ray extends to.
    /// @param Result      If the model was hit, this struct contains additional details of the hit.
    ///
    /// @returns true if the ray hit the model, false otherwise. When the model was hit, additional details are returned via the Result parameter.
    bool TraceRay(int SkinNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, TraceResultT& Result) const;

    /// Considers the given triangle in the given mesh, and returns the vertex that the given point P is closest to in this pose.
    /// The returned number is the index of the vertex in the mesh, \emph{not} the (0, 1 or 2) index in the triangle.
    unsigned int FindClosestVertex(unsigned int MeshNr, unsigned int TriNr, const Vector3fT& P) const;

    /// Returns the set of transformation matrices (one per joint) at the given sequence and frame number.
    const ArrayT<MatrixT>& GetJointMatrices() const;

    /// Returns the transformation matrix for the joint with the given name, or \c NULL if there is no such joint.
    const MatrixT* GetJointMatrix(const std::string& JointName) const;

    /// Returns the mesh infos with additional data for each mesh in this pose.
    const ArrayT<MeshInfoT>& GetMeshInfos() const;

    /// Returns the MatSys meshes for the model in this pose.
    const ArrayT<MatSys::MeshT>& GetDrawMeshes() const;

    /// Returns the bounding-box for the model in this pose.
    const BoundingBox3fT& GetBB() const;


    private:

    AnimPoseT(const AnimPoseT&);                    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const AnimPoseT&);             ///< Use of the Assignment Operator is not allowed.

    void SyncDimensions() const;
    void UpdateJointMatrices() const;
    void UpdateVertexPositions() const;
    void UpdateTangentSpaceHard(unsigned long MeshNr) const;
    void UpdateTangentSpaceGlobal(unsigned long MeshNr) const;
    void UpdateTangentSpaceSmGroups(unsigned long MeshNr) const;
    void Recache() const;

    const CafuModelT&             m_Model;          ///< The related model that this is a pose for.
    AnimExpressionPtrT            m_AnimExpr;       ///< The expression that describes the skeleton pose for which we have computed the cache data.
    const AnimPoseT*              m_SuperPose;
    AnimPoseT*                    m_DlodPose;       ///< The next pose in the chain of dlod poses matching the chain of dlod models.

    mutable AnimExpressionPtrT    m_CachedAE;       ///< Used to detect if the m_AnimExpr has changed, so that our matrices, meshes etc. can be recached.
    mutable ArrayT<MatrixT>       m_JointMatrices;  ///< The transformation matrices that represent the pose of the skeleton at the given animation sequence and frame number.
    mutable ArrayT<MeshInfoT>     m_MeshInfos;      ///< Additional data for each mesh in m_Model.
    mutable ArrayT<MatSys::MeshT> m_Draw_Meshes;    ///< The draw meshes resulting from the m_JointMatrices.
    mutable BoundingBox3fT        m_BoundingBox;    ///< The bounding-box for the model in this pose.
};

#endif
