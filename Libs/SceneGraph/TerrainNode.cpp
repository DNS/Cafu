/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "TerrainNode.hpp"
#include "_aux.hpp"
#include "MaterialSystem/MapComposition.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "Math3D/Matrix.hpp"
#include "Terrain/Terrain.hpp"

#include <cassert>

using namespace cf::SceneGraph;


TerrainNodeT::TerrainNodeT()
    : Terrain(NULL),
      TerrainShareID(0),
      RenderMaterial(NULL),
      LightMapTexture(NULL)
{
    Init();
}


TerrainNodeT::TerrainNodeT(const BoundingBox3dT& BB_, const TerrainT& Terrain_, unsigned long TerrainShareID_, const std::string& MaterialName_, const float LightMapPatchSize)
    : BB(BB_),
      Terrain(&Terrain_),
      TerrainShareID(TerrainShareID_),
      MaterialName(MaterialName_),
      LightMap(),
      SHLMap(),
      RenderMaterial(NULL),
      LightMapTexture(NULL)
{
    // Initialize the LightMap.
    const double        BB_DiffX           =BB.Max.x-BB.Min.x;
    const double        BB_DiffY           =BB.Max.y-BB.Min.y;
    const double        BB_MinDiff         =(BB_DiffX<BB_DiffY) ? BB_DiffX : BB_DiffY;
    const unsigned long NrOfLightMapPatches=(unsigned long)(BB_MinDiff / LightMapPatchSize) + 1;  // The +1 is for "ceil()".

    // Now find the next smaller power of two.
    unsigned long Pow2NrOfLightMapPatches=1;
    while (Pow2NrOfLightMapPatches*2<=NrOfLightMapPatches) Pow2NrOfLightMapPatches*=2;

    if (Pow2NrOfLightMapPatches>256) Pow2NrOfLightMapPatches=256;

    // This condition is also verified by CaLight, because it doesn't make sense
    // if a single terrain cell is covered by multiple lightmap elements.
    if (Pow2NrOfLightMapPatches>Terrain->GetSize()-1) Pow2NrOfLightMapPatches=Terrain->GetSize()-1;

    // (Now, the true patch size is obtained by (BB.Max.x-BB.Min.x)/Pow2NrOfLightMapPatches and (BB.Max.y-BB.Min.y)/Pow2NrOfLightMapPatches.)

    // Create a full-bright lightmap.
    for (unsigned long LMElemNr=0; LMElemNr<3*Pow2NrOfLightMapPatches*Pow2NrOfLightMapPatches; LMElemNr++) LightMap.PushBack(0xFF);

    // Initialize the SHLMap.
    // ...
    // Nothing needs to be done, since we're creating a 0-band SHL map.

    Init();
}


TerrainNodeT* TerrainNodeT::CreateFromFile_cw(std::istream& InFile, aux::PoolT& /*Pool*/, LightMapManT& /*LMM*/, SHLMapManT& /*SMM*/, const ArrayT<const TerrainT*>& ShTe)
{
    TerrainNodeT* TN=new TerrainNodeT();

    TN->BB.Min        =aux::ReadVector3d(InFile);
    TN->BB.Max        =aux::ReadVector3d(InFile);
    TN->TerrainShareID=aux::ReadUInt32(InFile);
    TN->MaterialName  =aux::ReadString(InFile);
    TN->Terrain       =ShTe[TN->TerrainShareID];

    for (unsigned long Size=aux::ReadUInt32(InFile); Size>0; Size--) { char c; InFile.read(&c, sizeof(c)); TN->LightMap.PushBack(c); }
    for (unsigned long Size=aux::ReadUInt32(InFile); Size>0; Size--) { float f=0.0; InFile.read((char*)&f, sizeof(f)); TN->SHLMap.PushBack(f); }

    TN->Init();
    return TN;
}


TerrainNodeT::~TerrainNodeT()
{
    Clean();
}


void TerrainNodeT::WriteTo(std::ostream& OutFile, aux::PoolT& /*Pool*/) const
{
    aux::Write(OutFile, "Terrain");

    aux::Write(OutFile, BB.Min);
    aux::Write(OutFile, BB.Max);
    aux::Write(OutFile, aux::cnc_ui32(TerrainShareID));
    aux::Write(OutFile, MaterialName);

    aux::Write(OutFile, aux::cnc_ui32(LightMap.Size()));
    for (unsigned long c=0; c<LightMap.Size(); c++) OutFile.write(&LightMap[c], sizeof(char));

    aux::Write(OutFile, aux::cnc_ui32(SHLMap.Size()));
    for (unsigned long c=0; c<SHLMap.Size(); c++) OutFile.write((char*)&SHLMap[c], sizeof(float));
}


