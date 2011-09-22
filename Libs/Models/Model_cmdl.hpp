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

#ifndef _CAFU_MODEL_HPP_
#define _CAFU_MODEL_HPP_

#include "Templates/Array.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix.hpp"
#include "Model.hpp"

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


class MaterialT;
class ModelLoaderT;
namespace MatSys { class RenderMaterialT; }
namespace ModelEditor { class CommandAddT; }
namespace ModelEditor { class CommandDeleteT; }
namespace ModelEditor { class CommandRenameT; }
namespace ModelEditor { class CommandSetAnimFPST; }
namespace ModelEditor { class CommandSetAnimNextT; }
namespace ModelEditor { class CommandSetMaterialT; }
namespace ModelEditor { class CommandTransformJointT; }
namespace ModelEditor { class CommandUpdateAnimT; }
namespace ModelEditor { class CommandUpdateGuiFixtureT; }


/// This class represents a native Cafu model.
class CafuModelT : public ModelT
{
    public:

    /// This struct represents a joint in the skeleton / a node in the hierarchy of the model.
    struct JointT
    {
        std::string Name;       ///< The name of the joint.
        int         Parent;     ///< The parent of the root joint is -1.
        Vector3fT   Pos;        ///< The position of the origin of the joint (relative, in the coordinate system of the parent joint).
        Vector3fT   Qtr;        ///< The orientation of the coordinate axes of the joint.
        Vector3fT   Scale;      ///< The scale  of the coordinate axes of the joint.
    };


    /// A mesh consisting of a material and a list of vertices.
    struct MeshT
    {
        /// A single triangle.
        struct TriangleT
        {
            TriangleT(unsigned int v0=0, unsigned int v1=0, unsigned int v2=0);

            unsigned int VertexIdx[3];  ///< The indices to the three vertices that define this triangle.

            int          NeighbIdx[3];  ///< The array indices of the three neighbouring triangles at the edges 01, 12 and 20. -1 indicates no neighbour, -2 indicates more than one neighbour.
            bool         Polarity;      ///< True if this triangle has positive polarity (texture is not mirrored), or false if it has negative polarity (texture is mirrored, SxT points inward).

            Vector3fT    Draw_Normal;   ///< The draw normal for this triangle, required for the shadow-silhouette determination.
        };

        /// A single vertex.
        struct VertexT
        {
            float                u;             ///< Texture coordinate u.
            float                v;             ///< Texture coordinate v.
            unsigned int         FirstWeightIdx;
            unsigned int         NumWeights;

            bool                 Polarity;      ///< True if this vertex belongs to triangles with positive polarity, false if it belongs to triangles with negative polarity. Note that a single vertex cannot belong to triangles of both positive and negative polarity (but a GeoDup of this vertex can belong to the other polarity).
            ArrayT<unsigned int> GeoDups;       ///< This array contains the indices of vertices that are geometrical duplicates of this vertex, see AreVerticesGeoDups() for more information. The indices are stored in increasing order, and do *not* include the index of "this" vertex. Note that from the presence of GeoDups in a cmdl/md5 file we can *not* conclude that a break in the smoothing was intended by the modeller. Cylindrically wrapping seams are one counter-example.

            Vector3fT            Draw_Pos;      ///< Position of this vertex.
            Vector3fT            Draw_Normal;   ///< Vertex normal.
            Vector3fT            Draw_Tangent;  ///< Vertex tangent.
            Vector3fT            Draw_BiNormal; ///< Vertex binormal.
        };

        /// A weight is a fixed position in the coordinate system of a joint.
        struct WeightT
        {
            unsigned int JointIdx;
            float        Weight;
            Vector3fT    Pos;
        };


        /// The default constructor.
        MeshT() : Material(NULL), RenderMaterial(NULL) {}

