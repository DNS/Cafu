/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "MapEntRepres.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "CompMapEntity.hpp"
#include "EntityClass.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "MapEntityBase.hpp"
#include "MapHelperBB.hpp"
#include "MapHelperModel.hpp"
#include "Options.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "Math3D/Matrix.hpp"


using namespace MapEditor;


/*** Begin of TypeSys related definitions for this class. ***/

void* MapEntRepresT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapEntRepresT::TypeInfo(GetMapElemTIM(), "MapEntRepresT", "MapElementT", MapEntRepresT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapEntRepresT::MapEntRepresT(MapEntityBaseT* Parent)
    : MapElementT(Options.colors.Entity),
      m_Helper(NULL),
      m_Cloned(NULL)
{
    wxASSERT(Parent);
    m_Parent = Parent;

    Update();
}


MapEntRepresT::MapEntRepresT(const MapEntRepresT& EntRepres)
    : MapElementT(EntRepres),
      m_Helper(NULL),
      m_Cloned(NULL)
{
    /*
     *
     * TODO:  Make a shallow copy only!
     *
     */
    m_Cloned = EntRepres.GetParent()->GetCompMapEntity()->GetEntity()->Clone();
    m_Parent = GetMapEnt(m_Cloned)->GetMapEntity();

    Update();
}


MapEntRepresT::~MapEntRepresT()
{
    delete m_Helper;
    m_Helper = NULL;
}


void MapEntRepresT::Update()
{
    delete m_Helper;
    m_Helper = NULL;

    wxASSERT(m_Parent);

    if (m_Parent->FindProperty("model"))
    {
        static const HelperInfoT HelperInfo;

        m_Helper = new MapHelperModelT(*this, &HelperInfo);
    }
    else
    {
        m_Helper = new MapHelperBoundingBoxT(*this,
            m_Parent->GetClass() ? m_Parent->GetClass()->GetBoundingBox() : BoundingBox3fT(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8)));
    }
}


MapEntRepresT* MapEntRepresT::Clone() const
{
    return new MapEntRepresT(*this);
}


void MapEntRepresT::Assign(const MapElementT* Elem)
{
    if (Elem == this) return;

    MapElementT::Assign(Elem);

    const MapEntRepresT* MapRepres = dynamic_cast<const MapEntRepresT*>(Elem);
    wxASSERT(MapRepres != NULL);
    if (MapRepres == NULL) return;

    /************************/
    /*** TODO: Assign the other entity's stuff to ours! ***/
    /*******************************/
#if 0
    MapEntityBaseT* ThisEnt  = GetParent();
    MapEntityBaseT* OtherEnt = MapRepres->GetParent();

    ThisEnt->GetTransform()->SetOrigin(OtherEnt->GetTransform()->GetOrigin());
 // ThisEnt->GetTransform()->SetQuaternion(OtherEnt->GetTransform()->GetQuaternion());
#endif

    // Note the we're here concerned only with the entity itself, not with its primitives.
    // This is intentional, because there are MapPrimitiveT classes elsewhere in the application
    // that are dedicated to this purpose.
    Update();
}


wxColour MapEntRepresT::GetColor(bool ConsiderGroup) const
{
    if (m_Group && ConsiderGroup)
        return m_Group->Color;

    if (m_Parent->GetClass())
        return m_Parent->GetClass()->GetColor();

    return m_Color;
}


wxString MapEntRepresT::GetDescription() const
{
    wxString Desc = "The representation of an entity in the map";

    Desc += ", class \"" + m_Parent->GetClass()->GetName() + "\"";

    const EntPropertyT* NameProp = m_Parent->FindProperty("name");

    if (NameProp)
        Desc += ", name \"" + NameProp->Value + "\"";

    return Desc + ".";
}


void MapEntRepresT::Render2D(Renderer2DT& Renderer) const
{
    const BoundingBox3fT BB     = GetBB();
    const wxPoint        Point1 = Renderer.GetViewWin2D().WorldToTool(BB.Min);
    const wxPoint        Point2 = Renderer.GetViewWin2D().WorldToTool(BB.Max);
    const wxPoint        Center = Renderer.GetViewWin2D().WorldToTool(BB.GetCenter());
    const wxColour       Color  = IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors);

    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, Color);

    // Render the entities bounding box.
    Renderer.Rectangle(wxRect(Point1, Point2), false);

    // Render the center X handle.
    Renderer.XHandle(Center);


    if (Options.view2d.ShowEntityInfo && (Renderer.GetViewWin2D().GetZoom() >= 1))
    {
        Renderer.SetTextColor(Color, Options.Grid.ColorBackground);
        Renderer.DrawText(m_Parent->GetClass()->GetName(), Point1 + wxPoint(2, 1));

        const EntPropertyT* NameProp = m_Parent->FindProperty("name");
        if (NameProp!=NULL) Renderer.DrawText(NameProp->Value, Point1 + wxPoint(2, 12));
    }

    if (Options.view2d.ShowEntityTargets)
    {
        const EntPropertyT* TargetProp = m_Parent->FindProperty("target");

        if (TargetProp!=NULL)
        {
            const ArrayT<MapEntityBaseT*>& Entities = Renderer.GetViewWin2D().GetMapDoc().GetEntities();
            ArrayT<MapEntityBaseT*>        FoundEntities;

            for (unsigned long EntNr = 1/*skip world*/; EntNr < Entities.Size(); EntNr++)
            {
                const EntPropertyT* FoundProp = Entities[EntNr]->FindProperty("name");

                if (FoundProp!=NULL && FoundProp->Value == TargetProp->Value)
                {
                    FoundEntities.PushBack(Entities[EntNr]);
                }
            }

            for (unsigned long EntNr = 0; EntNr < FoundEntities.Size(); EntNr++)
            {
                const wxPoint OtherCenter = Renderer.GetViewWin2D().WorldToTool(FoundEntities[EntNr]->GetOrigin());

                Renderer.DrawLine(Center, OtherCenter);
            }
        }
    }

    // Render the coordinate axes of our local system.
    if (IsSelected() && m_Parent->FindProperty("angles") != NULL)
    {
        Renderer.BasisVectors(m_Parent->GetOrigin(), cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Parent->GetAngles()));
    }

    // Render the helper.
    if (m_Helper)
        m_Helper->Render2D(Renderer);
}


