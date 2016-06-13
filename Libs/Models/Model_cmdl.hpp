/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODEL_HPP_INCLUDED
#define CAFU_MODEL_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix.hpp"
#include "AnimPose.hpp"     // Only for TEMPORARILY implementing the "old" ModelT interface methods.

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


class AnimImporterT;
class MaterialT;
class ModelLoaderT;
namespace MatSys { class RenderMaterialT; }
namespace ModelEditor { class CommandAddT; }
namespace ModelEditor { class CommandDeleteT; }
namespace ModelEditor { class CommandRenameT; }
namespace ModelEditor { class CommandSetAnimFPST; }
namespace ModelEditor { class CommandSetAnimNextT; }
namespace ModelEditor { class CommandSetMaterialT; }
namespace ModelEditor { class CommandSetMeshTSMethodT; }
namespace ModelEditor { class CommandSetMeshShadowsT; }
namespace ModelEditor { class CommandTransformJointT; }
namespace ModelEditor { class CommandUpdateAnimT; }
namespace ModelEditor { class CommandUpdateChannelT; }
namespace ModelEditor { class CommandUpdateGuiFixtureT; }
namespace ModelEditor { class CommandUpdateTriangleT; }
namespace ModelEditor { class CommandUpdateUVCoordsT; }


