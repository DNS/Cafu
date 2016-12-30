/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "MapBezierPatch.hpp"
#include "MapDocument.hpp"
#include "ChildFrame.hpp"
#include "ToolEditSurface.hpp"
#include "ToolManager.hpp"
#include "DialogEditSurfaceProps.hpp"

#include "../EditorMaterial.hpp"
#include "../EditorMaterialManager.hpp"
#include "../Options.hpp"

#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/BezierPatch.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "TypeSys.hpp"

#include "wx/wx.h"


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


// This should match the value in CaBSP's LoadWorld() function.
// As a possible alternative, see the implementation of BezierPatchT<T>::Subdivide() for an
// elegant suggestion that would make the error metric independent of the size of the Bezier
// patch, and thus independent of any reference to the world, so that the `MAX_CURVE_ERROR`
// could actually be removed.
static const float MAX_CURVE_ERROR = 24.0f;


/*** Begin of TypeSys related definitions for this class. ***/

void* MapBezierPatchT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapBezierPatchT::TypeInfo(GetMapElemTIM(), "MapBezierPatchT", "MapPrimitiveT", MapBezierPatchT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapBezierPatchT::MapBezierPatchT(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, int SubdivsHorz_, int SubdivsVert_)
    : MapPrimitiveT(wxColour(0, 100 + (rand() % 156), 100 + (rand() % 156))),
      cv_Pos(),
      cv_UVs(),
      cv_Width(0),
      cv_Height(0),
      SubdivsHorz(SubdivsHorz_),
      SubdivsVert(SubdivsVert_),
      NeedsUpdate(true),
      SurfaceInfo(),
      LMM(LMM_),
      BPRenderMesh(new cf::SceneGraph::BezierPatchNodeT(LMM, MAX_CURVE_ERROR)),
      CollModel(NULL),
      Material(NULL)
{
    SetMaterial(Material_);
}


MapBezierPatchT::MapBezierPatchT(const MapBezierPatchT& BP)
    : MapPrimitiveT(BP),
      cv_Pos(BP.cv_Pos),
      cv_UVs(BP.cv_UVs),
      cv_Width(BP.cv_Width),
      cv_Height(BP.cv_Height),
      SubdivsHorz(BP.SubdivsHorz),
      SubdivsVert(BP.SubdivsVert),
      NeedsUpdate(true),
      SurfaceInfo(BP.SurfaceInfo),
      LMM(BP.LMM),
      BPRenderMesh(new cf::SceneGraph::BezierPatchNodeT(LMM, MAX_CURVE_ERROR)),
      CollModel(NULL),
      Material(BP.Material)
{
    // UpdateRenderMesh();      // No need to call this here.
}


MapBezierPatchT::~MapBezierPatchT()
{
    delete CollModel;
    delete BPRenderMesh;
}


MapBezierPatchT* MapBezierPatchT::CreateSimplePatch(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long width, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(width, height);

    float sx = fabs(max.x - min.x)/(float)(BP->cv_Width - 1);
    float sy = -(fabs(max.y - min.y)/(float)(BP->cv_Height - 1));
    float px, py, pz;

    px = min.x;
    py = max.y;
    pz = max.z - ((max.z-min.z)*0.5);

    for (unsigned long y=0; y<BP->cv_Height; y++)
    {
        for (unsigned long x=0; x<BP->cv_Width; x++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));
            px += sx;
        }
        px = min.x;
        py += sy;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreatePatchCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, height);

    // init the points
    double stepSizeX = (max.x - min.x) / 2;
    double stepSizeY = (max.y - min.y) / 2;
    double stepSizeZ = (max.z - min.z) / (BP->cv_Height - 1);

    // define from left/center/top
    double px;
    double py;
    double pz = max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x == 2 || x == 6)
            px = min.x + stepSizeX;
        else if (x == 3 || x == 4 || x == 5)
            px = max.x;
        else
            px = min.x;

        if (x == 1 || x == 2 || x == 3)
            py = min.y;
        else if (x == 5 || x == 6 || x == 7)
            py = max.y;
        else
            py = min.y + stepSizeY;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz -= stepSizeZ;
        }
        pz = max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateSquareCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, height);

    // init the points
    double stepSizeX = (max.x - min.x) / 2;
    double stepSizeY = (max.y - min.y) / 2;
    double stepSizeZ = (max.z - min.z) / (BP->cv_Height - 1);

    // define from left/center/top
    double px;
    double py;
    double pz = max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x == 1 || x == 5)
            px = min.x + stepSizeX;
        else if (x == 2 || x == 3 || x == 4)
            px = max.x;
        else
            px = min.x;

        if (x == 3 || x == 7)
            py = min.y + stepSizeY;
        else if (x == 4 || x == 5 || x == 6)
            py = max.y;
        else
            py = min.y;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz -= stepSizeZ;
        }
        pz = max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateQuarterCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(3, height);

    // Init the stepsize in z direction.
    double stepSizeZ=(max.z-min.z)/(BP->cv_Height-1);

    // Vector parts of each vertex.
    double px;
    double py;
    double pz=max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x==0)
            px=min.x;
        else
            px=max.x;

        if (x==2)
            py=max.y;
        else
            py=min.y;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz-=stepSizeZ;
        }
        pz=max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateHalfCylinder(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(5, height);

    // Initialize step size in all axis directions.
    double stepSizeX=(max.x - min.x)/2;
    double stepSizeZ=(max.z - min.z)/(BP->cv_Height - 1);

    // Control vertex components.
    double px;
    double py;
    double pz=max.z;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x==2)
            px=min.x + stepSizeX;
        else if (x==3 || x==4)
            px=max.x;
        else
            px=min.x;

        if (x==1 || x==2 || x==3)
            py=min.y;
        else
            py=max.y;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));

            pz-=stepSizeZ;
        }
        pz=max.z;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateEdgePipe(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, 3);

    // Init the steps.
    double stepSizeX=(max.x - min.x)/2;
    double stepSizeY=(max.y - min.y)/2;
    double stepSizeZ=(max.z - min.z)/(BP->cv_Height - 1);

    // Vector parts of each control vertex.
    double px;
    double py;
    double pz;

    for (unsigned long x=0; x<BP->cv_Width; x++)
    {
        if (x==1 || x==2 || x==3)
            py=min.y;
        else if (x==5 || x==6 || x==7)
            py=max.y;
        else
            py=min.y+stepSizeY;

        for (unsigned long y=0; y<BP->cv_Height; y++)
        {
            if (x==0 || x==1 || x==7 || x==8)
            {
                if      (y==0) { px=max.x; pz=max.z; }
                else if (y==1) { px=min.x; pz=max.z; }
                else           { px=min.x; pz=min.z; }
            }
            else if (x==2 || x==6)
            {
                if      (y==0) { px=max.x;             pz=max.z-stepSizeZ/2; }
                else if (y==1) { px=min.x+stepSizeX/2; pz=max.z-stepSizeZ/2; }
                else           { px=min.x+stepSizeX/2; pz=min.z; }
            }
            else
            {
                if      (y==0) { px=max.x;           pz=max.z-stepSizeZ; }
                else if (y==1) { px=min.x+stepSizeX; pz=max.z-stepSizeZ; }
                else           { px=min.x+stepSizeX; pz=min.z; }
            }
            BP->SetCvPos(x, y, Vector3fT(px, py, pz));
        }
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateCone(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, unsigned long height, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, height);

    // Init the steps.
    double stepSizeX=(max.x - min.x)/2/(BP->cv_Height-1);
    double stepSizeY=(max.y - min.y)/2/(BP->cv_Height-1);
    double stepSizeZ=(max.z - min.z)  /(BP->cv_Height-1);

    // Vector parts of each vertex.
    double px;
    double py;
    double pz=max.z;

    for (unsigned long y=0; y<BP->cv_Height; y++)
    {
        for (unsigned long x=0; x<BP->cv_Width; x++)
        {
            if (x==2 || x==6)
                px=min.x+stepSizeX*(BP->cv_Height-1);
            else if (x==3 || x==4 || x==5)
                px=max.x-stepSizeX*(BP->cv_Height-1-y);
            else
                px=min.x+stepSizeX*(BP->cv_Height-1-y);

            if (x==1 || x==2 || x==3)
                py=min.y+stepSizeY*(BP->cv_Height-1-y);
            else if (x==5 || x==6 || x==7)
                py=max.y-stepSizeY*(BP->cv_Height-1-y);
            else
                py=min.y+stepSizeY*(BP->cv_Height-1);

            BP->SetCvPos(x, y, Vector3fT(px, py, pz));
        }

        pz -= stepSizeZ;
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateSphere(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(9, 5);

    double stepSizeX=(max.x - min.x)/2;
    double stepSizeY=(max.y - min.y)/2;
    double stepSizeZ=(max.z - min.z)/2;

    Vector3fT destination; // Destination vector, that is used to position every vertex of the bezier patch.
    Vector3fT top        (min.x+stepSizeX, min.y+stepSizeY, max.z);
    Vector3fT bottom     (min.x+stepSizeX, min.y+stepSizeY, min.z);

    for (int y=0; y<5; y++)
    {
        for (int x=0; x<9; x++)
        {
            if (y==0)
            {
                destination=top;
            }

            if (y==1 || y==2 || y==3)
            {
                destination.z=max.z-(y-1)*stepSizeZ;

                if (x == 2 || x == 6)
                    destination.x=min.x+stepSizeX;
                else if (x == 3 || x == 4 || x == 5)
                    destination.x=max.x;
                else
                    destination.x=min.x;

                if (x == 1 || x == 2 || x == 3)
                    destination.y=min.y;
                else if (x == 5 || x == 6 || x == 7)
                    destination.y=max.y;
                else
                    destination.y=min.y+stepSizeY;
            }

            if (y==4)
            {
                destination=bottom;
            }

            BP->SetCvPos(x, y, destination);
        }
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateQuarterDisc(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_, EndCapPosE pos, bool Inverted)
{
    MapBezierPatchT* BP=new MapBezierPatchT(Material_, LMM_, SubdivsHorz_, SubdivsVert_);

    BP->SetSize(3, 3);

    switch (pos)
    {
        case TOP_RIGHT:
        {
            const Vector3fT Center=Vector3fT(min.x, min.y, max.z);

            BP->SetCvPos(2 /*0*/, 2,                     Vector3fT(min.x, max.y, max.z));
            BP->SetCvPos(1 /*1*/, 2, Inverted ? Center : Vector3fT(max.x, max.y, max.z));
            BP->SetCvPos(0 /*2*/, 2,                     Vector3fT(max.x, min.y, max.z));

            BP->SetCvPos(0, 1, Center);
            BP->SetCvPos(1, 1, Center);
            BP->SetCvPos(2, 1, Center);

            BP->SetCvPos(0, 0, Center);
            BP->SetCvPos(1, 0, Center);
            BP->SetCvPos(2, 0, Center);
            break;
        }

        case TOP_LEFT:
        {
            const Vector3fT Center=Vector3fT(max.x, min.y, max.z);

            BP->SetCvPos(2 /*0*/, 2,                     Vector3fT(min.x, min.y, max.z));
            BP->SetCvPos(1 /*1*/, 2, Inverted ? Center : Vector3fT(min.x, max.y, max.z));
            BP->SetCvPos(0 /*2*/, 2,                     Vector3fT(max.x, max.y, max.z));

            BP->SetCvPos(0, 1, Center);
            BP->SetCvPos(1, 1, Center);
            BP->SetCvPos(2, 1, Center);

            BP->SetCvPos(0, 0, Center);
            BP->SetCvPos(1, 0, Center);
            BP->SetCvPos(2, 0, Center);
            break;
        }

        case BOTTOM_RIGHT:
        {
            const Vector3fT Center=Vector3fT(min.x, max.y, max.z);

            BP->SetCvPos(0, 2, Center);
            BP->SetCvPos(1, 2, Center);
            BP->SetCvPos(2, 2, Center);

            BP->SetCvPos(0, 1, Center);
            BP->SetCvPos(1, 1, Center);
            BP->SetCvPos(2, 1, Center);

            BP->SetCvPos(2 /*0*/, 0,                     Vector3fT(min.x, min.y, max.z));
            BP->SetCvPos(1 /*1*/, 0, Inverted ? Center : Vector3fT(max.x, min.y, max.z));
            BP->SetCvPos(0 /*2*/, 0,                     Vector3fT(max.x, max.y, max.z));
            break;
        }

        case BOTTOM_LEFT:
        {
            const Vector3fT Center=Vector3fT(max.x, max.y, max.z);

            BP->SetCvPos(0, 2, Center);
            BP->SetCvPos(1, 2, Center);
            BP->SetCvPos(2, 2, Center);

            BP->SetCvPos(0, 1, Center);
            BP->SetCvPos(1, 1, Center);
            BP->SetCvPos(2, 1, Center);

            BP->SetCvPos(2 /*0*/, 0,                     Vector3fT(min.x, max.y, max.z));
            BP->SetCvPos(1 /*1*/, 0, Inverted ? Center : Vector3fT(min.x, min.y, max.z));
            BP->SetCvPos(0 /*2*/, 0,                     Vector3fT(max.x, min.y, max.z));
            break;
        }
    }

    BP->SurfaceInfo.TexCoordGenMode=MatFit;
    BP->UpdateTextureSpace();

    return BP;
}


MapBezierPatchT* MapBezierPatchT::CreateConcaveEndcap(EditorMaterialI* Material_, cf::SceneGraph::LightMapManT& LMM_, const Vector3fT& min, const Vector3fT& max, int SubdivsHorz_, int SubdivsVert_, EndCapPosE pos)
{
    // TODO: This method should be removed entirely.
    // Instead, the user code should call CreateQuarterDisc() with Inverted=true.
    switch (pos)
    {
        case TOP_RIGHT:    pos=BOTTOM_LEFT;  break;
        case TOP_LEFT:     pos=BOTTOM_RIGHT; break;
        case BOTTOM_RIGHT: pos=TOP_LEFT;     break;
        case BOTTOM_LEFT:  pos=TOP_RIGHT;    break;
    }

    return CreateQuarterDisc(Material_, LMM_, min, max, SubdivsHorz_, SubdivsVert_, pos, true);
}


MapBezierPatchT* MapBezierPatchT::Clone() const
{
    return new MapBezierPatchT(*this);
}


BoundingBox3fT MapBezierPatchT::GetBB() const
{
    BoundingBox3fT BB;

    // TODO: Cache!
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            BB+=GetCvPos(x, y);

    return BB;
}


bool MapBezierPatchT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    UpdateRenderMesh();

    // If possible (RayOrigin is not in cull bounding-box), do a quick bounding-box check first.
    if (!GetBB().Contains(RayOrigin) && !MapPrimitiveT::TraceRay(RayOrigin, RayDir, Fraction, FaceNr)) return false;

    const static cf::ClipSys::TracePointT Point;
    const double              RayLength=100000.0;
    cf::ClipSys::TraceResultT Result(1.0);

    wxASSERT(CollModel!=NULL);      // Made sure by UpdateRenderMesh().
    CollModel->TraceConvexSolid(Point, RayOrigin.AsVectorOfDouble(), RayDir.AsVectorOfDouble()*RayLength, MaterialT::ClipFlagsT(0xFFFFFFFF), Result);

    if (Result.Fraction==1.0) return false;

    Fraction=float(Result.Fraction*RayLength);
    return true;
}


bool MapBezierPatchT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    const wxRect         Disc=wxRect(Pixel, Pixel).Inflate(Radius, Radius);
    const BoundingBox3fT BB  =GetBB();

    // 1. Do a quick preliminary bounding-box check:
    // Determine if this map elements BB intersects the Disc (which is actually rectangular...).
    if (!wxRect(ViewWin.WorldToWindow(BB.Min), ViewWin.WorldToWindow(BB.Max)).Intersects(Disc)) return false;

    // 2. Check the center X handle.
    if (Disc.Contains(ViewWin.WorldToWindow(BB.GetCenter()))) return true;

    // 3. If we select map elements by center handle only, then this brush was not hit.
    if (Options.view2d.SelectByHandles) return false;

    // 4. Check all edges of the patch mesh for a hit.
    unsigned long xstep=(GetRenderWidth ()-1)/(cv_Width -1);
    unsigned long ystep=(GetRenderHeight()-1)/(cv_Height-1);

    // Make sure we don't get into an infinite loop.
    // E.g. RenderInfo.width==2 and cv_Width==3 *is* possible!
    if (xstep==0) xstep=1;
    if (ystep==0) ystep=1;

    for (unsigned long y=0; y<GetRenderHeight(); y+=ystep)
    {
        for (unsigned long x=0; x+1<GetRenderWidth(); x++)
        {
            const wxPoint Vertex1=ViewWin.WorldToWindow(GetRenderVertexPos(x,   y));
            const wxPoint Vertex2=ViewWin.WorldToWindow(GetRenderVertexPos(x+1, y));

            if (ViewWindow2DT::RectIsIntersected(Disc, Vertex1, Vertex2))
                return true;
        }
    }

    for (unsigned long y=0; y+1<GetRenderHeight(); y++)
    {
        for (unsigned long x=0; x<GetRenderWidth(); x+=xstep)
        {
            const wxPoint Vertex1=ViewWin.WorldToWindow(GetRenderVertexPos(x, y));
            const wxPoint Vertex2=ViewWin.WorldToWindow(GetRenderVertexPos(x, y+1));

            if (ViewWindow2DT::RectIsIntersected(Disc, Vertex1, Vertex2))
                return true;
        }
    }

    // 5. Despite all efforts we found no hit.
    return false;
}


bool MapBezierPatchT::IsTranslucent() const
{
    if (Material==NULL) return false;

    return Material->IsTranslucent();
}


void MapBezierPatchT::Render2D(Renderer2DT& Renderer) const
{
    UpdateRenderMesh();

    const wxColour Color=IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors);
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, Color);
    Renderer.SetFillColor(Color);

    unsigned long xstep=(GetRenderWidth ()-1)/(cv_Width -1);
    unsigned long ystep=(GetRenderHeight()-1)/(cv_Height-1);

    // Make sure we don't get into an infinite loop.
    // E.g. RenderInfo.width==2 and cv_Width==3 *is* possible!
    if (xstep==0) xstep=1;
    if (ystep==0) ystep=1;

    for (unsigned long y=0; y<GetRenderHeight(); y+=ystep)
    {
        wxPoint Cur=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(0, y));

        for (unsigned long x=1; x<GetRenderWidth(); x++)
        {
            wxPoint Next=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(x, y));

            Renderer.DrawLine(Cur, Next);
            Cur=Next;
        }
    }

    for (unsigned long x=0; x<GetRenderWidth(); x+=xstep)
    {
        wxPoint Cur=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(x, 0));

        for (unsigned long y=1; y<GetRenderHeight(); y++)
        {
            wxPoint Next=Renderer.GetViewWin2D().WorldToTool(GetRenderVertexPos(x, y));

            Renderer.DrawLine(Cur, Next);
            Cur=Next;
        }
    }

    // Render the center X handle.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(150, 150, 150));
    Renderer.XHandle(Renderer.GetViewWin2D().WorldToTool(GetBB().GetCenter()));
}


