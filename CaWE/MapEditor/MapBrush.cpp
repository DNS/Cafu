/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MapBrush.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "../EditorMaterial.hpp"
#include "../Options.hpp"

#include "Math3D/Matrix.hpp"
#include "Math3D/Misc.hpp"
#include "Math3D/Polygon.hpp"
#include "TypeSys.hpp"

#include "wx/wx.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapBrushT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapBrushT::TypeInfo(GetMapElemTIM(), "MapBrushT", "MapPrimitiveT", MapBrushT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapBrushT::MapBrushT()
    : MapPrimitiveT(wxColour(0, 100 + (rand() % 156), 100 + (rand() % 156))),
      m_Faces()
{
}


MapBrushT::MapBrushT(const ArrayT<Vector3fT>& HullVertices, EditorMaterialI* Material, bool FaceAligned, const MapBrushT* RefBrush)
    : MapPrimitiveT(wxColour(0, 100 + (rand() % 156), 100 + (rand() % 156))),
      m_Faces()
{
    // wxLogDebug("### MapBrushT(HullVertices)  -  create new brush from hull vertices.");
    // for (unsigned long VertexNr=0; VertexNr<HullVertices.Size(); VertexNr++)
    //     wxLogDebug("    Vertex %lu: %s", VertexNr, convertToString(HullVertices[VertexNr]).c_str());

    ArrayT<Plane3fT>      HullPlanes;
    ArrayT<unsigned long> HullPlanesPIs;
    Plane3fT::ConvexHull(HullVertices, HullPlanes, &HullPlanesPIs, 0.1f);

    // for (unsigned long PlaneNr=0; PlaneNr<HullPlanes.Size(); PlaneNr++)
    //     wxLogDebug("    Plane %lu: %s", PlaneNr, convertToString(HullPlanes[PlaneNr]).c_str());

    for (unsigned long PlaneNr=0; PlaneNr<HullPlanes.Size(); PlaneNr++)
    {
        const Vector3fT HullPlanePoints[3]={ HullVertices[HullPlanesPIs[3*PlaneNr+0]],
                                             HullVertices[HullPlanesPIs[3*PlaneNr+1]],
                                             HullVertices[HullPlanesPIs[3*PlaneNr+2]] };

        // If we were given a reference brush, find the face that is closest to the current hull plane.
        // BestDot is intentionally initialized at 0.01. If there is no closer face in RefBrush, we're better off without the rest!
        float           BestDot =0.01f;
        const MapFaceT* BestFace=NULL;

        if (RefBrush)
        {
            for (unsigned long FNr=0; FNr<RefBrush->m_Faces.Size(); FNr++)
            {
                const MapFaceT& RefFace=RefBrush->m_Faces[FNr];
                const float     TestDot=dot(HullPlanes[PlaneNr].Normal, RefFace.GetPlane().Normal);

                if (TestDot>BestDot)
                {
                    BestDot =TestDot;
                    BestFace=&RefFace;
                }
            }
        }

        if (BestFace)
        {
            m_Faces.PushBack(MapFaceT(BestFace->m_Material, HullPlanes[PlaneNr], HullPlanePoints, FaceAligned));

            m_Faces[m_Faces.Size()-1].m_SurfaceInfo=BestFace->GetSurfaceInfo();
        }
        else
        {
            m_Faces.PushBack(MapFaceT(Material, HullPlanes[PlaneNr], HullPlanePoints, FaceAligned));
        }
    }

    CompleteFaceVertices();

    // for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    // {
    //     const MapFaceT& F=m_Faces[FaceNr];
    //     const Plane3fT  TestPlane(F.m_PlanePoints[0], F.m_PlanePoints[1], F.m_PlanePoints[2], 0.1f);
    //
    //     wxLogDebug("    Face %lu: Plane: %s", FaceNr, convertToString(F.m_Plane).c_str());
    //     wxLogDebug("            TestPln: %s   %s", convertToString(TestPlane).c_str(), (F.m_Plane.Normal==TestPlane.Normal && F.m_Plane.Dist==TestPlane.Dist) ? "" : "####### != #######");
    //     wxLogDebug("            PlPts: %s, %s, %s", convertToString(F.m_PlanePoints[0]).c_str(), convertToString(F.m_PlanePoints[1]).c_str(), convertToString(F.m_PlanePoints[2]).c_str());
    //     wxLogDebug("            %lu Vertices:", F.m_Vertices.Size());
    //
    //     for (unsigned long VertexNr=0; VertexNr<F.m_Vertices.Size(); VertexNr++)
    //         wxLogDebug("                %s", convertToString(F.m_Vertices[VertexNr]).c_str());
    // }
}


