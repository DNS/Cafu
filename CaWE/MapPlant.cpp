/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "MapPlant.hpp"

#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "LuaAux.hpp"
#include "Options.hpp"
#include "MapEntity.hpp"

#include "MaterialSystem/Renderer.hpp"
#include "Plants/PlantDescription.hpp"
#include "Math3D/Matrix3x3.hpp"

#include "wx/wx.h"


static const float CAFU_ENG_SCALE=25.4f;


/*** Begin of TypeSys related definitions for this class. ***/

void* MapPlantT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapPlantT::TypeInfo(GetMapElemTIM(), "MapPlantT", "MapPrimitiveT", MapPlantT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapPlantT::MapPlantT()
    : MapPrimitiveT(wxColour(0, 100 + (rand() % 156), 0))
{
}


MapPlantT::MapPlantT(const PlantDescriptionT* PlantDescription, unsigned long RandomSeed, const Vector3fT& Position)
    : MapPrimitiveT(wxColour(0, 100 + (rand() % 156), 0)),
      m_Tree(PlantDescription, RandomSeed),
      m_RandomSeed(RandomSeed),
      m_Angles(),
      m_Position(Position),
      m_DescrFileName(PlantDescription->FileName)
{
}


MapPlantT::MapPlantT(const MapPlantT& Plant)
    : MapPrimitiveT(Plant),
      m_Tree(Plant.m_Tree),
      m_RandomSeed(Plant.m_RandomSeed),
      m_Angles(Plant.m_Angles),
      m_Position(Plant.m_Position),
      m_DescrFileName(Plant.m_DescrFileName)
{
}


MapPlantT* MapPlantT::Clone() const
{
    return new MapPlantT(*this);
}


void MapPlantT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapPrimitiveT::Assign(Elem);

    const MapPlantT* Plant=dynamic_cast<const MapPlantT*>(Elem);
    wxASSERT(Plant!=NULL);
    if (Plant==NULL) return;

    m_Tree         =Plant->m_Tree;
    m_RandomSeed   =Plant->m_RandomSeed;
    m_Angles       =Plant->m_Angles;
    m_Position     =Plant->m_Position;
    m_DescrFileName=Plant->m_DescrFileName;
}


BoundingBox3fT MapPlantT::GetBB() const
{
    BoundingBox3fT BB=m_Tree.GetTreeBounds();

    // Construct all eight vertices of this BB.
    Vector3fT VerticesBB[8]=
    {
        Vector3fT(BB.Min.x, BB.Min.y, BB.Min.z), Vector3fT(BB.Max.x, BB.Min.y, BB.Min.z),
        Vector3fT(BB.Min.x, BB.Min.y, BB.Max.z), Vector3fT(BB.Max.x, BB.Min.y, BB.Max.z),
        Vector3fT(BB.Min.x, BB.Max.y, BB.Min.z), Vector3fT(BB.Max.x, BB.Max.y, BB.Min.z),
        Vector3fT(BB.Min.x, BB.Max.y, BB.Max.z), Vector3fT(BB.Max.x, BB.Max.y, BB.Max.z)
    };

    // Rotate all eight vertices.
    for (unsigned long VertexNr=0; VertexNr<8; VertexNr++)
        VerticesBB[VertexNr]=VerticesBB[VertexNr].GetRotX(m_Angles[ROLL]).GetRotY(m_Angles[PITCH]).GetRotZ(m_Angles[YAW]);

    // Build a new BB of the rotated BB.
    BoundingBox3fT RotBB(VerticesBB[0]);

    for (unsigned long VertexNr=1; VertexNr<8; VertexNr++)
        RotBB.Insert(VerticesBB[VertexNr]);

    RotBB.Min=RotBB.Min/CAFU_ENG_SCALE*1000.0f + m_Position;
    RotBB.Max=RotBB.Max/CAFU_ENG_SCALE*1000.0f + m_Position;

    return RotBB;
}


