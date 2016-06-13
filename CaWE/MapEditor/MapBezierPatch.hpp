/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAP_BEZIER_PATCH_HPP_INCLUDED
#define CAFU_MAP_BEZIER_PATCH_HPP_INCLUDED

#include "MapPrimitive.hpp"
#include "SurfaceInfo.hpp"

#include "MaterialSystem/Mesh.hpp"
#include "SceneGraph/BezierPatchNode.hpp"
#include "SceneGraph/LightMapMan.hpp"


class EditorMaterialI;
class EditorMatManT;
namespace MatSys { class RenderMaterialT; }
namespace cf { namespace ClipSys { class CollisionModelStaticT; } }


/// This class represents a bezier patch.
///
/// Implementation notes:
///
/// Patches consist of several distinct parts: The control-vertices (with position and texture-coordinates each),
/// a mesh for rendering, and auxiliary data that allows the user to scale, shift and rotate the texture-coordinates in a GUI dialog.
/// The auxiliary data consists of a duplicate of the control-vertices (texture-coords only, no positions), used as "reference",
/// plus a transformation matrix. The result of the transformation being applied to the reference tex-coords is always kept identical
/// to the tex-coords of the control-vertices.
///
/// Several dependency relationships exist between these components:
/// 1. The control-vertices are the authoritative master data. Only those are ever loaded from and saved to disk.
/// 2. If the positions of the control-vertices change, the render mesh (positions, normals, tangets, binormals, ...) must be updated.
/// 3. If the tex-coords of the control-vertices change,
///    a) the render mesh (its tex-coords) must be updated, and
///    b) also the auxiliary data must be reset: The transformation must be set to identity and the reference tex-coords must be
///       set equal to those of the control-vertices, so that the auxiliary data describes the same tex-coords as the control-vertices.
/// 4. If the transformation matrix of the auxiliary data changes (as a result of user interactivity),
///    the tex-coords of the control-vertices must be set to the transformed reference tex-coords of the auxiliary data.
///    Doing so should recurse into the render mesh (case 3a), but not into the auxiliary data (case 3b).
class MapBezierPatchT : public MapPrimitiveT
{
    public:

    // Endcap positions.
    enum EndCapPosE
    {
        TOP_RIGHT=0,
        TOP_LEFT,
        BOTTOM_RIGHT,
        BOTTOM_LEFT
    };


    /// The default constructor. It creates an "empty" bezier patch.
    MapBezierPatchT(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, int SubdivsHorz_=-1, int SubdivsVert_=-1);

    /// The copy constructor for copying a bezier patch.
    /// @param BP   The bezier patch to copy-construct this bezier patch from.
    MapBezierPatchT(const MapBezierPatchT& BP);

    /// The destructor.
    ~MapBezierPatchT();