MapBrushT::MapBrushT(const ArrayT<Plane3fT>& Planes, EditorMaterialI* Material, bool FaceAligned)
    : MapPrimitiveT(wxColour(0, 100 + (rand() % 156), 100 + (rand() % 156))),
      m_Faces()
{
    for (unsigned long PlaneNr=0; PlaneNr<Planes.Size(); PlaneNr++)
        m_Faces.PushBack(MapFaceT(Material, Planes[PlaneNr], NULL, FaceAligned));

    CompleteFaceVertices();
}


MapBrushT::MapBrushT(const MapBrushT& Brush)
    : MapPrimitiveT(Brush),
      m_Faces(Brush.m_Faces)
{
}


/// Computes the points of an ellipse that is defined by the XY-plane rectangle described by the given BB.
/// @param BB           The bounding-box that defines the dimensions of the ellipse.
/// @param NrOfPoints   The number of points that are to be created for the ellipse approximation.
static ArrayT<Vector3fT> ComputeEllipse(const BoundingBox3fT& BB, unsigned long NrOfPoints)
{
    const Vector3dT   Center    =BB.GetCenter().AsVectorOfDouble();
    const Vector3dT   Radius    =BB.Max.AsVectorOfDouble()-Center;
    const double      DeltaAngle=cf::math::AnglesdT::DegToRad(360.0/double(NrOfPoints));
    ArrayT<Vector3fT> Points;

    for (unsigned long PointNr=0; PointNr<NrOfPoints; PointNr++)
    {
        const double Angle=DeltaAngle*PointNr;

        Points.PushBack(Vector3dT(cf::math::round(Center.x + sin(Angle)*Radius.x),
                                  cf::math::round(Center.y + cos(Angle)*Radius.y),
                                  BB.Min.z).AsVectorOfFloat());
    }

    return Points;
}


MapBrushT* MapBrushT::CreateBlock(const BoundingBox3fT& Box, EditorMaterialI* Material)
{
    ArrayT<Vector3fT> HullVertices;

    HullVertices.PushBackEmpty(8);
    Box.GetCornerVertices(&HullVertices[0]);

    return new MapBrushT(HullVertices, Material, Options.general.NewUVsFaceAligned);
}


MapBrushT* MapBrushT::CreateWedge(const BoundingBox3fT& Box, EditorMaterialI* Material)
{
    ArrayT<Vector3fT> HullVertices;

    HullVertices.PushBackEmpty(8);
    Box.GetCornerVertices(&HullVertices[0]);

    // Remove the two vertices of the upper left corner.
    // The vertex with the higher index number is removed first, in order to avoid implicit renumbering (prone to confusion!).
    HullVertices.RemoveAt(3);
    HullVertices.RemoveAt(2);

    return new MapBrushT(HullVertices, Material, Options.general.NewUVsFaceAligned);
}


MapBrushT* MapBrushT::CreateCylinder(const BoundingBox3fT& Box, const unsigned long NrOfSides, EditorMaterialI* Material)
{
    ArrayT<Vector3fT> HullVertices=ComputeEllipse(Box, NrOfSides);

    // Repeat the points in the "upper" location.
    for (unsigned long VertexNr=0; VertexNr<NrOfSides; VertexNr++)
    {
        Vector3fT Vertex=HullVertices[VertexNr];
        Vertex.z=Box.Max.z;

        HullVertices.PushBack(Vertex);
    }

    return new MapBrushT(HullVertices, Material, Options.general.NewUVsFaceAligned);
}


MapBrushT* MapBrushT::CreatePyramid(const BoundingBox3fT& Box, const unsigned long NrOfSides, EditorMaterialI* Material)
{
    ArrayT<Vector3fT> HullVertices=ComputeEllipse(Box, NrOfSides);

    // Add the tip.
    HullVertices.PushBack(Box.GetCenter());
    HullVertices[HullVertices.Size()-1].z=cf::math::round(Box.Max.z);

    return new MapBrushT(HullVertices, Material, Options.general.NewUVsFaceAligned);
}