void MapBezierPatchT::Render3D_Basic(MatSys::RenderMaterialT* RenderMat, const wxColour& MeshColor, const int MeshAlpha) const
{
    UpdateRenderMesh();

    BPRenderMesh->UpdateMeshColor(MeshColor.Red()/255.0, MeshColor.Green()/255.0, MeshColor.Blue()/255.0, MeshAlpha/255.0);

    // Backup previous rendermaterial and assign new one.
    MatSys::RenderMaterialT* tmp=BPRenderMesh->RenderMaterial;
    BPRenderMesh->RenderMaterial=RenderMat;

    // "Empty" vector is submitted, because this Vector isn't used in the method.
    BPRenderMesh->DrawAmbientContrib(Vector3dT());

    // Reload old previous render material.
    BPRenderMesh->RenderMaterial=tmp;
}


void MapBezierPatchT::Render3D(Renderer3DT& Renderer) const
{
    const ViewWindow3DT& ViewWin=Renderer.GetViewWin3D();

    switch (ViewWin.GetViewType())
    {
        case ViewWindowT::VT_3D_EDIT_MATS:
            // Note that the mesh color is ignored for most normal materials anyway... (they don't have the "useMeshColors" property).
            Render3D_Basic(Material!=NULL ? Material->GetRenderMaterial(false) : Renderer.GetRMatFlatShaded(), *wxWHITE, 255);
            break;

        case ViewWindowT::VT_3D_FULL_MATS:
            // Note that the mesh color is ignored for most normal materials anyway... (they don't have the "useMeshColors" property).
            Render3D_Basic(Material!=NULL ? Material->GetRenderMaterial(true) : Renderer.GetRMatFlatShaded(), *wxWHITE, 255);
            break;

        case ViewWindowT::VT_3D_LM_GRID:
        case ViewWindowT::VT_3D_LM_PREVIEW:
            // The concept of lightmaps does not really apply to bezier patches...
            Render3D_Basic(Renderer.GetRMatFlatShaded(), IsSelected() ? Options.colors.SelectedFace : *wxWHITE, 255);
            break;

        case ViewWindowT::VT_3D_FLAT:
            Render3D_Basic(Renderer.GetRMatFlatShaded(), IsSelected() ? Options.colors.SelectedFace : GetColor(Options.view2d.UseGroupColors), 255);
            break;

        case ViewWindowT::VT_3D_WIREFRAME:
            Render3D_Basic(Renderer.GetRMatWireframe(), IsSelected() ? Options.colors.SelectedEdge : GetColor(Options.view2d.UseGroupColors), 255);
            break;

        default:
            wxASSERT(0);
            break;
    }

    // If every MapElementT had it's pointer to the map document struct like this one wouldn't be necessary. For now it's the
    // only possibility to get to the surface dialog that holds information whether selected bezier patches should be rendered
    // with a selection mask or the toolmanager to check whether the surface editing tool is active.
    if (IsSelected())
        if (   ViewWin.GetMapDoc().GetChildFrame()->GetToolManager().GetActiveToolType()!=&ToolEditSurfaceT::TypeInfo
            || ViewWin.GetMapDoc().GetChildFrame()->GetSurfacePropsDialog()->WantSelectionOverlay())
            if ((ViewWin.GetViewType()==ViewWindowT::VT_3D_EDIT_MATS || ViewWin.GetViewType()==ViewWindowT::VT_3D_FULL_MATS))
            {
                Render3D_Basic(Renderer.GetRMatOverlay(), Options.colors.SelectedFace, 64);
                Render3D_Basic(Renderer.GetRMatWireframe_OffsetZ(), wxColour(255, 255, 0), 255);
            }
}