/// This class represents a native Cafu model.
class CafuModelT
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


    /// This struct defines a triangle mesh in the model.
    /// A mesh essentially consists of a list of triangles whose vertices are composed
    /// of weighted positions that are attached to the joints of the model.
    struct MeshT
    {
        /// The methods that can be used to generate the tangent-space axes at the vertices of a mesh.
        enum TangentSpaceMethodT
        {
            HARD,       ///< Hard edges, no smoothing: The tangent-space of the triangle is used as the tangent-space of its vertices. (Any smoothing groups info, if available, is ignored.)
            GLOBAL,     ///< Considers all triangles in the mesh to be in the same common smoothing group, and smoothes them globally. (Any smoothing groups info, if available, is ignored.) This method is equivalent to SM_GROUPS when all triangles are in the same smoothing group. It is also the default method, as it requires no smoothing groups info at all, provides better performance than SM_GROUPS, and was implemented in earlier versions of Cafu.
            SM_GROUPS   ///< [NOT YET IMPLEMENTED! At this time, this is the same as GLOBAL.] Like GLOBAL, but takes the given smoothing groups into account.
        };

        /// A single triangle.
        struct TriangleT
        {
            TriangleT(unsigned int v0=0, unsigned int v1=0, unsigned int v2=0);

            unsigned int VertexIdx[3];  ///< The indices to the three vertices that define this triangle.
            uint32_t     SmoothGroups;  ///< The smoothing groups that this triangle is in: If bit \c i is set, the triangle is in smoothing group \c i.

            int          NeighbIdx[3];  ///< The array indices of the three neighbouring triangles at the edges 01, 12 and 20. -1 indicates no neighbour, -2 indicates more than one neighbour.
            bool         Polarity;      ///< True if this triangle has positive polarity (texture is not mirrored), or false if it has negative polarity (texture is mirrored, SxT points inward).
            bool         SkipDraw;      ///< True if this triangle should be skipped when drawing the mesh (but not for casting stencil shadows and not for collision detection). This is useful for hiding triangles that are in the same plane as GUI panels and would otherwise cause z-fighting.
        };

        /// A single vertex.
        struct VertexT
        {
            float                u;             ///< Texture coordinate u.
            float                v;             ///< Texture coordinate v.
            unsigned int         FirstWeightIdx;///< The index of the first weight in the Weights array.
            unsigned int         NumWeights;    ///< The number of weights that form this vertex.

            bool                 Polarity;      ///< True if this vertex belongs to triangles with positive polarity, false if it belongs to triangles with negative polarity. Note that a single vertex cannot belong to triangles of both positive and negative polarity (but a GeoDup of this vertex can belong to the other polarity).
            ArrayT<unsigned int> GeoDups;       ///< This array contains the indices of vertices that are geometrical duplicates of this vertex, see AreVerticesGeoDups() for more information. The indices are stored in increasing order, and do *not* include the index of "this" vertex. Note that from the presence of GeoDups in a cmdl/md5 file we can *not* conclude that a break in the smoothing was intended by the modeller. Cylindrically wrapping seams are one counter-example.
        };

        /// A weight is a fixed position in the coordinate system of a joint.
        struct WeightT
        {
            unsigned int JointIdx;
            float        Weight;
            Vector3fT    Pos;
        };


        /// The default constructor.
        MeshT() : Material(NULL), RenderMaterial(NULL), TSMethod(GLOBAL), CastShadows(true) {}

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

        /// Returns a string representation of the TSMethod enum member.
        std::string GetTSMethod() const;

        /// Sets the TSMethod enum member by string.
        /// The given string should be one of the strings returned by GetTSMethod().
        /// If the string does not match a known method, TSMethod is set to GLOBAL.
        /// @param m   The method to set TSMethod to.
        void SetTSMethod(const std::string& m);

        std::string              Name;            ///< Name of this mesh.
        MaterialT*               Material;        ///< The material of this mesh.
        MatSys::RenderMaterialT* RenderMaterial;  ///< The render material used to render this mesh.
        TangentSpaceMethodT      TSMethod;        ///< How to generate the tangent-space axes at the vertices of this mesh? For meshes with normal-maps, the method should match the one that was used in the program that created the normal-maps.
        bool                     CastShadows;     ///< Should this mesh cast shadows?
        ArrayT<TriangleT>        Triangles;       ///< List of triangles this mesh consists of.
        ArrayT<VertexT>          Vertices;        ///< List of vertices this mesh consists of.
        ArrayT<WeightT>          Weights;         ///< List of weights that are attached to the skeleton (hierarchy of bones/joints).
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

        /// Returns whether the first frame in the sequence is equal to the last.
        /// The editor may use this to ask the user if he wishes to delete the last frame.
        bool IsLastFrameDup() const;

        std::string        Name;            ///< Name of this animation sequence.
        float              FPS;             ///< Playback rate for this animation sequence.
        int                Next;            ///< The sequence that should play after this. Use "this" for looping sequences, -1 for none.
     // ...                Events;          ///< E.g. "call a script function at frame 3".
        ArrayT<AnimJointT> AnimJoints;      ///< AnimJoints.Size() == m_Joints.Size()
        ArrayT<FrameT>     Frames;          ///< List of keyframes this animation consists of.
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


    /// The constructor. Creates a new Cafu model from a file as directed by the given model loader.
    /// @param Loader   The model loader that actually imports the file and fills in the model data.
    CafuModelT(ModelLoaderT& Loader);

    /// The destructor.
    ~CafuModelT();

    /// Imports animations into this model using the given AnimImporterT.
    /// At this time, this method is actually unused, because in the Model Editor,
    /// the related command is a friend and thus accesses the m_Anims member directly.
    void Import(AnimImporterT& Importer);

    /// Saves the model into the given stream.
    void Save(std::ostream& OutStream) const;

    // Inspector methods.
    const std::string&          GetFileName() const { return m_FileName; }
    const MaterialManagerImplT& GetMaterialManager() const { return m_MaterialMan; }
    const ArrayT<JointT>&       GetJoints() const { return m_Joints; }
    const ArrayT<MeshT>&        GetMeshes() const { return m_Meshes; }
    const ArrayT<SkinT>&        GetSkins() const { return m_Skins; }
    const ArrayT<GuiFixtureT>&  GetGuiFixtures() const { return m_GuiFixtures; }
    const ArrayT<AnimT>&        GetAnims() const { return m_Anims; }
    const ArrayT<ChannelT>&     GetChannels() const { return m_Channels; }
    const CafuModelT*           GetDlodModel() const { return m_DlodModel; }
    float                       GetDlodDist() const { return m_DlodDist; }

    /// Returns the proper material for the given mesh in the given skin.
    const MaterialT* GetMaterial(unsigned long MeshNr, int SkinNr) const;

    /// Returns the proper render material for the given mesh in the given skin.
    MatSys::RenderMaterialT* GetRenderMaterial(unsigned long MeshNr, int SkinNr) const;

    /// Determines if <tt>GF.Points[PointNr].MeshNr</tt> is a valid index into this model.
    bool IsMeshNrOK(const GuiFixtureT& GF, unsigned int PointNr) const;

    /// Determines if <tt>GF.Points[PointNr].VertexNr</tt> is a valid index into this model.
    bool IsVertexNrOK(const GuiFixtureT& GF, unsigned int PointNr) const;

    /// This method returns the pool of anim expressions for this model.
    AnimExprPoolT& GetAnimExprPool() const { return m_AnimExprPool; }

    /// This method is strictly for backwards-compatibility only, do not use in new code!
    AnimPoseT* GetSharedPose(IntrusivePtrT<AnimExpressionT> AE) const;

    /// Prints some model data to stdout, used for debugging.
    void Print() const;

    /// The current version of the \c cmdl file format.
    static const unsigned int CMDL_FILE_VERSION;


    private:

    friend class ModelEditor::CommandAddT;
    friend class ModelEditor::CommandDeleteT;
    friend class ModelEditor::CommandRenameT;
    friend class ModelEditor::CommandSetAnimFPST;
    friend class ModelEditor::CommandSetAnimNextT;
    friend class ModelEditor::CommandSetMaterialT;
    friend class ModelEditor::CommandSetMeshTSMethodT;
    friend class ModelEditor::CommandSetMeshShadowsT;
    friend class ModelEditor::CommandTransformJointT;
    friend class ModelEditor::CommandUpdateAnimT;
    friend class ModelEditor::CommandUpdateChannelT;
    friend class ModelEditor::CommandUpdateGuiFixtureT;
    friend class ModelEditor::CommandUpdateTriangleT;
    friend class ModelEditor::CommandUpdateUVCoordsT;

    CafuModelT(const CafuModelT&);          ///< Use of the Copy    Constructor is not allowed.
    void operator = (const CafuModelT&);    ///< Use of the Assignment Operator is not allowed.

    void InitMeshes();                      ///< An auxiliary method for the constructors.

    const std::string     m_FileName;       ///< File name of this model.   TODO: Remove!?!
    MaterialManagerImplT  m_MaterialMan;    ///< The material manager for the materials that are used with the meshes of this model.
    ArrayT<JointT>        m_Joints;         ///< Array of joints of this model.
    ArrayT<MeshT>         m_Meshes;         ///< Array of (sub)meshes of this model.
    ArrayT<SkinT>         m_Skins;          ///< Array of additional/alternative skins for this model.
    ArrayT<GuiFixtureT>   m_GuiFixtures;    ///< Array of GUI fixtures in the model.
    ArrayT<AnimT>         m_Anims;          ///< Array of animations of this model.
    ArrayT<ChannelT>      m_Channels;       ///< Array of channels in this model.

    CafuModelT*           m_DlodModel;      ///< Use the m_DlodModel instead of this when the camera is more than m_DlodDist away.
    float                 m_DlodDist;       ///< The distance beyond which the m_DlodModel is used instead of this.

    mutable AnimExprPoolT m_AnimExprPool;   ///< The pool of anim expressions for this model.
    mutable AnimPoseT*    m_TEMP_Pose;      ///< TEMPORARY!
};

#endif