MapBrushT* MapBrushT::CreateSphere(const BoundingBox3fT& Box, const unsigned long NrOfSides, EditorMaterialI* Material)
{
    const Vector3dT   Center    =Box.GetCenter().AsVectorOfDouble();
    const Vector3dT   Radius    =Box.Max.AsVectorOfDouble()-Center;
    const Vector3fT   NorthPole =cf::math::round(Vector3fT(Center.x, Center.y, Box.Max.z));
    const Vector3fT   SouthPole =cf::math::round(Vector3fT(Center.x, Center.y, Box.Min.z));
    const double      DeltaAngle=cf::math::AnglesdT::DegToRad(360.0/double(NrOfSides));
    ArrayT<Vector3fT> HullVertices;

    // Examined from the side, a sphere with S sides has a north pole and floor(S/2) circles of latitude (Breitenkreise),
    // with an elevation of 360°/S between each. Note that when S is even, the last circle of latitude is degenerate at the south pole.
    HullVertices.PushBack(NorthPole);

    for (unsigned long BreitenkreisNr=0; BreitenkreisNr<NrOfSides/2; BreitenkreisNr++)
    {
        const double theta=DeltaAngle*(BreitenkreisNr+1);   // Elevation, 0°...180° from positive z-axis (not -90°...+90° from equator plane).

        if (2*(BreitenkreisNr+1)==NrOfSides)
        {
            HullVertices.PushBack(SouthPole);
            break;
        }

        for (unsigned long SliceNr=0; SliceNr<NrOfSides; SliceNr++)
        {
            const double    phi   =DeltaAngle*SliceNr;      // Heading/azimut, 0°...360° from positive y-axis.
            const Vector3dT Vertex=Center + Vector3dT(sin(theta)*sin(phi), sin(theta)*cos(phi), cos(theta)).GetScaled(Radius);

            // Can unfortunately not round Vertex here, because rounding makes the quad surfaces become non-coplanar!
            HullVertices.PushBack(Vertex.AsVectorOfFloat());
        }
    }

    return new MapBrushT(HullVertices, Material, Options.general.NewUVsFaceAligned);
}


MapBrushT* MapBrushT::Clone() const
{
    return new MapBrushT(*this);
}


void MapBrushT::Render2D(Renderer2DT& Renderer) const
{
    const wxColour LineColor=IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors);

    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        const MapFaceT& Face=m_Faces[FaceNr];

        for (unsigned long j=1; j<=Face.GetVertices().Size(); j++)
        {
            wxPoint Cur =Renderer.GetViewWin2D().WorldToTool(Face.GetVertices()[(j-1)                        ]);
            wxPoint Next=Renderer.GetViewWin2D().WorldToTool(Face.GetVertices()[j % Face.GetVertices().Size()]);

            if (Options.view2d.DrawVertices && j!=Face.GetVertices().Size())
            {
                // Highlight each vertex with a very small (3x3 pixels) background rectangle (usually in white color).
                Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, Options.colors.Vertex);
                Renderer.SetFillColor(Options.colors.Vertex);
                Renderer.Rectangle(wxRect(Cur-wxPoint(1, 1), Cur+wxPoint(1, 1)), true);
            }

            Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, LineColor);
            Renderer.DrawLine(Cur, Next);
        }
    }

    // Render the center X handle.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(150, 150, 150));
    Renderer.XHandle(Renderer.GetViewWin2D().WorldToTool(GetBB().GetCenter()));
}


void MapBrushT::Render3D(Renderer3DT& Renderer) const
{
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        m_Faces[FaceNr].Render3D(Renderer, this);
    }
}


bool MapBrushT::IsTranslucent() const
{
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
        if (m_Faces[FaceNr].m_Material && m_Faces[FaceNr].m_Material->IsTranslucent())
            return true;

    return false;
}


BoundingBox3fT MapBrushT::GetBB() const
{
    BoundingBox3fT BB;

    // We can skip the first face, as its vertices all occur in the other faces as well.
    for (unsigned long FaceNr=1; FaceNr<m_Faces.Size(); FaceNr++)
        BB+=m_Faces[FaceNr].GetVertices();

    return BB;
}