void MapBezierPatchT::SetMaterial(EditorMaterialI* Material_)
{
    Material = Material_;
}


void MapBezierPatchT::SetSurfaceInfo(const SurfaceInfoT& SI)
{
    SurfaceInfo=SI;
    UpdateTextureSpace();
}


void MapBezierPatchT::InvertPatch()
{
    // Loop over all rows of the matrix and invert each row.
    for (unsigned long y=0; y<cv_Height; y++)
    {
        for (unsigned long x=0; x<cv_Width/2; x++)
        {
            // Switch CVs.
            const Vector3fT cv_pos_tmp=GetCvPos(x, y);
            SetCvPos(x, y, GetCvPos(cv_Width-1-x, y));
            SetCvPos(cv_Width-1-x, y, cv_pos_tmp);
        }
    }
}


void MapBezierPatchT::UpdateTextureSpace()
{
    if (SurfaceInfo.TexCoordGenMode==PlaneProj) // Project material onto patch using UV axis.
    {
        for (unsigned long y=0; y<GetHeight(); y++)
        {
            for (unsigned long x=0; x<GetWidth(); x++)
            {
                const Vector3fT& cv_xyz=GetCvPos(x, y);
                Vector3fT cv_uvw;

                cv_uvw.x=dot(SurfaceInfo.UAxis, cv_xyz)*SurfaceInfo.Scale[0]+SurfaceInfo.Trans[0];
                cv_uvw.y=dot(SurfaceInfo.VAxis, cv_xyz)*SurfaceInfo.Scale[1]+SurfaceInfo.Trans[1];
                cv_uvw.z=0.0f;

                SetCvUV(x, y, cv_uvw);
            }
        }
    }

    if (SurfaceInfo.TexCoordGenMode==MatFit) // Calculate texture coordinates using BP specific MaterialFit mode.
    {
        if (cv_Width < 3 || cv_Height < 3)
            return;

        const float xs = 1.0f/(cv_Width-1);
        const float ys = 1.0f/(cv_Height-1);

        for (unsigned long y=0; y<cv_Height; y++)
        {
            for (unsigned int x=0; x<cv_Width; x++)
            {
                Vector3fT cv_uvw;

                cv_uvw.x=(x*xs)*SurfaceInfo.Scale[0]+SurfaceInfo.Trans[0];
                cv_uvw.y=(y*ys)*SurfaceInfo.Scale[1]+SurfaceInfo.Trans[1];
                cv_uvw.z=0.0f;

                cv_uvw=cv_uvw.GetRotZ(SurfaceInfo.Rotate);

                SetCvUV(x, y, cv_uvw);
            }
        }

        // Set projection plane vectors to zero, because they don't exist for this kind of "projection".
        SurfaceInfo.UAxis=Vector3fT();
        SurfaceInfo.VAxis=Vector3fT();
    }
}