const BoundingBox3T<double>& TerrainNodeT::GetBoundingBox() const
{
    return BB;
}


bool TerrainNodeT::IsOpaque() const
{
    return true;
}


void TerrainNodeT::DrawAmbientContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    // TODO: Get fov_y directly from OpenGLWindow.
    // TODO: Try to avoid getting the mp and mv matrices every frame.
    // TODO: Note that the matrices stuff needs attention only once per frame for all terrains, not for each terrain anew.
    TerrainT::ViewInfoT VI;

    const float Window_Width =1024.0;
    const float Window_Height= 768.0;

    // The basic formula for fov_x is from soar/main.c, reshape_callback function, rearranged for fov_x.
    // The 67.5 is a fixed value from OpenGLWindow.cpp.
    const float tau  =4.0;      // Error tolerance in pixel.
    const float fov_y=67.5;
    const float fov_x=2.0f*atan(Window_Width/Window_Height*tan((fov_y*3.14159265358979323846f/180.0f)/2.0f));
    const float kappa=tau/Window_Width*fov_x;

    VI.cull     =true;
    VI.nu       =kappa>0.0f ? 1.0f/kappa : 999999.0f;   // Inverse of error tolerance in radians.
    VI.nu_min   =VI.nu*2.0f/3.0f;                       // Lower morph parameter.
    VI.nu_max   =VI.nu;                                 // Upper morph parameter.
    VI.viewpoint=ViewerPos.AsVectorOfFloat();

    // Set up VI.viewplanes (clip planes) for view frustum culling.
    MatrixT mpv=MatSys::Renderer->GetMatrix(MatSys::Renderer->PROJECTION)*MatSys::Renderer->GetMatrixModelView();

    // Compute view frustum planes.
    for (unsigned long i=0; i<5; i++)
    {
        // m can be used to easily minify / shrink the view frustum.
        // The values should be between 0 and 8: 0 is the default (no minification), 8 is the reasonable maximum.
        const char  m = 0;
        const float d = (i < 4) ? 1.0f - 0.75f*m/8.0f : 1.0f;
        float       plane[4];

        for (unsigned long j=0; j<4; j++)
            plane[j]=((i & 1) ? mpv.m[i/2][j] : -mpv.m[i/2][j]) - d*mpv.m[3][j];

        const float l=sqrt(plane[0]*plane[0]+plane[1]*plane[1]+plane[2]*plane[2]);

        VI.viewplanes[i]=Plane3fT(Vector3fT(plane[0]/l, plane[1]/l, plane[2]/l), -plane[3]/l);
    }


    // Setup texturing for the terrain, using automatic texture coordinates generation.
    const float CoordPlane1[4]={ 1.0f/float(BB.Max.x-BB.Min.x),  0.0f, 0.0f, float(-BB.Min.x/(BB.Max.x-BB.Min.x)) };
    const float CoordPlane2[4]={ 0.0f, -1.0f/float(BB.Max.y-BB.Min.y), 0.0f, float( BB.Max.y/(BB.Max.y-BB.Min.y)) };   // Texture y-axis points top-down!

    MatSys::Renderer->SetGenPurposeRenderingParam( 4, CoordPlane1[0]);
    MatSys::Renderer->SetGenPurposeRenderingParam( 5, CoordPlane1[1]);
    MatSys::Renderer->SetGenPurposeRenderingParam( 6, CoordPlane1[2]);
    MatSys::Renderer->SetGenPurposeRenderingParam( 7, CoordPlane1[3]);
    MatSys::Renderer->SetGenPurposeRenderingParam( 8, CoordPlane2[0]);
    MatSys::Renderer->SetGenPurposeRenderingParam( 9, CoordPlane2[1]);
    MatSys::Renderer->SetGenPurposeRenderingParam(10, CoordPlane2[2]);
    MatSys::Renderer->SetGenPurposeRenderingParam(11, CoordPlane2[3]);


    MatSys::Renderer->SetCurrentMaterial(RenderMaterial);
    MatSys::Renderer->SetCurrentLightMap(LightMapTexture);
    MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.


    // Finally, draw the terrain.