bool MapBrushT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    // If possible (RayOrigin is not in cull bounding-box), do a quick bounding-box check first.
    if (!GetBB().Contains(RayOrigin) && !MapPrimitiveT::TraceRay(RayOrigin, RayDir, Fraction, FaceNr)) return false;

    // Ok, we hit the bounding box. Now test the actual brush.
    // The following code has been taken and adapted from Brush3T<T>::TraceBoundingBox().
    // It cannot be called directly or indirectly, because in addition to the results it provides,
    // we also need to know the FaceNr of the intersection.

    // First test if RayOrigin is inside the brush. If so, there is no intersection.
    unsigned long PlaneNr;

    for (PlaneNr=0; PlaneNr<m_Faces.Size(); PlaneNr++)
        if (m_Faces[PlaneNr].GetPlane().GetDistance(RayOrigin)>=0) break;

    if (PlaneNr==m_Faces.Size()) return false;

    // Now check for intersections with each plane.
    for (PlaneNr=0; PlaneNr<m_Faces.Size(); PlaneNr++)
    {
        const Plane3fT& Plane=m_Faces[PlaneNr].GetPlane();

        // Bestimmen, bei welchem Bruchteil (Fraction F) von Dir wir die Plane schneiden.
        const float Nenner=dot(Plane.Normal, RayDir);

        // Dir muß dem Normalenvektor der Ebene wirklich entgegenzeigen! Ist der Nenner==0, so ist Dir parallel zur Plane,
        // und mit dieser Plane ex. kein Schnittpunkt. Ist der Nenner>0, nutzen wir die Konvexität des Brushs aus:
        // Es gibt damit nur genau einen Schnittpunkt mit dem Brush (Eintrittsstelle) und die Plane behindert nicht
        // eine Bewegung von ihr weg (Feststecken wenn Dist==0 (s.u.)).
        if (Nenner>=0) continue;

        const float Dist= Plane.GetDistance(RayOrigin);
        const float Frac=-Dist/Nenner;

        // Der Schnitt macht nur Sinn, wenn F im gewünschten Intervall liegt
        if (Frac<0) continue;

        // Prüfen, ob Schnittpunkt wirklich auf dem Brush liegt
        const Vector3fT HitPos=RayOrigin + RayDir*Frac;
        unsigned long   PNr;

        for (PNr=0; PNr<m_Faces.Size(); PNr++)
            if (PNr!=PlaneNr /*Rundungsfehlern zuvorkommen!*/ && m_Faces[PNr].GetPlane().GetDistance(HitPos)>0.01) break;

        if (PNr<m_Faces.Size()) continue;

        Fraction=Frac;
        FaceNr  =PlaneNr;
        return true;
    }

    return false;
}


bool MapBrushT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
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

    // 4. Check the edges of all faces for a hit.
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        const MapFaceT&          Face    =m_Faces[FaceNr];
        const ArrayT<Vector3fT>& Vertices=Face.GetVertices();

        if (Vertices.Size()==0) continue;

        wxPoint Vertex1=ViewWin.WorldToWindow(Vertices[Vertices.Size()-1]);

        for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
        {
            const wxPoint Vertex2=ViewWin.WorldToWindow(Vertices[VertexNr]);

            if (ViewWindow2DT::RectIsIntersected(Disc, Vertex1, Vertex2))
                return true;

            Vertex1=Vertex2;
        }
    }

    // 5. Despite all efforts we found no hit.
    return false;
}


wxString MapBrushT::GetDescription() const
{
    return wxString::Format("Brush with %lu faces", m_Faces.Size());
}


void MapBrushT::Split(const Plane3T<float>& Plane, MapBrushT** Front, MapBrushT** Back) const
{
    unsigned long VerticesOnFront=0;
    unsigned long VerticesOnBack =0;

    wxASSERT(Plane.IsValid());

    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        for (unsigned long VertexNr=0; VertexNr<m_Faces[FaceNr].GetVertices().Size(); VertexNr++)
        {
            const float HALF_PLANE_THICKNESS=0.001f;
            const float Dist=Plane.GetDistance(m_Faces[FaceNr].GetVertices()[VertexNr]);

            if (Dist> HALF_PLANE_THICKNESS) VerticesOnFront++;
            if (Dist<-HALF_PLANE_THICKNESS) VerticesOnBack++;
        }
    }

    // If this brush was entirely on the back side of Plane, just return a copy if the caller wants one.
    if (VerticesOnFront==0)
    {
        if (Back) *Back=new MapBrushT(*this);
        return;
    }

    // If this brush was entirely on the front side of Plane, just return a copy if the caller wants one.
    if (VerticesOnBack==0)
    {
        if (Front) *Front=new MapBrushT(*this);
        return;
    }

    if (Front)
    {
        // The front half is a copy of this brush with an additional face defined by Plane (mirrored).
        MapBrushT* FrontHalf=new MapBrushT(*this);

        FrontHalf->m_Faces.PushBack(MapFaceT(m_Faces[0].GetMaterial(), Plane.GetMirror(), NULL, Options.general.NewUVsFaceAligned));
        FrontHalf->CompleteFaceVertices();

        if (FrontHalf->IsValid()) *Front=FrontHalf; else delete FrontHalf;
    }

    if (Back)
    {
        // The back half is a copy of this brush with an additional face defined by Plane (straight).
        MapBrushT* BackHalf=new MapBrushT(*this);

        BackHalf->m_Faces.PushBack(MapFaceT(m_Faces[0].GetMaterial(), Plane, NULL, Options.general.NewUVsFaceAligned));
        BackHalf->CompleteFaceVertices();

        if (BackHalf->IsValid()) *Back=BackHalf; else delete BackHalf;
    }
}


