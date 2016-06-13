/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MapTerrain.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "ToolOptionsBars.hpp"

#include "../Camera.hpp"
#include "../EditorMaterial.hpp"
#include "../EditorMaterialManager.hpp"
#include "../Options.hpp"

#include "Bitmap/Bitmap.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "TextParser/TextParser.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapTerrainT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapTerrainT::TypeInfo(GetMapElemTIM(), "MapTerrainT", "MapPrimitiveT", MapTerrainT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapTerrainT::MapTerrainT(const BoundingBox3fT& Box, const wxString& HeightMapFile, EditorMaterialI* Material)
    : MapPrimitiveT(wxColour(50 + (rand() % 50), 150 + (rand() % 106), 30)),
      m_Resolution(65),
      m_HeightData(),
      m_TerrainBounds(Box),
      m_Material(Material),
      m_ToolBounds(Vector3fT()),
      m_RenderEyeDropper(false),
      m_NeedsUpdate(true),
      m_Terrain(),
      m_TerrainMesh(MatSys::MeshT::TriangleStrip)
{
    // Try to load the height data from the passed height map file. Create empty terrain if it fails.
    try
    {
        //XXX Hmmmmm.  Evtl. sollten wir eine TerrainLoaderT Klasse machen (die später auch speichern kann)...
        LoadHeightData(HeightMapFile);
    }
    catch (const BitmapT::LoadErrorT& /*E*/)
    {
        // Set default resolution in case LoadHeightData has already changed it.
        m_Resolution=65;

        for (unsigned int i=0; i<m_Resolution*m_Resolution; i++)
            m_HeightData.PushBack(0);
    }
}


MapTerrainT::MapTerrainT(const MapTerrainT& Terrain)
    : MapPrimitiveT(Terrain),
      m_Resolution(Terrain.m_Resolution),
      m_HeightData(Terrain.m_HeightData),
      m_TerrainBounds(Terrain.m_TerrainBounds),
      m_Material(Terrain.m_Material),
      m_ToolBounds(Terrain.m_ToolBounds),
      m_RenderEyeDropper(false),
      m_NeedsUpdate(Terrain.m_NeedsUpdate),
      m_Terrain(Terrain.m_Terrain),
      m_TerrainMesh(Terrain.m_TerrainMesh)
{
}


MapTerrainT* MapTerrainT::Clone() const
{
    return new MapTerrainT(*this);
}


void MapTerrainT::SetTerrainBounds(const BoundingBox3fT& Bounds)
{
    m_TerrainBounds=Bounds;
    m_NeedsUpdate=true;
}


BoundingBox3fT MapTerrainT::GetBB() const
{
    return m_TerrainBounds;
}


void MapTerrainT::Render2D(Renderer2DT& Renderer) const
{
    const wxPoint Point1=Renderer.GetViewWin2D().WorldToTool(m_TerrainBounds.Min);
    const wxPoint Point2=Renderer.GetViewWin2D().WorldToTool(m_TerrainBounds.Max);
    const wxPoint Center=Renderer.GetViewWin2D().WorldToTool(m_TerrainBounds.GetCenter());

    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN,
        IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors));

    Renderer.Rectangle(wxRect(Point1, Point2), false);

    // Render the center X handle.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(150, 150, 150));
    Renderer.XHandle(Center);
}