#if 1
    const ArrayT<Vector3fT>& VectorStrip=Terrain->ComputeVectorStrip(VI);

    static MatSys::MeshT TerrainMesh(MatSys::MeshT::TriangleStrip);
    TerrainMesh.Vertices.Overwrite();
    TerrainMesh.Vertices.PushBackEmpty(VectorStrip.Size());

    for (unsigned long VNr=0; VNr<VectorStrip.Size(); VNr++)
        TerrainMesh.Vertices[VNr].SetOrigin(VectorStrip[VNr]);
#else
    const ArrayT<Vector3fT>& VectorStrip=Terrain->ComputeVectorStripByMorphing(VI);

    static MatSys::MeshT TerrainMesh(MatSys::MeshT::TriangleStrip);
    TerrainMesh.Vertices.Overwrite();
    TerrainMesh.Vertices.PushBackEmpty(VectorStrip.Size()-1);

    // Note that the first VectorT at VectorStrip[0] must be skipped!
    for (unsigned long VNr=1; VNr<VectorStrip.Size(); VNr++)
        TerrainMesh.Vertices[VNr-1].SetOrigin(VectorStrip[VNr]);
#endif

    MatSys::Renderer->RenderMesh(TerrainMesh);
}


void TerrainNodeT::DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::STENCILSHADOW);
}


void TerrainNodeT::DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING);
}


void TerrainNodeT::DrawTranslucentContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);
}


void TerrainNodeT::Init()
{
    if (MatSys::Renderer         ==NULL) return;
    if (MatSys::TextureMapManager==NULL) return;
    if (Terrain==NULL) return;
    if (LightMap.Size()<3) return;

    Clean();

#if 1
    ArrayT<char>& GammaLightMap=LightMap;
#else   // This code should actually be removed entirely - gamma for lightmaps is now properly applied in CaLight, and thus there is no need for toying around with it elsewhere.
    ArrayT<char> GammaLightMap;

    // Wende r_gamma_lm auf die LightMaps an.
    // Beachte die nette Optimierung (Vorberechnung der 256 möglichen Gammawerte)!
    // Dieser Code ist identisch zum DrawableMapT Constructor!
    {
        char GammaValuesLM[256];

        for (unsigned long ValueNr=0; ValueNr<256; ValueNr++)
        {
            // FIXME!!!! Changes here require also changes in DrawableMapT::r_gamma_lm!
            float Value=pow(float(ValueNr)/255.0f, 1.0f/ 1.6f )*255.0f;

            if (Value<0.0f) Value=0.0f; if (Value>255.0f) Value=255.0f;
            GammaValuesLM[ValueNr]=char(Value+0.49f);
        }

        for (unsigned long Nr=0; Nr<LightMap.Size(); Nr++)
            GammaLightMap.PushBack(GammaValuesLM[LightMap[Nr]]);
    }
#endif

    const int LightMapSize=int(sqrt(GammaLightMap.Size()/3.0)+0.5);

    LightMapTexture=MatSys::TextureMapManager->GetTextureMap2D(
        &GammaLightMap[0],
        LightMapSize, LightMapSize,
        3, true,    // MipMaps and scaling down is no problem here, because the bitmap is not shared with other LightMaps, but ClampToEdge should be set!
        MapCompositionT(MapCompositionT::Linear_MipMap_Linear, MapCompositionT::Linear, MapCompositionT::ClampToEdge, MapCompositionT::ClampToEdge));

    RenderMaterial=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(MaterialName));
}


void TerrainNodeT::Clean()
{
    if (RenderMaterial!=NULL)
    {
        // Note that also the MatSys::Renderer pointer can be NULL, but then the RenderMaterial must be NULL, too.
        assert(MatSys::Renderer!=NULL);

        MatSys::Renderer->FreeMaterial(RenderMaterial);
        RenderMaterial=NULL;
    }

    if (LightMapTexture!=NULL)
    {
        // Note that also the MatSys::TextureMapManager pointer can be NULL, but then the LightMapTexture must be NULL, too.
        assert(MatSys::TextureMapManager!=NULL);

        MatSys::TextureMapManager->FreeTextureMap(LightMapTexture);
        LightMapTexture=NULL;
    }
}