bool MapBrushT::Subtract(const MapBrushT* B, ArrayT<MapBrushT*>& Result) const
{
    // When the bounding-boxes don't overlap, there is no point in performing the subtraction.
    if (!GetBB().Intersects(B->GetBB())) return false;

    MapBrushT WorkA(*this);

    for (unsigned long FaceNr=0; FaceNr<B->m_Faces.Size(); FaceNr++)
    {
        MapBrushT* Front=NULL;
        MapBrushT* Back =NULL;

        WorkA.Split(B->m_Faces[FaceNr].GetPlane(), &Front, &Back);

        // Any front piece is a part of the result.
        if (Front!=NULL) Result.PushBack(Front);

        // Back==NULL means that this brush A is completely in front of one of the planes of B.
        if (Back==NULL) return false;

        // Start the next iteration without the cut-away result portion (the front piece) of WorkA.
        WorkA.m_Faces = Back->m_Faces;      // WorkA = *Back;
        delete Back;
    }

    // The remaining WorkA is the union of this brush and B.
    return true;
}


namespace
{
    class BrushTrafoMementoT : public TrafoMementoT
    {
        public:

        BrushTrafoMementoT(const ArrayT<MapFaceT>& Faces)
            : m_Faces(Faces)
        {
        }

        const ArrayT<MapFaceT> m_Faces;
    };
}


TrafoMementoT* MapBrushT::GetTrafoState() const
{
    return new BrushTrafoMementoT(m_Faces);
}


void MapBrushT::RestoreTrafoState(const TrafoMementoT* TM)
{
    const BrushTrafoMementoT* BrushTM = dynamic_cast<const BrushTrafoMementoT*>(TM);

    wxASSERT(BrushTM);
    if (!BrushTM) return;

    // Using ArrayT assignment would be shorter:
    //     m_Faces = BrushTM->m_Faces;
    // but (as implemented at this time) also unnecessarily re-allocate all array elements as well.
    wxASSERT(m_Faces.Size() == BrushTM->m_Faces.Size());
    for (unsigned int i = 0; i < m_Faces.Size(); i++)
        m_Faces[i] = BrushTM->m_Faces[i];
}


void MapBrushT::TrafoMove(const Vector3fT& Delta, bool LockTexCoords)
{
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        MapFaceT& Face=m_Faces[FaceNr];

        for (unsigned long VertexNr=0; VertexNr<3; VertexNr++)
            Face.m_PlanePoints[VertexNr]+=Delta;

        for (unsigned long VertexNr=0; VertexNr<Face.m_Vertices.Size(); VertexNr++)
            Face.m_Vertices[VertexNr]+=Delta;

        if (LockTexCoords)
        {
            SurfaceInfoT& SI=Face.m_SurfaceInfo;
            wxASSERT(SI.TexCoordGenMode==PlaneProj);

            // Shift the material by the projection of the translation into uv-space.
            // In order to derive these statements, consider the equation in MapFaceT::UpdateTextureSpace():
            //     (UAxis dot Vertex) * s + t
            // replace Vertex by Vertex+Delta, then rearrange until you find what you have to replace "t"
            // with in order to obtain the same result as before (the answer is "t-(UAxis dot Delta)*s").
            SI.Trans[0]-=dot(Delta, SI.UAxis) * SI.Scale[0];
            SI.Trans[1]-=dot(Delta, SI.VAxis) * SI.Scale[1];
            SI.WrapTranslations();
        }

        Face.m_Plane=Plane3T<float>(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
        Face.UpdateTextureSpace();
    }

    MapPrimitiveT::TrafoMove(Delta, LockTexCoords);
}


