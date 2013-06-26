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
#include "Camera.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "CompMapEntity.hpp"
#include "EntityClass.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "Options.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "MaterialSystem/Renderer.hpp"


using namespace MapEditor;


/*** Begin of TypeSys related definitions for this class. ***/

void* MapEntRepresT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapEntRepresT::TypeInfo(GetMapElemTIM(), "MapEntRepresT", "MapElementT", MapEntRepresT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapEntRepresT::MapEntRepresT(IntrusivePtrT<MapEditor::CompMapEntityT> Parent)
    : MapElementT(Options.colors.Entity),
      m_Cloned(NULL)
{
    wxASSERT(Parent != NULL);
    m_Parent = Parent;
}


MapEntRepresT::MapEntRepresT(const MapEntRepresT& EntRepres)
    : MapElementT(EntRepres),
      m_Cloned(NULL)
{
    m_Cloned = EntRepres.GetParent()->GetEntity()->Clone(false /*Recursive?*/);
    m_Parent = GetMapEnt(m_Cloned);

    // The entity was copied...
    wxASSERT(m_Cloned->GetChildren().Size() == 0);    /// ... non-recursively,
    wxASSERT(m_Parent->GetPrimitives().Size() == 0);  /// ... without any primitives.
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

    IntrusivePtrT<CompMapEntityT> ThisEnt  = GetParent();
    IntrusivePtrT<CompMapEntityT> OtherEnt = MapRepres->GetParent();

    // Note that we're here concerned only with the entity itself, not with its primitives.
    // This is intentional, because the primitives are handled explicitly elsewhere.
    ThisEnt->SetClass(OtherEnt->GetClass());
    ThisEnt->GetProperties() = OtherEnt->GetProperties();

    ThisEnt->GetEntity()->GetTransform()->SetOrigin(OtherEnt->GetOrigin());
    ThisEnt->GetEntity()->GetTransform()->SetQuat(OtherEnt->GetEntity()->GetTransform()->GetQuat());
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
    Desc += ", name \"" + m_Parent->GetEntity()->GetBasics()->GetEntityName() + "\"";

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

        Renderer.DrawText(m_Parent->GetEntity()->GetBasics()->GetEntityName(), Point1 + wxPoint(2, 12));
    }

    if (Options.view2d.ShowEntityTargets)
    {
        const EntPropertyT* TargetProp = m_Parent->FindProperty("target");

        if (TargetProp!=NULL)
        {
            const ArrayT< IntrusivePtrT<CompMapEntityT> >& Entities = Renderer.GetViewWin2D().GetMapDoc().GetEntities();
            ArrayT< IntrusivePtrT<CompMapEntityT> >        FoundEntities;

            for (unsigned long EntNr = 1/*skip world*/; EntNr < Entities.Size(); EntNr++)
            {
                if (Entities[EntNr]->GetEntity()->GetBasics()->GetEntityName() == TargetProp->Value)
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
    if (IsSelected())
    {
        Renderer.BasisVectors(m_Parent->GetOrigin(), cf::math::Matrix3x3fT(m_Parent->GetEntity()->GetTransform()->GetQuat()));
    }
}


void MapEntRepresT::Render3D(Renderer3DT& Renderer) const
{
    const float EntDist = length(m_Parent->GetOrigin() - Renderer.GetViewWin3D().GetCamera().Pos);

    if (GetComponentsBB().IsInited() && EntDist < float(Options.view3d.ModelDistance))
    {
        // Render the cf::GameSys::EntityT instance itself.
        IntrusivePtrT<cf::GameSys::EntityT> Ent = m_Parent->GetEntity();

        MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
        MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, Ent->GetModelToWorld());

        // TODO: There is no support for LoD-parameters at this time, add it!
        // const float CAFU_ENG_SCALE = 25.4f;
        // Ent->RenderComponents(CAFU_ENG_SCALE * EntDist);
        Ent->RenderComponents();

        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        if (IsSelected())
            Renderer.RenderBox(GetBB(), Options.colors.Selection, false /*Solid?*/);
    }
    else
    {
        // Did not render the real entity (the distance was too great), thus render a replacement bounding-box.
        Renderer.RenderBox(GetBB(),
            IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors), true /*Solid?*/);
    }

    // Render the coordinate axes of our local system.
    if (IsSelected())
        Renderer.BasisVectors(m_Parent->GetOrigin(), cf::math::Matrix3x3fT(m_Parent->GetEntity()->GetTransform()->GetQuat()));
}


bool MapEntRepresT::IsTranslucent() const
{
    return false;
}


BoundingBox3fT MapEntRepresT::GetBB() const
{
    BoundingBox3fT BB = GetComponentsBB();

    if (!BB.IsInited())
    {
        // If the components didn't come up with something, fall back to the old, bare-bones, humble bounding-box.
        return GetRepresBB();
    }

    // Transform BB from the entity's local space into world space.
    const MatrixT Mat = m_Parent->GetEntity()->GetModelToWorld();
    Vector3fT     VerticesBB[8];

    BB.GetCornerVertices(VerticesBB);

    // Rotate all eight vertices.
    for (unsigned int VertexNr = 0; VertexNr < 8; VertexNr++)
        VerticesBB[VertexNr] = Mat.Mul1(VerticesBB[VertexNr]);

    // Build a new BB of the rotated BB.
    BB = BoundingBox3fT(VerticesBB[0]);

    for (unsigned int VertexNr = 1; VertexNr < 8; VertexNr++)
        BB += VerticesBB[VertexNr];

    return BB;
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

    m_Parent->GetEntity()->GetTransform()->SetOrigin(Origin + Delta);

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

    const cf::math::AnglesfT AngRad = Angles * (cf::math::AnglesfT::PI / 180.0);

    const cf::math::QuaternionfT OldQuat = m_Parent->GetEntity()->GetTransform()->GetQuat();
    const cf::math::QuaternionfT RotQuat = cf::math::QuaternionfT::Euler(-AngRad[1], AngRad[2], AngRad[0]);

    m_Parent->GetEntity()->GetTransform()->SetOrigin(Origin);
    m_Parent->GetEntity()->GetTransform()->SetQuat(OldQuat * RotQuat);

    MapElementT::TrafoRotate(RefPoint, Angles);
}


void MapEntRepresT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale)
{
    const Vector3fT Origin = m_Parent->GetOrigin();

    m_Parent->GetEntity()->GetTransform()->SetOrigin(RefPoint + (Origin - RefPoint).GetScaled(Scale));

    MapElementT::TrafoScale(RefPoint, Scale);
}