void MapTerrainT::Render3D(Renderer3DT& Renderer) const
{
    TerrainT::ViewInfoT VI;

    const wxSize    WinSize=Renderer.GetViewWin3D().GetClientSize();
    const CameraT&  Camera =Renderer.GetViewWin3D().GetCamera();
    const Plane3fT* Planes =Renderer.GetViewFrustumPlanes();
    const float     tau    =4.0;    // Error tolerance in pixel.
    const float     fov_y  =Camera.VerticalFOV;
    const float     fov_x  =2.0*atan(float(WinSize.GetWidth())/float(WinSize.GetHeight())*tan((fov_y*3.14159265358979323846/180.0)/2.0));
    const float     kappa  =tau/WinSize.GetWidth()*fov_x;

    VI.cull     =true;
    VI.nu       =kappa>0.0 ? 1.0/kappa : 999999.0;  // Inverse of error tolerance in radians.
    VI.nu_min   =VI.nu*2.0/3.0;                     // Lower morph parameter.
    VI.nu_max   =VI.nu;                             // Upper morph parameter.
    VI.viewpoint=Camera.Pos;

    for (unsigned long i=0; i<5; i++)   // We don't need the sixth (the "far") plane.
        VI.viewplanes[i]=Planes[i];


    // Setup texturing for the terrain, using automatic texture coordinates generation.
    {
        const BoundingBox3fT& BB=m_TerrainBounds;

        const float CoordPlane1[4]={ 1.0f/(BB.Max.x-BB.Min.x),  0.0f, 0.0f, -BB.Min.x/(BB.Max.x-BB.Min.x) };
        const float CoordPlane2[4]={ 0.0f, -1.0f/(BB.Max.y-BB.Min.y), 0.0f,  BB.Max.y/(BB.Max.y-BB.Min.y) };   // Texture y-axis points top-down!

        MatSys::Renderer->SetGenPurposeRenderingParam( 4, CoordPlane1[0]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 5, CoordPlane1[1]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 6, CoordPlane1[2]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 7, CoordPlane1[3]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 8, CoordPlane2[0]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 9, CoordPlane2[1]);
        MatSys::Renderer->SetGenPurposeRenderingParam(10, CoordPlane2[2]);
        MatSys::Renderer->SetGenPurposeRenderingParam(11, CoordPlane2[3]);
    }


    // Finally, draw the terrain.
    m_TerrainMesh.Vertices.Overwrite();

#if 1
    const ArrayT<Vector3fT>& VectorStrip=GetTerrain().ComputeVectorStrip(VI);

    m_TerrainMesh.Vertices.PushBackEmpty(VectorStrip.Size());

    for (unsigned long VNr=0; VNr<VectorStrip.Size(); VNr++)
        m_TerrainMesh.Vertices[VNr].SetOrigin(VectorStrip[VNr]);
#else
    if (/*VI_morph*/ true)
    {
        const ArrayT<Vector3fT>& VectorStrip=GetTerrain().ComputeVectorStripByMorphing(VI);

        m_TerrainMesh.Vertices.PushBackEmpty(VectorStrip.Size()-1);

        // Note that the first VectorT at VectorStrip[0] must be skipped!
        for (unsigned long VNr=1; VNr<VectorStrip.Size(); VNr++)
            m_TerrainMesh.Vertices[VNr-1].SetOrigin(VectorStrip[VNr]);
    }
    else
    {
        const ArrayT<unsigned long>& IdxStrip=GetTerrain().ComputeIndexStripByRefinement(VI);
        const TerrainT::VertexT*     Vertices=GetTerrain().GetVertices();

        m_TerrainMesh.Vertices.PushBackEmpty(IdxStrip.Size()-1);

        // Note that the first index at IdxStrip[0] must be skipped!
        for (unsigned long IdxNr=1; IdxNr<IdxStrip.Size(); IdxNr++)
            m_TerrainMesh.Vertices[IdxNr-1].SetOrigin(Vertices[IdxStrip[IdxNr]]);
    }
#endif

    MatSys::Renderer->SetCurrentMaterial(m_Material->GetRenderMaterial(true /*Terrains are always in "Full Material" mode.*/));
 // MatSys::Renderer->SetCurrentLightMap(LightMapTexture);
 // MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.
    MatSys::Renderer->RenderMesh(m_TerrainMesh);

    if (IsSelected())
    {
        // Todo: Set all vertex colors to yellow.
        MatSys::Renderer->SetCurrentMaterial(Renderer.GetRMatWireframe_OffsetZ());
        MatSys::Renderer->RenderMesh(m_TerrainMesh);
    }

    // Render tool if its bounding box has volume.
    if (m_ToolBounds.Min!=m_ToolBounds.Max)
    {
        const BoundingBox3fT& BB=m_ToolBounds;

        const float CoordPlane1[4]={ 1.0f/(BB.Max.x-BB.Min.x),  0.0f, 0.0f, -BB.Min.x/(BB.Max.x-BB.Min.x) };
        const float CoordPlane2[4]={ 0.0f, -1.0f/(BB.Max.y-BB.Min.y), 0.0f,  BB.Max.y/(BB.Max.y-BB.Min.y) };   // Texture y-axis points top-down!

        MatSys::Renderer->SetGenPurposeRenderingParam( 4, CoordPlane1[0]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 5, CoordPlane1[1]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 6, CoordPlane1[2]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 7, CoordPlane1[3]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 8, CoordPlane2[0]);
        MatSys::Renderer->SetGenPurposeRenderingParam( 9, CoordPlane2[1]);
        MatSys::Renderer->SetGenPurposeRenderingParam(10, CoordPlane2[2]);
        MatSys::Renderer->SetGenPurposeRenderingParam(11, CoordPlane2[3]);

        if (m_RenderEyeDropper)
            MatSys::Renderer->SetCurrentMaterial(Renderer.GetRMatTerrainEyeDropper());
        else
            MatSys::Renderer->SetCurrentMaterial(Renderer.GetRMatTerrainEditorTool());

        MatSys::Renderer->RenderMesh(m_TerrainMesh);
    }
}