namespace
{
    class BezierPatchTrafoMementoT : public TrafoMementoT
    {
        public:

        BezierPatchTrafoMementoT(const ArrayT<Vector3fT>& cv_Pos)
            : m_cv_Pos(cv_Pos)
        {
        }

        const ArrayT<Vector3fT> m_cv_Pos;
    };
}


TrafoMementoT* MapBezierPatchT::GetTrafoState() const
{
    return new BezierPatchTrafoMementoT(cv_Pos);
}


void MapBezierPatchT::RestoreTrafoState(const TrafoMementoT* TM)
{
    const BezierPatchTrafoMementoT* BezierPatchTM = dynamic_cast<const BezierPatchTrafoMementoT*>(TM);

    wxASSERT(BezierPatchTM);
    if (!BezierPatchTM) return;

    // Using ArrayT assignment would be shorter:
    //     cv_Pos = BezierPatchTM->m_cv_Pos;
    // but (as implemented at this time) also unnecessarily re-allocate all array elements as well.
    wxASSERT(cv_Pos.Size() == BezierPatchTM->m_cv_Pos.Size());
    for (unsigned int i = 0; i < cv_Pos.Size(); i++)
        cv_Pos[i] = BezierPatchTM->m_cv_Pos[i];

    NeedsUpdate = true;
}