void MapEntRepresT::TrafoMirror(unsigned int NormalAxis, float Dist)
{
    Vector3fT Origin = m_Parent->GetOrigin();

    Origin[NormalAxis] = Dist - (Origin[NormalAxis] - Dist);

    m_Parent->GetEntity()->GetTransform()->SetOrigin(Origin);

    MapElementT::TrafoMirror(NormalAxis, Dist);
}


void MapEntRepresT::Transform(const MatrixT& Matrix)
{
    const Vector3fT Origin = m_Parent->GetOrigin();

    m_Parent->GetEntity()->GetTransform()->SetOrigin(Matrix.Mul1(Origin));

    MapElementT::Transform(Matrix);
}


BoundingBox3fT MapEntRepresT::GetComponentsBB() const
{
    BoundingBox3fT BB;

    // Augment BB with the bounding-boxes of the components.
    // The CompMapEntityT application component (the set of primitives) is intentionally not included,
    // because we intend to cover the entity's *representation*, not its whole.
    // Map primitives are more like children of an entity (and have explicit treatment in the Map Editor anyway).
    IntrusivePtrT<cf::GameSys::EntityT> Ent = m_Parent->GetEntity();

    for (unsigned long CompNr = 0; CompNr < Ent->GetComponents().Size(); CompNr++)
    {
        const BoundingBox3fT CompBB = Ent->GetComponents()[CompNr]->GetEditorBB();

        if (CompBB.IsInited())
            BB += CompBB;
    }

    return BB;
}


BoundingBox3fT MapEntRepresT::GetRepresBB() const
{
    BoundingBox3fT BB = m_Parent->GetClass() ? m_Parent->GetClass()->GetBoundingBox() : BoundingBox3fT(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8));

    BB.Min += m_Parent->GetOrigin();
    BB.Max += m_Parent->GetOrigin();

    return BB;
}