// NOTE: An overload of this method with a similar signature exists!
bool MapTerrainT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    // If possible (RayOrigin is not in cull bounding-box), do a quick bounding-box check first.
    if (!GetBB().Contains(RayOrigin) && !MapPrimitiveT::TraceRay(RayOrigin, RayDir, Fraction, FaceNr)) return false;

    const double       RayLength=1000000.0;
    VB_Trace3T<double> Trace(1.0);

    m_Terrain.TraceBoundingBox(BoundingBox3dT(Vector3dT()), RayOrigin.AsVectorOfDouble(), RayDir.AsVectorOfDouble()*RayLength, Trace);

    if (Trace.Fraction==1.0) return false;

    Fraction=Trace.Fraction*RayLength;
    return true;
}


bool MapTerrainT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    const BoundingBox3fT BB  =GetBB();
    const wxRect         Disc=wxRect(Pixel, Pixel).Inflate(Radius, Radius);
    const wxRect         Rect=wxRect(ViewWin.WorldToWindow(BB.Min), ViewWin.WorldToWindow(BB.Max));

    // Note that the check against the Rect frame (that has a width of 2*Radius) is done in two steps:
    // First by checking if Disc is entirely outside of Rect, then below by checking if Disc is entirely inside Rect.
    if (!Rect.Intersects(Disc)) return false;
    if (Disc.Contains(ViewWin.WorldToWindow(BB.GetCenter()))) return true;
    if (Options.view2d.SelectByHandles) return false;
    return !Rect.Contains(Disc);
}


void MapTerrainT::SetToolBounds(int PosX, int PosY, int Radius)
{
    if (PosX==-1 || PosY==-1 || Radius==-1)
    {
        // Set bounding box to zero.
        m_ToolBounds=BoundingBox3fT(Vector3fT());
        return;
    }

    // First we have to flip the y position horizontally.
    PosY=m_Resolution-1-PosY;

    // Calculate tool bounds from terrain bounds.
    const float ToolMinX=float(PosX-Radius-1)/float(m_Resolution-1);
    const float ToolMinY=float(PosY-Radius-1)/float(m_Resolution-1);
    const float ToolMaxX=float(PosX+Radius+1)/float(m_Resolution-1);
    const float ToolMaxY=float(PosY+Radius+1)/float(m_Resolution-1);

    const float TerrainXLength=m_TerrainBounds.Max.x-m_TerrainBounds.Min.x;
    const float TerrainYLength=m_TerrainBounds.Max.y-m_TerrainBounds.Min.y;

    const Vector3fT ToolMin(m_TerrainBounds.Min.x+TerrainXLength*ToolMinX, m_TerrainBounds.Min.y+TerrainYLength*ToolMinY, 0.0f);
    const Vector3fT ToolMax(m_TerrainBounds.Min.x+TerrainXLength*ToolMaxX, m_TerrainBounds.Min.y+TerrainYLength*ToolMaxY, 0.0f);

    m_ToolBounds=BoundingBox3fT(ToolMin, ToolMax);
}