void MapBezierPatchT::TrafoMove(const Vector3fT& Delta, bool LockTexCoords)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            SetCvPos(x, y, GetCvPos(x, y)+Delta);

    MapPrimitiveT::TrafoMove(Delta, LockTexCoords);
}


void MapBezierPatchT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
        {
            Vector3fT Vertex=GetCvPos(x, y)-RefPoint;

            if (Angles.x!=0.0f) Vertex=Vertex.GetRotX( Angles.x);
            if (Angles.y!=0.0f) Vertex=Vertex.GetRotY(-Angles.y);
            if (Angles.z!=0.0f) Vertex=Vertex.GetRotZ( Angles.z);

            SetCvPos(x, y, Vertex+RefPoint);
        }

    MapPrimitiveT::TrafoRotate(RefPoint, Angles, LockTexCoords);
}


void MapBezierPatchT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            SetCvPos(x, y, RefPoint + (GetCvPos(x, y)-RefPoint).GetScaled(Scale));

    MapPrimitiveT::TrafoScale(RefPoint, Scale, LockTexCoords);
}


void MapBezierPatchT::TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords)
{
    for (unsigned long y = 0; y < cv_Height; y++)
        for (unsigned long x = 0; x < cv_Width; x++)
        {
            Vector3fT v = GetCvPos(x, y);

            v[NormalAxis] = Dist - (v[NormalAxis] - Dist);

            SetCvPos(x, y, v);
        }

    InvertPatch();  // Need this so that textures don't get turned "inside out".

    MapPrimitiveT::TrafoMirror(NormalAxis, Dist, LockTexCoords);
}