    // Create bezier patches in different forms.
    static MapBezierPatchT* CreateSimplePatch(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long width, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);
    static MapBezierPatchT* CreatePatchCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);
    static MapBezierPatchT* CreateSquareCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);
    static MapBezierPatchT* CreateQuarterCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);
    static MapBezierPatchT* CreateHalfCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);
    static MapBezierPatchT* CreateEdgePipe(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);
    static MapBezierPatchT* CreateCone(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);
    static MapBezierPatchT* CreateSphere(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1);

    // Endcaps for bezier patches.
    static MapBezierPatchT* CreateQuarterDisc(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1, EndCapPosE pos=TOP_RIGHT, bool Inverted=false);
    static MapBezierPatchT* CreateConcaveEndcap(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_=-1, int SubdivsVert_=-1, EndCapPosE pos=TOP_RIGHT);


    // Implementations and overrides for base class methods.
    MapBezierPatchT* Clone() const override;


    // MapElementT functions
    BoundingBox3fT GetBB() const;
    bool TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const;
    bool TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;

    // Implement the MapElementT transformation methods.
    TrafoMementoT* GetTrafoState() const override;
    void RestoreTrafoState(const TrafoMementoT* TM) override;
    void TrafoMove(const Vector3fT& Delta, bool LockTexCoords) override;
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords) override;
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords) override;
    void TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords) override;
    void Transform(const Matrix4x4fT& Matrix, bool LockTexCoords) override;

    void                        Load_D3_map(TextParserT& TP, unsigned long patchDef, EditorMatManT& MatMan);
    void                        Load_cmap(TextParserT& TP, MapDocumentT& MapDoc, bool IgnoreGroups) override;
    void                        Save_cmap(std::ostream& OutFile, unsigned long PatchNr, const MapDocumentT& MapDoc) const;

    bool                        IsTranslucent() const;
    void                        Render2D(Renderer2DT& Renderer) const;
    void                        Render3D(Renderer3DT& Renderer) const;

    void                        SetMaterial(EditorMaterialI* Mat);
    EditorMaterialI*            GetMaterial() const { return Material; }

    // patch manipulation
    const Vector3fT& GetCvPos(unsigned long x, unsigned long y) const { return cv_Pos[y*cv_Width + x]; }

    void SetCvPos(unsigned long x, unsigned long y, const Vector3fT& Pos)
    {
        cv_Pos[y*cv_Width + x]=Pos;

        NeedsUpdate=true;
    }


    const Vector3fT& GetCvUV(unsigned long x, unsigned long y) const { return cv_UVs[y*cv_Width + x]; }

    void SetCvUV(unsigned long x, unsigned long y, const Vector3fT& uv)
    {
        cv_UVs[y*cv_Width + x]=uv;

        NeedsUpdate=true;
    }


    int  GetSubdivsHorz() const { return SubdivsHorz; }

    void SetSubdivsHorz(int subdivs)
    {
        SubdivsHorz=subdivs;

        NeedsUpdate=true;
    }


    int  GetSubdivsVert() const { return SubdivsVert; }

    void SetSubdivsVert(int subdivs)
    {
        SubdivsVert=subdivs;

        NeedsUpdate=true;
    }

    void InvertPatch();

    /// Set a new SurfaceInfoT.
    void SetSurfaceInfo(const SurfaceInfoT& SI);

    /// Returns the surface info that is associated with this patch.
    const SurfaceInfoT& GetSurfaceInfo() const { return SurfaceInfo; }


    void                SetSize(unsigned long width, unsigned long height);

    // Get the number of control vertices in width or height of the bezier patch.
    unsigned long       GetWidth()        const { return cv_Width; }
    unsigned long       GetHeight()       const { return cv_Height; }
    // Get the number of vertices in width or height of the rendermesh representation of the bezier patch.
    unsigned long       GetRenderWidth()  const { UpdateRenderMesh(); return BPRenderMesh->Meshes.Size()+1; }
    unsigned long       GetRenderHeight() const { UpdateRenderMesh(); assert(BPRenderMesh->Meshes.Size()>0); return BPRenderMesh->Meshes[0]->Vertices.Size()/2; }

    // Get the origin of vertice x/y from the rendermesh
    Vector3fT           GetRenderVertexPos(unsigned long x, unsigned long y) const;

    void Render3D_Basic(MatSys::RenderMaterialT* RenderMat, const wxColour& MeshColor, const int MeshAlpha) const;    ///< A helper method for Render3D(), but also useful e.g. for preview renderings by the "New Bezier Patch" tool.

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    void operator = (const MapBezierPatchT&);   ///< Use of the Assignment Operator is not allowed.

    bool ReconstructMatFit();
    bool ReconstructPlaneProj(bool AltOrigin);

    /// If `SurfaceInfo.TexCoordGenMode == Custom`, this method tries to reconstruct a
    /// `SurfaceInfo` in `MatFit` or `PlaneProj` mode.
    bool ReconstructSI();

    void UpdateRenderMesh() const;
    void UpdateTextureSpace();      ///< Updates the UV coordinates of the patch according to the projection and orientation values from SurfaceInfoT.

    // Note: The members starting with cv_ could be replaced by a BezierPatchNodeT object, that contains the same variables.
    ArrayT<Vector3fT>             cv_Pos;       ///< The positions of the control vertices.
    ArrayT<Vector3fT>             cv_UVs;       ///< The texture-coordinates of the control vertices.
    unsigned long                 cv_Width;     ///< The size of the control vertices array in x-direction.
    unsigned long                 cv_Height;    ///< The size of the control vertices array in y-direction.
    int                           SubdivsHorz;  ///< The explicit number of subdivisions in horizontal direction for rendering (and clipping?) this patch, or -1 for automatic choice.
    int                           SubdivsVert;  ///< The explicit number of subdivisions in vertical   direction for rendering (and clipping?) this patch, or -1 for automatic choice.

    mutable bool                  NeedsUpdate;

    SurfaceInfoT                  SurfaceInfo;
    cf::SceneGraph::LightMapManT& LMM;
    mutable cf::SceneGraph::BezierPatchNodeT*   BPRenderMesh;
    mutable cf::ClipSys::CollisionModelStaticT* CollModel;  ///< For implementing the TraceRay() method.
    EditorMaterialI*               Material;
};

#endif