#if __GNUC__>3
// This is utterly annoying, see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509 for details.
static size_t fread_no_warn(void* buffer, size_t size, size_t items, FILE* file)
{
    return fread(buffer, size, items, file);
}

#define fread fread_no_warn
#endif


void LoadHeightmap(const wxString& FileName, unsigned long& Resolution, ArrayT<unsigned short>& HeightData)
{
    if (FileName.EndsWith(".ter"))
    {
        // Load terrain from Terragen file.
        FILE* FilePtr=fopen(FileName.c_str(), "rb");
        if (FilePtr==NULL) throw BitmapT::LoadErrorT();

        // Read header.
        char Header[20];
        fread(Header, sizeof(char), 16, FilePtr);
        Header[16] = 0;
        if (strncmp(Header, "TERRAGENTERRAIN ", 16)!=0) { fclose(FilePtr); throw BitmapT::LoadErrorT(); }   // Header=="TERRAGENTERRAIN "

        // Reset the return values.
        unsigned int SizeX=0;
        unsigned int SizeY=0;

        // Other values from the file.
        VectorT MetersPerPixel(30.0, 30.0, 30.0);           // The distance in meters between two neighboured pixels.

        // Read the chunks.
        while (true)
        {
            char ChunkMarker[10];
            fread(ChunkMarker, sizeof(char), 4, FilePtr);
            ChunkMarker[4] = 0;

            if (feof(FilePtr)) break;
            if (strncmp(ChunkMarker, "EOF ", 4)==0) break;

            if (strncmp(ChunkMarker, "XPTS", 4)==0)
            {
                short int XPts;
                fread(&XPts, sizeof(XPts), 1, FilePtr);     // Read two bytes.
                SizeX=XPts;                                 // Let the XPTS chunk always override previous values (it appears only after SIZE anyway).
                fread(&XPts, sizeof(XPts), 1, FilePtr);     // Overread padding bytes.
            }
            else if (strncmp(ChunkMarker, "YPTS", 4)==0)
            {
                short int YPts;
                fread(&YPts, sizeof(YPts), 1, FilePtr);     // Read two bytes.
                SizeY=YPts;                                 // Let the YPTS chunk always override previous values (it appears only after SIZE anyway).
                fread(&YPts, sizeof(YPts), 1, FilePtr);     // Overread padding bytes.
            }
            else if (strncmp(ChunkMarker, "SIZE", 4)==0)
            {
                short int Size;
                fread(&Size, sizeof(Size), 1, FilePtr);     // Read two bytes.
                SizeX=Size+1;                               // Note that the XPTS and YPTS chunks appear *after* the SIZE chunk.
                SizeY=Size+1;
                fread(&Size, sizeof(Size), 1, FilePtr);     // Overread padding bytes.
            }
            else if (strncmp(ChunkMarker, "SCAL", 4)==0)
            {
                float Scale;
                fread(&Scale, sizeof(Scale), 1, FilePtr); MetersPerPixel.x=Scale;
                fread(&Scale, sizeof(Scale), 1, FilePtr); MetersPerPixel.y=Scale;
                fread(&Scale, sizeof(Scale), 1, FilePtr); MetersPerPixel.z=Scale;
            }
            else if (strncmp(ChunkMarker, "CRAD", 4)==0)
            {
                float PlanetRadius;
                fread(&PlanetRadius, sizeof(PlanetRadius), 1, FilePtr);     // This value is ignored.
            }
            else if (strncmp(ChunkMarker, "CRVM", 4)==0)
            {
                unsigned long RenderMode;
                fread(&RenderMode, sizeof(RenderMode), 1, FilePtr);         // This value is ignored.
            }
            else if (strncmp(ChunkMarker, "ALTW", 4)==0)
            {
                short int HeightScale;
                fread(&HeightScale, sizeof(HeightScale), 1, FilePtr);
                short int BaseHeight;
                fread(&BaseHeight, sizeof(BaseHeight), 1, FilePtr);

                if (SizeX!=SizeY) throw BitmapT::LoadErrorT();

                Resolution=SizeX;

                HeightData.Clear();
                HeightData.PushBackEmpty(Resolution*Resolution);

                for (unsigned long y=0; y<Resolution; y++)
                    for (unsigned long x=0; x<Resolution; x++)
                    {
                        short int Elevation;
                        fread(&Elevation, sizeof(Elevation), 1, FilePtr);

                        HeightData[y*Resolution+x]=(unsigned short)(int(Elevation)+32768);
                    }

                // Break here, because we're done *and* unspecified chunks like the THMB thumbnail-chunk might follow.
                // As I don't know how to deal with such chunks (where are the specs anyway?), just stop here.
                break;
            }
            else
            {
                // Unknown chunk?
                fclose(FilePtr);
                throw BitmapT::LoadErrorT();
            }
        }

        fclose(FilePtr);
    }
    else if (FileName.EndsWith(".pgm"))
    {
        try
        {
            // Load terrain from Portable Gray-Map file (only the ASCII variant).
            TextParserT TP(FileName.c_str(), "", true, '#');

            std::string MagicNumber=TP.GetNextToken();
            bool        IsAscii    =(MagicNumber=="P2");

            // If it's not P2 and not P5, something is wrong.
            if (!IsAscii && MagicNumber!="P5") throw TextParserT::ParseError();

            unsigned int SizeX=TP.GetNextTokenAsInt();
            unsigned int SizeY=TP.GetNextTokenAsInt();
            double MaxVal     =TP.GetNextTokenAsFloat();

            if (SizeX!=SizeY) throw BitmapT::LoadErrorT();

            Resolution=SizeX;

            HeightData.Clear();
            HeightData.PushBackEmpty(Resolution*Resolution);

            if (IsAscii)
            {
                // It's the P2 ascii type.
                for (unsigned long y=0; y<Resolution; y++)
                    for (unsigned long x=0; x<Resolution; x++)
                        HeightData[(Resolution-y-1)*Resolution+x]=(unsigned short)((TP.GetNextTokenAsFloat()/MaxVal)*65535.0);
            }
            else
            {
                // It's the P5 binary type.
                FILE* FilePtr=fopen(FileName, "rb");
                if (FilePtr==NULL) throw BitmapT::LoadErrorT();

                // Assume that after the last text token (the maximum value) exactly *one* byte of white-space (e.g. '\r' or '\n') follows.
                fseek(FilePtr, TP.GetReadPosByte()+1, SEEK_SET);

                for (unsigned long y=0; y<Resolution; y++)
                    for (unsigned long x=0; x<Resolution; x++)
                    {
                        unsigned char Value;
                        fread(&Value, sizeof(Value), 1, FilePtr);   // This probably slow as hell, but alas! Who cares?

                        HeightData[(Resolution-y-1)*Resolution+x]=(unsigned short)((Value/MaxVal)*65535.0);
                    }

                fclose(FilePtr);
            }
        }
        catch (const TextParserT::ParseError&)
        {
            throw BitmapT::LoadErrorT();
        }
    }
    else
    {
        // Load terrain from image file.
        BitmapT HeightMap=BitmapT(FileName.c_str());

        if (HeightMap.SizeX!=HeightMap.SizeY) throw BitmapT::LoadErrorT();

        Resolution=HeightMap.SizeX;

        HeightData.Clear();
        HeightData.PushBackEmpty(Resolution*Resolution);

        // Note that we pick the red channel of the RBG HeightMap.Data as the relevant channel.
        // Moreover, note that the y-axis of the HeightMap.Data points down in screen-space and thus towards us in world space.
        // Our world-space y-axis points opposite (away from us), though, and therefore we access the HeightMap.Data at (i, Size-j-1).
        for (unsigned long y=0; y<Resolution; y++)
            for (unsigned long x=0; x<Resolution; x++)
                HeightData[y*Resolution+x]=(unsigned short)((double(HeightMap.Data[(Resolution-y-1)*Resolution+x] & 0xFF)/255.0)*65535);
    }


    // Repeat the check from TerrainT::Init() here.
    // This is quite cumbersome though, we should probably move and combine this into a common a HeightmapT class...
    if (Resolution < 3) throw BitmapT::LoadErrorT();
    if (Resolution > (1UL << 30/2) + 1) throw BitmapT::LoadErrorT();

    unsigned long Levels = 1;
    while ((1UL << Levels) + 1 < Resolution) Levels++;
    Levels *= 2;

    const unsigned long Size = (1UL << (Levels/2)) + 1;

    if (Size != Resolution) throw BitmapT::LoadErrorT();
}