void MapBezierPatchT::Transform(const MatrixT& Matrix, bool LockTexCoords)
{
    for (unsigned long y=0; y<cv_Height; y++)
        for (unsigned long x=0; x<cv_Width; x++)
            SetCvPos(x, y, Matrix.Mul1(GetCvPos(x, y)));

    MapPrimitiveT::Transform(Matrix, LockTexCoords);
}


bool MapBezierPatchT::ReconstructMatFit()
{
    const Vector3fT MinUV = GetCvUV(0, 0);
    const Vector3fT VecU  = GetCvUV(cv_Width - 1,  0) - MinUV;
    const Vector3fT VecV  = GetCvUV(0, cv_Height - 1) - MinUV;

    float tx, ty, sx, sy, r;

    if (fabs(VecU.x) > fabs(VecU.y))
    {
        // Ok, this is the normal case:
        // VecU grows along the x-axis (and VecV supposedly along the y-axis).
        tx = MinUV.x;
        ty = MinUV.y;
        sx = VecU.x;
        sy = VecV.y;
        r  = 0.0f;
    }
    else
    {
        // This is the special case:
        // In the cv_UVs, the u- and v-coordinates are *swapped*.
        tx = -MinUV.y;
        ty =  MinUV.x;
        sx = -VecU.y;
        sy =  VecV.x;
        r  = -90.0f;
    }

    wxLogDebug("MinUV (%f %f), VecU (%f %f), VecV (%f %f)", MinUV.x, MinUV.y, VecU.x, VecU.y, VecV.x, VecV.y);
    wxLogDebug("t (%f, %f);  s (%f, %f);  r %f", tx, ty, sx, sy, r);

    for (unsigned int y = 0; y < cv_Height; y++)
    {
        const float dy = ty + sy * y / (cv_Height - 1);

        for (unsigned int x = 0 ; x < cv_Width; x++)
        {
            const float      dx  = tx + sx * x / (cv_Width - 1);
            const Vector3fT& Is  = GetCvUV(x, y);
            const Vector3fT  Fit = Vector3fT(dx, dy, 0.0f).GetRotZ(r);

            wxLogDebug("%u %u -- Is (%f %f), Fit (%f %f)", x, y, Is.x, Is.y, Fit.x, Fit.y);

            if (fabs(Fit.x - Is.x) > 0.1f || fabs(Fit.y - Is.y) > 0.1f)
                return false;
        }
    }

    SurfaceInfo.TexCoordGenMode = MatFit;

    SurfaceInfo.Trans[0] = tx;
    SurfaceInfo.Trans[1] = ty;
    SurfaceInfo.Scale[0] = sx;
    SurfaceInfo.Scale[1] = sy;
    SurfaceInfo.Rotate   = r;

#ifdef DEBUG
    // Assert that the above produced indeed the expected results.
    const ArrayT<Vector3fT> PrevUVs = cv_UVs;

    UpdateTextureSpace();

    for (unsigned int i = 0; i < cv_UVs.Size(); i++)
        assert(fabs(cv_UVs[i].x - PrevUVs[i].x) < 0.1f &&
               fabs(cv_UVs[i].y - PrevUVs[i].y) < 0.1f);
#endif

    return true;
}