        /// Determines whether the two vertices with array indices Vertex1Nr and Vertex2Nr are geometrical duplicates of each other.
        /// Two distinct vertices are geometrical duplicates of each other if
        /// (a) they have the same NumWeights and the same FirstWeightIdx, or
        /// (b) they have the same NumWeights and the referred weights have the same contents.
        /// The two vertices may in general have different uv-coordinates, which are not considered for the geodup property.
        /// Note that if Vertex1Nr==Vertex2Nr, true is returned (case (a) above).
        /// @param Vertex1Nr Array index of first vertex.
        /// @param Vertex2Nr Array index of second vertex.
        /// @return Whether the vertices are geometrical duplicates of each other.
        bool AreGeoDups(unsigned int Vertex1Nr, unsigned int Vertex2Nr) const;

        std::string              Name;           ///< Name of this mesh.
        MaterialT*               Material;       ///< The material of this mesh.
        MatSys::RenderMaterialT* RenderMaterial; ///< The render material used to render this mesh.
        ArrayT<TriangleT>        Triangles;      ///< List of triangles this mesh consists of.
        ArrayT<VertexT>          Vertices;       ///< List of vertices this mesh consists of.
        ArrayT<WeightT>          Weights;        ///< List of weights that are attached to the skeleton (hierarchy of bones/joints).
    };


    /// This struct describes one animation sequence, e.g.\ "run", "walk", "jump", etc.
    /// We use it to obtain an array of joints (ArrayT<JointT>, just like m_Joints) for any point (frame number) in the animation sequence.
    struct AnimT
    {
        struct AnimJointT
        {
         // std::string  Name;              ///< Checked to be identical with the name  of the base mesh at load time.
         // int          Parent;            ///< Checked to be identical with the value of the base mesh at load time.
            Vector3fT    DefaultPos;        ///< The default position of the origin of the joint (relative, in the coordinate system of the parent joint). Used for all frames in this anim sequence unless bits 0 to 2 in @c Flags indicate that we have individiual values for each frame.
            Vector3fT    DefaultQtr;        ///< The default orientation of the coordinate axes of the joint. Used for all frames in this anim sequence unless bits 3 to 5 in @c Flags indicate that we have individiual values for each frame.
            Vector3fT    DefaultScale;      ///< The default scale  of the coordinate axes of the joint. Used for all frames in this anim sequence unless bits 6 to 8 in @c Flags indicate that we have individiual values for each frame.
            unsigned int Flags;             ///< If the i-th bit in Flags is \emph{not} set, DefaultPos[i], DefaultQtr[i-3] and DefaultScale[i-6] are used for all frames in this anim sequence. Otherwise, for each set bit, we find frame-specific values in AnimData[FirstDataIdx+...].
            unsigned int FirstDataIdx;      ///< If f is the current frame, this is the index into Frames[f].AnimData[...].
         // int          NumDatas;          ///< There are so many data values as there are bits set in Flags.
        };

        /// A keyframe in the animation.
        struct FrameT
        {
            BoundingBox3fT BB;              ///< The bounding box of the model in this frame.
            ArrayT<float>  AnimData;
        };


        /// Recomputes the bounding-box for the specified frame in this animation sequence.
        /// @param FrameNr   The number of the frame to recompute the bounding-box for.
        /// @param Joints    The joints of the related model.
        /// @param Meshes    The meshes of the related model.
        void RecomputeBB(unsigned int FrameNr, const ArrayT<JointT>& Joints, const ArrayT<MeshT>& Meshes);


        std::string        Name;            ///< Name of this animation sequence.
        float              FPS;             ///< Playback rate for this animation sequence.
        int                Next;            ///< The sequence that should play after this. Use "this" for looping sequences, "none" for none.
     // ...                Events;          ///< E.g. "call a script function at frame 3".
        ArrayT<AnimJointT> AnimJoints;      ///< AnimJoints.Size() == m_Joints.Size()
        ArrayT<FrameT>     Frames;          ///< List of keyframes this animation consists of.
    };


    /// This struct describes additional/alternative skins for the meshes of this model.
    struct SkinT
    {
        std::string                      Name;              ///< The name of this skin.
        ArrayT<MaterialT*>               Materials;         ///< For each mesh \c m, <tt>Materials[m]</tt> is the material for the mesh in this skin. If <tt>Materials[m]</tt> is NULL, the material of the default skin is used.
        ArrayT<MatSys::RenderMaterialT*> RenderMaterials;   ///< Analogous to \c Materials, these are the (possibly NULL) render materials for the meshes in this skin.
    };