void MapTerrainT::LoadHeightData(const wxString& FileName)
{
    unsigned long          NewResolution = 0;
    ArrayT<unsigned short> NewHeightData;

    LoadHeightmap(FileName, NewResolution, NewHeightData);

    m_Resolution = NewResolution;
    m_HeightData = NewHeightData;
    m_NeedsUpdate = true;
}


// Helper function for code below.
static int Round(float a)
{
    return int(a>=0.0 ? a+0.5f : a-0.5f);
}


// NOTE: An overload of this method with a similar signature exists!
wxPoint MapTerrainT::TraceRay(const Vector3fT& Source, const Vector3fT& Direction) const
{
    VB_Trace3T<double> TraceResult(1.0);

    m_Terrain.TraceBoundingBox(BoundingBox3T<double>(Vector3dT(), Vector3dT()), Source.AsVectorOfDouble(), Direction.AsVectorOfDouble(), TraceResult);

    if (TraceResult.Fraction==1.0) return wxPoint(-1, -1);

    Vector3fT HitPos(Source+Direction*TraceResult.Fraction);

    float TerrainXLength=m_TerrainBounds.Max.x-m_TerrainBounds.Min.x;
    float TerrainYLength=m_TerrainBounds.Max.y-m_TerrainBounds.Min.y;

    Vector3fT HitPosRelative(HitPos.x-m_TerrainBounds.Min.x, HitPos.y-m_TerrainBounds.Min.y, 0.0f);

    wxPoint HeightDataPos(Round(HitPosRelative.x/float(TerrainXLength)*float(m_Resolution-1)), Round(HitPosRelative.y/float(TerrainYLength)*float(m_Resolution-1)));

    // Flip tools y position.
    HeightDataPos.y=m_Resolution-1-HeightDataPos.y;

    // If height data position lies outside height data boundaries return undefined position.
    if (HeightDataPos.x<0 || HeightDataPos.y<0 || (unsigned long)HeightDataPos.x>m_Resolution-1 || (unsigned long)HeightDataPos.y>m_Resolution-1)
        return wxPoint(-1, -1);

    return HeightDataPos;
}