void MapBrushT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords)
{
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        MapFaceT& Face=m_Faces[FaceNr];

        for (unsigned long VertexNr=0; VertexNr<Face.m_PlanePoints.Size(); VertexNr++)
        {
            Vector3fT& Vertex=Face.m_PlanePoints[VertexNr];

            Vertex-=RefPoint;

            if (Angles.x!=0.0f) Vertex=Vertex.GetRotX( Angles.x);
            if (Angles.y!=0.0f) Vertex=Vertex.GetRotY(-Angles.y);
            if (Angles.z!=0.0f) Vertex=Vertex.GetRotZ( Angles.z);

            Vertex+=RefPoint;
        }

        for (unsigned long VertexNr=0; VertexNr<Face.m_Vertices.Size(); VertexNr++)
        {
            Vector3fT& Vertex=Face.m_Vertices[VertexNr];

            Vertex-=RefPoint;

            if (Angles.x!=0.0f) Vertex=Vertex.GetRotX( Angles.x);
            if (Angles.y!=0.0f) Vertex=Vertex.GetRotY(-Angles.y);
            if (Angles.z!=0.0f) Vertex=Vertex.GetRotZ( Angles.z);

            Vertex+=RefPoint;
        }

        if (LockTexCoords)
        {
            SurfaceInfoT& SI=Face.m_SurfaceInfo;

            // Rotate the texture-space axes just like the face.
            if (Angles.x!=0.0f) { SI.UAxis=SI.UAxis.GetRotX( Angles.x); SI.VAxis=SI.VAxis.GetRotX( Angles.x); }
            if (Angles.y!=0.0f) { SI.UAxis=SI.UAxis.GetRotY(-Angles.y); SI.VAxis=SI.VAxis.GetRotY(-Angles.y); }
            if (Angles.z!=0.0f) { SI.UAxis=SI.UAxis.GetRotZ( Angles.z); SI.VAxis=SI.VAxis.GetRotZ( Angles.z); }

            // Properly account for the shift in translation offset due to the rotation.
            Vector3fT Shift=-RefPoint;

            if (Angles.x!=0.0f) Shift=Shift.GetRotX( Angles.x);
            if (Angles.y!=0.0f) Shift=Shift.GetRotY(-Angles.y);
            if (Angles.z!=0.0f) Shift=Shift.GetRotZ( Angles.z);

            Shift+=RefPoint;

            SI.Trans[0]-=dot(Shift, SI.UAxis) * SI.Scale[0];
            SI.Trans[1]-=dot(Shift, SI.VAxis) * SI.Scale[1];
            SI.WrapTranslations();
        }
        else
        {
            // Let's just re-initialize the uv-axes after a rotation without "texture locking" enabled.
            // If we instead just did nothing (i.e. keep the axes as-is), one of the axes might become
            // perpendicular to the Face.m_Plane (i.e. identical to the Face.m_Plane.Normal).
            Face.m_SurfaceInfo.ResetUVAxes(Face.m_Plane, Options.general.NewUVsFaceAligned);
        }

        Face.m_Plane=Plane3T<float>(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
        Face.UpdateTextureSpace();
    }

    MapPrimitiveT::TrafoRotate(RefPoint, Angles, LockTexCoords);
}


void MapBrushT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords)
{
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        MapFaceT& Face=m_Faces[FaceNr];

        for (unsigned long VertexNr=0; VertexNr<3; VertexNr++)
            Face.m_PlanePoints[VertexNr]=(Face.m_PlanePoints[VertexNr]-RefPoint).GetScaled(Scale) + RefPoint;

        for (unsigned long VertexNr=0; VertexNr<Face.m_Vertices.Size(); VertexNr++)
            Face.m_Vertices[VertexNr]=(Face.m_Vertices[VertexNr]-RefPoint).GetScaled(Scale) + RefPoint;

        if (LockTexCoords && Face.m_Vertices.Size()>0)
        {
            SurfaceInfoT& SI=Face.m_SurfaceInfo;

            // The code for updating the SI here is obtained from the same key idea as in TrafoMove():
            // In the following, let A#Scale mean A.GetScaled(Scale).
            //
            // In MapFaceT::UpdateTextureSpace(), we have   (UAxis dot Vertex) * s + t   Now find UAxis', s' and t'
            // such that   (UAxis dot Vertex) * s + t == (UAxis' dot [(Vertex-R)#Scale+R]) * s' + t'.
            // Rearranging, we find   ... == (UAxis' dot Vertex#Scale) * s' + (UAxis' dot [R-R#Scale]) * s' + t'.
            //
            // Given this, choose UAxis' initially as UAxis#(1/Scale) in order to get the first dot product "right",
            // but also make the new UAxis' a unit vector (a general requirement for our texture axes).
            // Compensate for this by scaling s' appropriately. In summary:
            //    UAxis' = UAxis#(1/Scale) / LenU   and   s' = s * LenU   where   LenU = ||UAxis#(1/Scale)||.
            // This yields the new value for t' from the rest of the term.
            // (Unfortunately, the downside of this approach is that UAxis' in general foregoes the face-alignment of UAxis.)
            const Vector3fT NewU(SI.UAxis.x/Scale.x, SI.UAxis.y/Scale.y, SI.UAxis.z/Scale.z);
            const Vector3fT NewV(SI.VAxis.x/Scale.x, SI.VAxis.y/Scale.y, SI.VAxis.z/Scale.z);
            const float     LenU=length(NewU);
            const float     LenV=length(NewV);

            if (LenU>0.0001f && LenV>0.0001f)
            {
                SI.UAxis=NewU/LenU;
                SI.VAxis=NewV/LenV;

                SI.Scale[0]*=LenU;
                SI.Scale[1]*=LenV;

                SI.Trans[0]-=dot(SI.UAxis, RefPoint-RefPoint.GetScaled(Scale)) * SI.Scale[0];
                SI.Trans[1]-=dot(SI.VAxis, RefPoint-RefPoint.GetScaled(Scale)) * SI.Scale[1];
                SI.WrapTranslations();
            }
        }

        Face.m_Plane=Plane3T<float>(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
        Face.UpdateTextureSpace();
    }

    MapPrimitiveT::TrafoScale(RefPoint, Scale, LockTexCoords);
}