    /// This struct defines how and where a GUI can be fixed to the model.
    /// The GUI rectangle is defined by three points: the origin, the x-axis endpoint, and the y-axis endpoint, numbered 0, 1 and 2.
    /// Each point is represented by an arbitrary vertex of one of meshes in the model.
    /// The whole GUI rectangle can be translated and scaled, in order to compensate for cases where the mesh vertices do not
    /// exactly match the desired rectangle dimensions.
    /// More than one GUI can be fixed to a model, and if the referenced mesh vertices are animated, the GUI rectangle is animated, too.
    struct GuiFixtureT
    {
        GuiFixtureT();

        struct PointT
        {
            unsigned int MeshNr;
            unsigned int VertexNr;
        };

        std::string Name;
        PointT      Points[3];
        float       Trans[2];
        float       Scale[2];
    };


    /// This structure is used to describe the locations where GUIs can be attached to the model.
    /// Note that the current static/fixed-position implementation (origin, x- and y-axis) is temporary though,
    /// it should eventually be possible to attach GUIs even to animated models.
    struct GuiLocT
    {
        Vector3fT Origin;
        Vector3fT AxisX;
        Vector3fT AxisY;
    };


    /// Channels allow animations to play only on a subset of the joints,
    /// so that multiple animations can play on different parts of the model at the same time.
    /// For example, you can play a walking animation on the legs, an animation for swinging
    /// the arms on the upper body, and an animation for moving the eyes on the head. 
    ///
    /// Technically, a channel defines a group of joints. It is used to limit or "filter"
    /// animations such that they affect only the joints that are members of the channel.
    struct ChannelT
    {
        std::string Name;   ///< The name of this channel.

        bool IsMember(unsigned int JointNr) const;
        void SetMember(unsigned int JointNr, bool Member=true);


        private:

        ArrayT<uint32_t> m_BitBlocks;
    };


    /// This struct describes information about a parent or "super" model whose skeleton pose should be used when rendering this model.
    /// For example, a player model can act as the super model for a weapon, so that the skeleton of the weapon is copied from the
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


    /// The constructor. Creates a new Cafu model from a file as directed by the given model loader.
    /// @param Loader   The model loader that actually imports the file and fills in the model data.
    CafuModelT(ModelLoaderT& Loader);

    /// The destructor.
    ~CafuModelT();

    /// Saves the model into the given stream.
    void Save(std::ostream& OutStream) const;

    // Inspector methods.
    bool GetUseGivenTS() const { return m_UseGivenTangentSpace; }
    const MaterialManagerImplT& GetMaterialManager() const { return m_MaterialMan; }
    const ArrayT<JointT>&       GetJoints() const { return m_Joints; }
    const ArrayT<MeshT>&        GetMeshes() const { return m_Meshes; }
    const ArrayT<AnimT>&        GetAnims() const { return m_Anims; }
    const ArrayT<SkinT>&        GetSkins() const { return m_Skins; }
    const ArrayT<GuiFixtureT>&  GetGuiFixtures() const { return m_GuiFixtures; }
    const ArrayT<ChannelT>&     GetChannels() const { return m_Channels; }

    /// This method returns the set of drawing matrices (one per joint) at the given sequence and frame number.
    const ArrayT<MatrixT>& GetDrawJointMatrices(int SequenceNr, float FrameNr, const SuperT* Super=NULL) const;

    /// The new method to draw the model.
    /// @param SequenceNr   The number of the animation sequence to use, -1 for the bind pose.
    /// @param FrameNr      The frame number in the animation sequence to render to model at.
    /// @param SkinNr       The skin to render the model with, -1 for the default skin.
    /// @param LodDist      The distance to the camera for reducing the level-of-detail (currently unused).
    /// @param Super        Information about a parent or "super" model whose skeleton pose should be used when rendering this model.
    void Draw(int SequenceNr, float FrameNr, int SkinNr, float LodDist, const SuperT* Super=NULL) const;