const TerrainT& MapTerrainT::GetTerrain() const
{
    if (m_NeedsUpdate)
    {
        m_Terrain=TerrainT(&m_HeightData[0], m_Resolution, m_TerrainBounds);
        m_NeedsUpdate=false;
    }

    return m_Terrain;
}


namespace
{
    class TerrainTrafoMementoT : public TrafoMementoT
    {
        public:

        TerrainTrafoMementoT(const BoundingBox3fT& BB)
            : m_BB(BB)
        {
        }

        const BoundingBox3fT m_BB;
    };
}


TrafoMementoT* MapTerrainT::GetTrafoState() const
{
    return new TerrainTrafoMementoT(m_TerrainBounds);
}


void MapTerrainT::RestoreTrafoState(const TrafoMementoT* TM)
{
    const TerrainTrafoMementoT* TerrainTM = dynamic_cast<const TerrainTrafoMementoT*>(TM);

    wxASSERT(TerrainTM);
    if (!TerrainTM) return;

    m_TerrainBounds = TerrainTM->m_BB;
    m_NeedsUpdate = true;
}


void MapTerrainT::TrafoMove(const Vector3fT& delta, bool LockTexCoords)
{
    m_TerrainBounds.Min+=delta;
    m_TerrainBounds.Max+=delta;

    m_NeedsUpdate=true;

    MapPrimitiveT::TrafoMove(delta, LockTexCoords);
}