/// Mirrors an array of vertices along the given mirror plane and appropriately reverses their order.
static void MirrorVertices(ArrayT<Vector3fT>& Vertices, unsigned int NormalAxis, float Dist)
{
    // First reverse the order of all vertices.
    for (unsigned long VertexNr=0; VertexNr<Vertices.Size()/2; VertexNr++)
    {
        const unsigned long RevNr=Vertices.Size()-1-VertexNr;
        const Vector3fT     Swap =Vertices[VertexNr];

        Vertices[VertexNr]=Vertices[RevNr];
        Vertices[RevNr   ]=Swap;
    }

    // Now mirror the vertices along the given plane.
    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
        Vertices[VertexNr][NormalAxis]=Dist-(Vertices[VertexNr][NormalAxis]-Dist);
}


void MapBrushT::TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords)
{
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        MapFaceT& Face=m_Faces[FaceNr];

        MirrorVertices(Face.m_PlanePoints, NormalAxis, Dist);
        MirrorVertices(Face.m_Vertices,    NormalAxis, Dist);

        // If texture locking is enabled, also mirror the texture-space U/V axes.
        if (LockTexCoords)
        {
            SurfaceInfoT& SI=Face.m_SurfaceInfo;

            SI.UAxis[NormalAxis]=-SI.UAxis[NormalAxis];
            SI.VAxis[NormalAxis]=-SI.VAxis[NormalAxis];

            // Properly account for the shift in translation offset due to the mirroring of the axes.
            // In order to derive these statements, consider the equation in MapFaceT::UpdateTextureSpace():
            //     (UAxis dot Vertex) * s + t
            // replace Vertex by the mirrored vertex, UAxis by the mirrored UAxis, then rearrange until you
            // find what you have to replace "t" with in order to obtain the same result as before.
            SI.Trans[0]-=2.0f * SI.UAxis[NormalAxis] * Dist * SI.Scale[0];
            SI.Trans[1]-=2.0f * SI.VAxis[NormalAxis] * Dist * SI.Scale[1];
            SI.WrapTranslations();
        }

        Face.m_Plane=Plane3T<float>(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
        Face.UpdateTextureSpace();
    }

    MapPrimitiveT::TrafoMirror(NormalAxis, Dist, LockTexCoords);
}


void MapBrushT::Transform(const MatrixT& Matrix, bool LockTexCoords)
{
    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        MapFaceT& Face=m_Faces[FaceNr];

        // We simply transform all faces (by transforming their points),
        // the surface infos (texture-space vectors, scales and translations) are intentionally left alone.
        // Note that this means that e.g. with shearing, texture planes that were face aligned before
        // are not necessarily face aligned after the transformation.
        for (unsigned long VertexNr=0; VertexNr<3; VertexNr++)
        {
            Vector3fT& Vertex=Face.m_PlanePoints[VertexNr];

            Vertex=Matrix.Mul1(Vertex);
        }

        for (unsigned long VertexNr=0; VertexNr<Face.m_Vertices.Size(); VertexNr++)
        {
            Vector3fT& Vertex=Face.m_Vertices[VertexNr];

            Vertex=Matrix.Mul1(Vertex);
        }

        if (LockTexCoords)
        {
            SurfaceInfoT& SI = Face.m_SurfaceInfo;

            // The key idea for updating the SI is simple, and analogous to that in the other Trafo*() methods:
            // In MapFaceT::UpdateTextureSpace(), we have
            //
            //     (UAxis dot Vertex) * s + t
            //
            // We easily find a new proper UAxis' and new scale s' by transforming the old (UAxis*s) vector just like
            // the face vertices (but as UAxis is a directional vector, Mul0() is used rather than Mul1()).
            // The leaves us with equation
            //
            //     (UAxis dot Vertex) * s + t == (UAxis' dot Matrix.Mul1(Vertex)) * s' + t'
            //
            // to solve for the unknown t'. As we already know UAxis' and s', there is fortunately no need to consider
            // the equation generically for all possible values of Vertex. Instead, we fathom that the equation must
            // yield the same value t' for *any* value of Vertex. Exploiting this, we arbitrarily choose (0, 0, 0) as
            // its value, which makes solving for t' very easy.
            const Vector3fT NewU = Matrix.Mul0(SI.UAxis) * SI.Scale[0];
            const Vector3fT NewV = Matrix.Mul0(SI.VAxis) * SI.Scale[1];
            const float     LenU = length(NewU);
            const float     LenV = length(NewV);

            if (LenU > 0.0001f && LenV > 0.0001f)
            {
                SI.UAxis = NewU/LenU;
                SI.VAxis = NewV/LenV;

                SI.Scale[0] = LenU;
                SI.Scale[1] = LenV;

                SI.Trans[0] -= dot(SI.UAxis, Vector3fT(Matrix[0][3], Matrix[1][3], Matrix[2][3])) * SI.Scale[0];
                SI.Trans[1] -= dot(SI.VAxis, Vector3fT(Matrix[0][3], Matrix[1][3], Matrix[2][3])) * SI.Scale[1];
                SI.WrapTranslations();
            }
        }

        Face.m_Plane=Plane3T<float>(Face.m_PlanePoints[0], Face.m_PlanePoints[1], Face.m_PlanePoints[2], 0.1f);
        Face.UpdateTextureSpace();
    }

    MapPrimitiveT::Transform(Matrix, LockTexCoords);
}