bool MapBezierPatchT::ReconstructPlaneProj(bool AltOrigin)
{
    wxLogDebug("");
    wxLogDebug("MatFit didn't work out, now try PlaneProj instead (%s origin).", AltOrigin ? "alternate" : "normal");

    // Patches may have several control vertices at the same coordinates.
    // Thus, if the vertex at (0, 0) doesn't work out, try the diagonally opposite one.
    const Vector3fT CvPos00 = AltOrigin ? GetCvPos(cv_Width - 1, cv_Height - 1) : GetCvPos(0, 0);
    const Vector3fT CvUV_00 = AltOrigin ? GetCvUV (cv_Width - 1, cv_Height - 1) : GetCvUV (0, 0);

    // This is quasi the same as computing the tangent and bi-tangent vectors for a model.
    // Thus, just reuse the same code here.
    //
    // Understanding what's going on here is easy. The key statement is
    // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
    // First, there is a short explanation in "The Cg Tutorial", chapter 8.
    // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
    const Vector3fT Edge01 = GetCvPos(cv_Width - 1,  0) - CvPos00;
    const Vector3fT Edge02 = GetCvPos(0, cv_Height - 1) - CvPos00;
    const Vector3fT uv01   = GetCvUV(cv_Width - 1,  0) - CvUV_00;
    const Vector3fT uv02   = GetCvUV(0, cv_Height - 1) - CvUV_00;
    const float     d      = uv01.x*uv02.y - uv01.y*uv02.x;

    if (fabs(d) < 0.01f) return false;

    const Vector3fT VecS = (scale(Edge02, -uv01.y) + scale(Edge01, uv02.y)) / d;
    const Vector3fT VecT = (scale(Edge02,  uv01.x) - scale(Edge01, uv02.x)) / d;

    const float LenS = length(VecS);
    const float LenT = length(VecT);

    wxLogDebug("VecS (%f %f %f), len %f", VecS.x, VecS.y, VecS.z, LenS);
    wxLogDebug("VecT (%f %f %f), len %f", VecT.x, VecT.y, VecT.z, LenT);

    if (LenS < 0.01f || LenT < 0.01f) return false;

    // UpdateTextureSpace() will use this code to compute the uv-coordinates:
    // cv_uvw.x = dot(SurfaceInfo.UAxis, cv_xyz) * SurfaceInfo.Scale[0] + SurfaceInfo.Trans[0];
    // cv_uvw.y = dot(SurfaceInfo.VAxis, cv_xyz) * SurfaceInfo.Scale[1] + SurfaceInfo.Trans[1];

    const float tx = CvUV_00.x - dot(VecS, CvPos00) / LenS / LenS;
    const float ty = CvUV_00.y - dot(VecT, CvPos00) / LenT / LenT;

    wxLogDebug("tx %f", tx);
    wxLogDebug("ty %f", ty);

    bool IsSuccess = true;

    for (unsigned int y = 0; y < cv_Height; y++)
    {
        for (unsigned int x = 0 ; x < cv_Width; x++)
        {
            const Vector3fT& cv_xyz = GetCvPos(x, y);
            const Vector3fT& cv_uv  = GetCvUV(x, y);

            const Vector3fT Fit(
                dot(VecS, cv_xyz) / LenS / LenS + tx,
                dot(VecT, cv_xyz) / LenT / LenT + ty,
                0.0f
            );

            const bool fail = fabs(Fit.x - cv_uv.x) > 0.1f || fabs(Fit.y - cv_uv.y) > 0.1f;

            wxLogDebug("%u %u -- Is (%f %f), Fit (%f %f), %s    -- xyz (%f %f %f)", x, y, cv_uv.x, cv_uv.y, Fit.x, Fit.y, fail ? "fail" : "ok  ", cv_xyz.x, cv_xyz.y, cv_xyz.z);

            if (fail)
                IsSuccess = false;
        }
    }

    if (!IsSuccess) return false;

    SurfaceInfo.TexCoordGenMode = PlaneProj;

    SurfaceInfo.Trans[0] = tx;
    SurfaceInfo.Trans[1] = ty;
    SurfaceInfo.Scale[0] = 1.0f / LenS;
    SurfaceInfo.Scale[1] = 1.0f / LenT;
    SurfaceInfo.Rotate   = 0.0f;
    SurfaceInfo.UAxis    = VecS / LenS;
    SurfaceInfo.VAxis    = VecT / LenT;

#ifdef DEBUG
    // Assert that the above produced indeed the expected results.
    const ArrayT<Vector3fT> PrevUVs = cv_UVs;

    UpdateTextureSpace();

    for (unsigned int i = 0; i < cv_UVs.Size(); i++)
        assert(fabs(cv_UVs[i].x - PrevUVs[i].x) < 0.1f &&
               fabs(cv_UVs[i].y - PrevUVs[i].y) < 0.1f);
#endif

    return true;
}