void MapTerrainT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords)
{
    Vector3fT Min=m_TerrainBounds.Min;
    Vector3fT Max=m_TerrainBounds.Max;

    Min-=RefPoint;
    if (Angles.x!=0.0f) Min=Min.GetRotX( Angles.x);
    if (Angles.y!=0.0f) Min=Min.GetRotY(-Angles.y);
    if (Angles.z!=0.0f) Min=Min.GetRotZ( Angles.z);
    Min+=RefPoint;

    Max-=RefPoint;
    if (Angles.x!=0.0f) Max=Max.GetRotX( Angles.x);
    if (Angles.y!=0.0f) Max=Max.GetRotY(-Angles.y);
    if (Angles.z!=0.0f) Max=Max.GetRotZ( Angles.z);
    Max+=RefPoint;

    const BoundingBox3fT NewBB(Min, Max);

    if (!NewBB.IsInited()) return;
    if (NewBB.Max.x-NewBB.Min.x < 4.0f) return;
    if (NewBB.Max.y-NewBB.Min.y < 4.0f) return;

    MapPrimitiveT::TrafoRotate(RefPoint, Angles, LockTexCoords);

    // TODO: Restrict this to steps of 90 degrees around the z-axis, then also rotate the m_HeightData!!!
    // And/or: Terrains cannot be arbitrarily rotated. The user must rotate the terrain's entity instead.

    m_TerrainBounds=NewBB;
    m_NeedsUpdate=true;
}


void MapTerrainT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords)
{
    const BoundingBox3fT NewBB(RefPoint + (m_TerrainBounds.Min-RefPoint).GetScaled(Scale),
                               RefPoint + (m_TerrainBounds.Max-RefPoint).GetScaled(Scale));

    if (!NewBB.IsInited()) return;
    if (NewBB.Max.x-NewBB.Min.x < 4.0f) return;
    if (NewBB.Max.y-NewBB.Min.y < 4.0f) return;

    MapPrimitiveT::TrafoScale(RefPoint, Scale, LockTexCoords);

    m_TerrainBounds=NewBB;
    m_NeedsUpdate=true;
}


void MapTerrainT::TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords)
{
    Vector3fT Min=m_TerrainBounds.Min;
    Vector3fT Max=m_TerrainBounds.Max;

    Min[NormalAxis]=Dist-(Min[NormalAxis]-Dist);
    Max[NormalAxis]=Dist-(Max[NormalAxis]-Dist);

    const BoundingBox3fT NewBB(Min, Max);

    if (!NewBB.IsInited()) return;
    if (NewBB.Max.x-NewBB.Min.x < 4.0f) return;
    if (NewBB.Max.y-NewBB.Min.y < 4.0f) return;

    MapPrimitiveT::TrafoMirror(NormalAxis, Dist, LockTexCoords);

    // TODO: Mirror the m_HeightData!!!

    m_TerrainBounds=NewBB;
    m_NeedsUpdate=true;
}


void MapTerrainT::Transform(const MatrixT& Matrix, bool LockTexCoords)
{
    const BoundingBox3fT NewBB(Matrix.Mul1(m_TerrainBounds.Min),
                               Matrix.Mul1(m_TerrainBounds.Max));

    // TODO: Terrains cannot be arbitrarily transformed. The user must transform the terrain's entity instead.
    if (!NewBB.IsInited()) return;
    if (NewBB.Max.x-NewBB.Min.x < 4.0f) return;
    if (NewBB.Max.y-NewBB.Min.y < 4.0f) return;

    MapPrimitiveT::Transform(Matrix, LockTexCoords);

    m_TerrainBounds=NewBB;
    m_NeedsUpdate=true;
}