void MapBrushT::CompleteFaceVertices()
{
    // Due to the much better precision, we intentionally use doubles, not floats, as the base type for the Polygon3Ts.
    // With old code that used only floats, we sometimes experienced errors in the vertex positions of about 0.02 units even with
    // simple brushes close to the origin. These errors were disturbingly visible in the 2D views even below the maximum zoom level.
    ArrayT< Polygon3T<double> > Polygons;

    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        const MapFaceT& Face=m_Faces[FaceNr];

        // That's a nice opportunity to make sure that Face defines the same plane in m_Plane and m_PlanePoints.
        wxASSERT(fabs(Face.m_Plane.GetDistance(Face.m_PlanePoints[0]))<0.1f);
        wxASSERT(fabs(Face.m_Plane.GetDistance(Face.m_PlanePoints[1]))<0.1f);
        wxASSERT(fabs(Face.m_Plane.GetDistance(Face.m_PlanePoints[2]))<0.1f);

        // Take the opportunity to remove this face if a preceeding face already had an identical plane.
        unsigned long FNr;

        for (FNr=0; FNr<FaceNr; FNr++)
        {
            const Plane3fT& P1=Face.GetPlane();
            const Plane3fT& P2=m_Faces[FNr].GetPlane();

            if (dot(P1.Normal, P2.Normal)>0.999f && fabs(P1.Dist-P2.Dist)<0.1f) break;
        }

        if (FNr<FaceNr)
        {
            m_Faces.RemoveAtAndKeepOrder(FaceNr);
            FaceNr--;
            continue;
        }

        // Add the plane of Face to the Polygons set.
        Polygons.PushBackEmpty();
        Polygons[FaceNr].Plane=Plane3dT(Face.m_Plane.Normal.AsVectorOfDouble(), Face.m_Plane.Dist);
    }

    Polygon3T<double>::Complete(Polygons, 0.2);

    for (unsigned long FaceNr=0; FaceNr<m_Faces.Size(); FaceNr++)
    {
        const Polygon3T<double>& Poly=Polygons[FaceNr];
        MapFaceT&                Face=m_Faces[FaceNr];

        if (!Poly.IsValid(0.2, 0.5))
        {
            // Have not been able to create a valid polygon for this face, so delete the entire face.
            // This incident is a bit problematic though, because deleting a plane might reversely affect the shape of the entire brush.
            // However, we cannot just render the entire brush invalid when this happens, because it might as well be a normal case of
            // a redundant plane, e.g. after the Clip tool knowingly inserted an additional clip plane.
            m_Faces.RemoveAtAndKeepOrder(FaceNr);
            Polygons.RemoveAtAndKeepOrder(FaceNr);
            FaceNr--;
            continue;
        }

        Face.m_Vertices.Overwrite();
        for (unsigned long VertexNr=0; VertexNr<Poly.Vertices.Size(); VertexNr++)
            Face.m_Vertices.PushBack(Poly.Vertices[VertexNr].AsVectorOfFloat());

        Face.UpdateTextureSpace();
    }

    // wxASSERT(IsValid());     // Cannot check for IsValid() here - e.g. the Clip tool may well have asked us to create an invalid brush!
}