void MapPlantT::Render2D(Renderer2DT& Renderer) const
{
    const BoundingBox3fT BB    =GetBB();
    const wxPoint        Point1=Renderer.GetViewWin2D().WorldToTool(BB.Min);
    const wxPoint        Point2=Renderer.GetViewWin2D().WorldToTool(BB.Max);
    const wxPoint        Center=Renderer.GetViewWin2D().WorldToTool(BB.GetCenter());

    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN,
        IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors));

    Renderer.Rectangle(wxRect(Point1, Point2), false);

    // Render the center X handle.
    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(150, 150, 150));
    Renderer.XHandle(Center);

    // Render the axes of our local system.
    if (IsSelected())
        Renderer.BasisVectors(m_Position, cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Angles));
}


void MapPlantT::Render3D(Renderer3DT& Renderer) const
{
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, m_Position.x, m_Position.y, m_Position.z);

        MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, m_Angles[YAW  ]);
        MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, m_Angles[PITCH]);
        MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, m_Angles[ROLL ]);

        MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, 1.0f/CAFU_ENG_SCALE*1000.0f);  // Scale tree units (meters) to CaWE units (inches).

        m_Tree.Draw();

    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);

    if (IsSelected())
    {
        Renderer.RenderBox(GetBB(), Options.colors.Selection, false /* Solid? */);

        // Render the axes of our local system.
        Renderer.BasisVectors(m_Position, cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Angles));
    }
}


bool MapPlantT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
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


void MapPlantT::TrafoMove(const Vector3fT& delta)
{
    m_Position+=delta;

    MapPrimitiveT::TrafoMove(delta);
}


void MapPlantT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles)
{
    // Rotate the origin.
    m_Position-=RefPoint;

    if (Angles.x!=0.0f) m_Position=m_Position.GetRotX( Angles.x);
    if (Angles.y!=0.0f) m_Position=m_Position.GetRotY(-Angles.y);
    if (Angles.z!=0.0f) m_Position=m_Position.GetRotZ( Angles.z);

    m_Position+=RefPoint;


    // Convert the existing orientation (expressed in m_Angles) and the additionally to be applied delta rotation
    // (expressed in Angles) to 3x3 rotation matrixes (for backwards-compatibility, both conversions require extra code).
    // Then multiply the matrices in order to obtain the new orientation, and convert that (again bw.-comp.) back to m_Angles.
    const cf::math::Matrix3x3fT OldMatrix=cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Angles);
    const cf::math::Matrix3x3fT RotMatrix=cf::math::Matrix3x3fT::GetFromAngles_COMPAT(cf::math::AnglesfT(-Angles[1], Angles[2], Angles[0]));

    m_Angles=(RotMatrix*OldMatrix).ToAngles_COMPAT();

    // Carefully round and normalize the angles.
    if (fabs(m_Angles[PITCH])<0.001f) m_Angles[PITCH]=0;
    if (fabs(m_Angles[YAW  ])<0.001f) m_Angles[YAW  ]=0; if (m_Angles[YAW]<0) m_Angles[YAW]+=360.0f;
    if (fabs(m_Angles[ROLL ])<0.001f) m_Angles[ROLL ]=0;

    MapPrimitiveT::TrafoRotate(RefPoint, Angles);
}


void MapPlantT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale)
{
    m_Position=RefPoint + (m_Position-RefPoint).GetScaled(Scale);

    MapPrimitiveT::TrafoScale(RefPoint, Scale);
}


void MapPlantT::TrafoMirror(unsigned int NormalAxis, float Dist)
{
    // Unfortunately, there is no way to mirror the actual mesh of the model...
    m_Position[NormalAxis]=Dist-(m_Position[NormalAxis]-Dist);

    MapPrimitiveT::TrafoMirror(NormalAxis, Dist);
}


void MapPlantT::Transform(const MatrixT& Matrix)
{
    m_Position=Matrix.Mul1(m_Position);

    MapPrimitiveT::Transform(Matrix);
}