bool MapBezierPatchT::ReconstructSI()
{
    if (SurfaceInfo.TexCoordGenMode != Custom) return false;

    if (cv_Width  < 2) return false;
    if (cv_Height < 2) return false;

    wxLogDebug("");
    wxLogDebug("----");
    wxLogDebug(Material->GetName());

    // For debugging, output the uv-coordinates of the entire patch.
    for (unsigned int y = 0; y < cv_Height; y++)
    {
        const unsigned int y_ = cv_Height - y - 1;

        wxString line = wxString::Format("    %u:    ", y_);

        for (unsigned int x = 0 ; x < cv_Width; x++)
        {
            const Vector3fT& uv = GetCvUV(x, y_);

            line += wxString::Format("(%7.5f, %7.5f)    ", uv.x, uv.y);
        }

        wxLogDebug(line);
    }

    return ReconstructMatFit() || ReconstructPlaneProj(false) || ReconstructPlaneProj(true);
}


// This method is const, so that it can be called from other const methods.
void MapBezierPatchT::UpdateRenderMesh() const
{
    // If either control vertex coordinates or UV coordinates have changed, the mesh needs to be updated.
    if (NeedsUpdate)
    {
        // 1. Create a new render mesh.
        assert(BPRenderMesh!=NULL);
        delete BPRenderMesh;
        BPRenderMesh=new cf::SceneGraph::BezierPatchNodeT(LMM, cv_Width, cv_Height, cv_Pos, cv_UVs, SubdivsHorz, SubdivsVert, Material->GetMaterial(), MAX_CURVE_ERROR);


        // 2. Create a new collision model.
        cf::math::BezierPatchT<float> CollisionBP(cv_Width, cv_Height, cv_Pos);

        if (SubdivsHorz>0 && SubdivsVert>0)
        {
            // The mapper may have provided an explicit number of subdivisions in order to avoid gaps between adjacent bezier patches.
            // The casts to unsigned long are needed in order to resolve ambiguity of the overloaded Subdivide() method.
            CollisionBP.Subdivide((unsigned long)SubdivsHorz, (unsigned long)SubdivsVert);
        }
        else
        {
            CollisionBP.Subdivide(MAX_CURVE_ERROR, -1.0f);
        }

        // Get a collision model from the curve mesh.
        ArrayT<Vector3dT> CoordsOnly;

        // wxLogDebug("%s: Vertices of CollisionBP, width %lu, height %lu:", __FUNCTION__, CollisionBP.Width, CollisionBP.Height);
        for (unsigned long VertexNr=0; VertexNr<CollisionBP.Mesh.Size(); VertexNr++)
        {
            CoordsOnly.PushBack(CollisionBP.Mesh[VertexNr].Coord.AsVectorOfDouble());
            // wxLogDebug("    %2lu: %s", VertexNr, convertToString(CoordsOnly[VertexNr]));
        }

        // We cannot use our own Material->GetMaterial() here, because it might clip nothing (ClipFlags==0)
        // and thus would wrongly not be considered in TraceRay().
        static MaterialT s_CollMaterial;
        s_CollMaterial.ClipFlags=MaterialT::Clip_AllBlocking;

        const double COLLISION_MODEL_MIN_NODE_SIZE = 40.0;

        delete CollModel;
        CollModel = new cf::ClipSys::CollisionModelStaticT(CollisionBP.Width, CollisionBP.Height, CoordsOnly, &s_CollMaterial, COLLISION_MODEL_MIN_NODE_SIZE);


        // 3. Everything updated for now.
        NeedsUpdate=false;
    }
}


void MapBezierPatchT::SetSize(unsigned long width, unsigned long height)
{
    if (cv_Width==width && cv_Height==height) return;

    cv_Width =width;
    cv_Height=height;

    const unsigned long Total=cv_Width*cv_Height;

    cv_Pos.Clear(); cv_Pos.PushBackEmpty(Total);
    cv_UVs.Clear(); cv_UVs.PushBackEmpty(Total);

    if (Total>0)
    {
        // I don't think that we really need this, becaue a call to this method should *always* be followed
        // by calls to SetCvPos() and SetCvUV(), but who ever knows...
        SetCvPos(0, 0, Vector3fT());
        SetCvUV (0, 0, Vector3fT());
    }
}


Vector3fT MapBezierPatchT::GetRenderVertexPos(unsigned long x, unsigned long y) const
{
    assert(BPRenderMesh->Meshes.Size()>0);
    assert(x<=BPRenderMesh->Meshes.Size());
    assert(y<BPRenderMesh->Meshes[0]->Vertices.Size()/2);

    unsigned long c=0;

    // Special case when accessing the render meshes last line of vertices
    if (x==BPRenderMesh->Meshes.Size()) c=1;

    Vector3fT VertexPos;

    VertexPos.x=BPRenderMesh->Meshes[x-c]->Vertices[2*y+c].Origin[0];
    VertexPos.y=BPRenderMesh->Meshes[x-c]->Vertices[2*y+c].Origin[1];
    VertexPos.z=BPRenderMesh->Meshes[x-c]->Vertices[2*y+c].Origin[2];

    return VertexPos;
}
