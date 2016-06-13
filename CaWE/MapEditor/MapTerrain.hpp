/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAP_TERRAIN_HPP_INCLUDED
#define CAFU_MAP_TERRAIN_HPP_INCLUDED

#include "MapPrimitive.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Terrain/Terrain.hpp"


class EditorMaterialI;
class EditorMatManT;


/// The TerrainT class represents a terrain in a map.
/// A terrain consists of height data and a material that is used to render the terrain.
/// The dimensions of the terrain are described by a bounding box and all tranformations are performed on this bounding box.
class MapTerrainT : public MapPrimitiveT
{
    public:

    /// Constructor.
    /// If no parameters are passed, an even terrain with dummy material and no size is created at 0,0,0. It is possible to adjust all
    /// these parameters later on, so you can create an "empty" terrain here and fill it later.
    /// @param Box The initial bounds of the terrain.
    /// @param HeightMapFile The heightmap data from which this terrain should be initialized.
    /// @param Material The material to render this terrain with.
    MapTerrainT(const BoundingBox3fT& Box=BoundingBox3fT(Vector3fT()), const wxString& HeightMapFile="", EditorMaterialI* Material=NULL);

    /// The copy constructor for copying a terrain.
    /// @param Terrain   The terrain to copy-construct this terrain from.
    MapTerrainT(const MapTerrainT& Terrain);


    // Implementations and overrides for base class methods.
    MapTerrainT* Clone() const override;


    /// Sets the bounds of the terrain.
    /// @param Bounds The new bounds of the terrain.
    void SetTerrainBounds(const BoundingBox3fT& Bounds);

    /// Sets the terrain material.
    /// @param Material New terrain material.
    void SetMaterial(EditorMaterialI* Material) { m_Material=Material; }

    /// Gets the terrains current material.
    /// @return Current terrain material.
    EditorMaterialI* GetMaterial() const { return m_Material; }

    /// Gets the terrains heigth data resolution side length.
    /// @return Height data side length.
    unsigned long GetResolution() const { return m_Resolution; }

    /// Gets a constant reference to the terrains height data.
    /// @return The terrains height data.
    const ArrayT<unsigned short>& GetHeightData() const { return m_HeightData; }

    /// Initializes the terrains height data from a file.
    /// Warning: This will overwrite any data that the terrain currently has.
    /// @param FileName The file from which the height data should be loaded.
    void LoadHeightData(const wxString& FileName);

    /// Traces the given ray and returns the position in the terrains height data when a hit with the terrain has occured.
    /// @param Source The point from were the trace is emanating.
    /// @param Direction The direction into which the ray is cast.
    /// @return Point of intersection in the height map or -1,-1 if ray doesn't intersect with terrain.
    wxPoint TraceRay(const Vector3fT& Source, const Vector3fT& Direction) const;

    /// Sets the bounds if the terrain edit tool.
    /// The tools bounds are calculated from the given parameters.
    /// @param PosX The x position of the tool in the terrains height data.
    /// @param PosY The y position of the tool in the terrains height data.
    /// @param Radius The tools radius.
    void SetToolBounds(int PosX, int PosY, int Radius);


    // MapElementT implementation.
    BoundingBox3fT GetBB() const;

    void Render2D(Renderer2DT& Renderer) const;
    void Render3D(Renderer3DT& Renderer) const;

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

    wxString GetDescription() const { return "Terrain"; }

    void Load_cmap(TextParserT& TP, MapDocumentT& MapDoc, bool IgnoreGroups) override;
    void Save_cmap(std::ostream& OutFile, unsigned long TerrainNr, const MapDocumentT& MapDoc) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    friend class CommandModifyTerrainT;    // Changes to a terrain are commited by commands that need friend access.
    friend class CommandChangeTerrainResT;
    friend class ToolTerrainEditorT;       // The tool needs friend access to the terrain bounds when checking a 3D hit position.

    /// Returns the terrain object of this map terrain and updates it if necessary.
    /// @return Terrain object of map terrain.
    const TerrainT& GetTerrain() const;


    unsigned long          m_Resolution;    ///< The resolution (side length) of the terrains height data.
    ArrayT<unsigned short> m_HeightData;    ///< Height data of this terrain.
    BoundingBox3fT         m_TerrainBounds; ///< The bounding-box that describes the spatial dimensions of the terrain.
    EditorMaterialI*       m_Material;      ///< The material applied to this terrain.

    BoundingBox3fT         m_ToolBounds;       ///< Bounding box of the editor tool being rendered onto the terrain.
    bool                   m_RenderEyeDropper; ///< Whether to render the editor tool or the eyedropper tool.

    mutable bool           m_NeedsUpdate;   ///< If the m_Terrain member needs to be updated.
    mutable TerrainT       m_Terrain;       ///< Our terrain object.
    mutable MatSys::MeshT  m_TerrainMesh;   ///< The renderable terrain mesh.
};

#endif