    /// Determines if <tt>GF.Points[PointNr].MeshNr</tt> is a valid index into this model.
    bool IsMeshNrOK(const GuiFixtureT& GF, unsigned int PointNr) const;

    /// Determines if <tt>GF.Points[PointNr].VertexNr</tt> is a valid index into this model.
    bool IsVertexNrOK(const GuiFixtureT& GF, unsigned int PointNr) const;

    /// Returns the proper material for the given mesh in the given skin.
    const MaterialT* GetMaterial(unsigned long MeshNr, int SkinNr) const;

    // The ModelT interface.
    const std::string& GetFileName() const;     // TODO: Remove!?!
    void               Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel=NULL) const;
    bool               GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;
    void               Print() const;
    unsigned int       GetNrOfSequences() const;
    BoundingBox3fT     GetBB(int SequenceNr, float FrameNr) const;
    bool               TraceRay(int SequenceNr, float FrameNr, int SkinNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, TraceResultT& Result) const;
 // float              GetNrOfFrames(int SequenceNr) const;
    float              AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop=true) const;

    static const unsigned int CMDL_FILE_VERSION;    ///< The current version of the \c cmdl file format.


    private:

    friend class ModelEditor::CommandAddT;
    friend class ModelEditor::CommandDeleteT;
    friend class ModelEditor::CommandRenameT;
    friend class ModelEditor::CommandSetAnimFPST;
    friend class ModelEditor::CommandSetAnimNextT;
    friend class ModelEditor::CommandSetMaterialT;
    friend class ModelEditor::CommandTransformJointT;
    friend class ModelEditor::CommandUpdateAnimT;
    friend class ModelEditor::CommandUpdateGuiFixtureT;

    void RecomputeBindPoseBB();                                                             ///< Recomputes the bounding box for the model in bind pose (stored in m_BindPoseBB).
    void InitMeshes();                                                                      ///< An auxiliary method for the constructors.
    void UpdateCachedDrawData(int SequenceNr, float FrameNr, const SuperT* Super) const;    ///< A private auxiliary method.
    MatSys::RenderMaterialT* GetRenderMaterial(unsigned long MeshNr, int SkinNr) const;     ///< Returns the proper render material for the given mesh in the given skin.


    const std::string     m_FileName;               ///< File name of this model.   TODO: Remove!?!
    MaterialManagerImplT  m_MaterialMan;            ///< The material manager for the materials that are used with the meshes of this model.
    ArrayT<JointT>        m_Joints;                 ///< Array of joints of this model.
    mutable ArrayT<MeshT> m_Meshes;                 ///< Array of (sub)meshes of this model.
    ArrayT<AnimT>         m_Anims;                  ///< Array of animations of this model.
    ArrayT<SkinT>         m_Skins;                  ///< Array of additional/alternative skins for this model.

    const bool            m_UseGivenTangentSpace;   ///< Whether this model should use the fixed, given tangent space that was loaded from the model file, or it the tangent space is dynamically recomputed (useful for animated models).
 // const bool            m_CastShadows;            ///< Should this model cast shadows?
    BoundingBox3fT        m_BindPoseBB;             ///< The bounding-box for the base pose of the model.
    ArrayT<GuiFixtureT>   m_GuiFixtures;            ///< Array of GUI fixtures in the model.
    ArrayT<GuiLocT>       m_GuiLocs;                ///< Array of locations where GUIs can be attached to this model.
    ArrayT<ChannelT>      m_Channels;               ///< Array of channels in this model.


    // Members for caching the data that is required for drawing the model at a given animation sequence and frame.
    mutable int                   m_Draw_CachedDataAtSequNr;    ///< The animation sequence number at which we have computed the cache data.
    mutable float                 m_Draw_CachedDataAtFrameNr;   ///< The animation frame    number at which we have computed the cache data.

    mutable ArrayT<MatrixT>       m_Draw_JointMatrices;         ///< The transformation matrices that represent the pose of the skeleton at the given animation sequence and frame number.
    mutable ArrayT<MatSys::MeshT> m_Draw_Meshes;                ///< The draw meshes resulting from m_Draw_JointMatrices.
};

#endif