void MapEntRepresT::Render3D(Renderer3DT& Renderer) const
{
    // Render the coordinate axes of our local system.
    if (IsSelected() && m_Parent->FindProperty("angles") != NULL)
    {
        Renderer.BasisVectors(m_Parent->GetOrigin(), cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Parent->GetAngles()));
    }

    // Render the helper.
    if (m_Helper)
        m_Helper->Render3D(Renderer);
}


bool MapEntRepresT::IsTranslucent() const
{
    return false;
}


BoundingBox3fT MapEntRepresT::GetBB() const
{
    if (m_Helper)
        return m_Helper->GetBB();

    return BoundingBox3fT(m_Parent->GetOrigin()).GetEpsilonBox(8.0f);
}


bool MapEntRepresT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    return GetBB().TraceRay(RayOrigin, RayDir, Fraction);
}


bool MapEntRepresT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    // Note that entities in 2D views are always indicated (drawn) and selected
    //   - via their bounding-box,
    //   - via their center handle, and
    //   - if they have an origin, via their origin.
    const BoundingBox3fT BB   = GetBB();
    const wxRect         Disc = wxRect(Pixel, Pixel).Inflate(Radius, Radius);
    const wxRect         Rect = wxRect(ViewWin.WorldToWindow(BB.Min), ViewWin.WorldToWindow(BB.Max));

    // Note that the check against the Rect frame (that has a width of 2*Radius) is done in two steps:
    // First by checking if Disc is entirely outside of Rect, then below by checking if Disc is entirely inside Rect.
    if (!Rect.Intersects(Disc)) return false;
    if (Disc.Contains(ViewWin.WorldToWindow(BB.GetCenter()))) return true;
    if (Options.view2d.SelectByHandles) return false;

    return !Rect.Contains(Disc);
}


void MapEntRepresT::TrafoMove(const Vector3fT& Delta)
{
    const Vector3fT Origin = m_Parent->GetOrigin();

    m_Parent->SetOrigin(Origin + Delta);

    MapElementT::TrafoMove(Delta);
}


void MapEntRepresT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles)
{
    Vector3fT Origin = m_Parent->GetOrigin();

    // Rotate the origin.
    Origin -= RefPoint;

    if (Angles.x != 0.0f) Origin = Origin.GetRotX( Angles.x);
    if (Angles.y != 0.0f) Origin = Origin.GetRotY(-Angles.y);
    if (Angles.z != 0.0f) Origin = Origin.GetRotZ( Angles.z);

    Origin += RefPoint;


    // Convert the existing orientation (expressed in m_Angles) and the additionally to be applied delta rotation
    // (expressed in Angles) to 3x3 rotation matrixes (for backwards-compatibility, both conversions require extra code).
    // Then multiply the matrices in order to obtain the new orientation, and convert that (again bw.-comp.) back to m_Angles.
    const cf::math::Matrix3x3fT OldMatrix = cf::math::Matrix3x3fT::GetFromAngles_COMPAT(m_Parent->GetAngles());
    const cf::math::Matrix3x3fT RotMatrix = cf::math::Matrix3x3fT::GetFromAngles_COMPAT(cf::math::AnglesfT(-Angles[1], Angles[2], Angles[0]));

    cf::math::AnglesfT NewAngles = (RotMatrix*OldMatrix).ToAngles_COMPAT();

    // Carefully round and normalize the angles.
    if (fabs(NewAngles[PITCH]) < 0.001f) NewAngles[PITCH] = 0;
    if (fabs(NewAngles[YAW  ]) < 0.001f) NewAngles[YAW  ] = 0; if (NewAngles[YAW] < 0) NewAngles[YAW] += 360.0f;
    if (fabs(NewAngles[ROLL ]) < 0.001f) NewAngles[ROLL ] = 0;

    m_Parent->SetOrigin(Origin);
    m_Parent->SetAngles(NewAngles);

    MapElementT::TrafoRotate(RefPoint, Angles);
}


void MapEntRepresT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale)
{
    const Vector3fT Origin = m_Parent->GetOrigin();

    m_Parent->SetOrigin(RefPoint + (Origin - RefPoint).GetScaled(Scale));

    MapElementT::TrafoScale(RefPoint, Scale);
}


void MapEntRepresT::TrafoMirror(unsigned int NormalAxis, float Dist)
{
    Vector3fT Origin = m_Parent->GetOrigin();

    Origin[NormalAxis] = Dist - (Origin[NormalAxis] - Dist);

    m_Parent->SetOrigin(Origin);

    MapElementT::TrafoMirror(NormalAxis, Dist);
}


void MapEntRepresT::Transform(const MatrixT& Matrix)
{
    const Vector3fT Origin = m_Parent->GetOrigin();

    m_Parent->SetOrigin(Matrix.Mul1(Origin));

    MapElementT::Transform(Matrix);
}
