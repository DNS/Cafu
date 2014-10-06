/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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
#include "ChildFrameViewWin3D.hpp"
#include "CompMapEntity.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "../Camera.hpp"
#include "../Options.hpp"

#include "MaterialSystem/Renderer.hpp"


using namespace MapEditor;


/*** Begin of TypeSys related definitions for this class. ***/

void* MapEntRepresT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapEntRepresT::TypeInfo(GetMapElemTIM(), "MapEntRepresT", "MapElementT", MapEntRepresT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapEntRepresT::MapEntRepresT(MapEditor::CompMapEntityT* Parent)
    : MapElementT(),
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
    m_Parent = GetMapEnt(m_Cloned).get();

    // The entity was copied...
    wxASSERT(m_Cloned->GetChildren().Size() == 0);    /// ... non-recursively,
    wxASSERT(m_Parent->GetPrimitives().Size() == 0);  /// ... without any primitives.
}


MapEntRepresT* MapEntRepresT::Clone() const
{
    return new MapEntRepresT(*this);
}


wxColour MapEntRepresT::GetColor(bool /*ConsiderGroup*/) const
{
    IntrusivePtrT<cf::GameSys::EntityT> Ent = m_Parent->GetEntity();

    if (Ent->GetComponents().Size() > 0)
        return Ent->GetComponents()[0]->GetEditorColor();

    return Ent->GetBasics()->GetEditorColor();
}


wxString MapEntRepresT::GetDescription() const
{
    wxString Desc = "The representation of an entity in the map";

    // Desc += ", class \"" + m_Parent->GetClass()->GetName() + "\"";
    Desc += ", name \"" + m_Parent->GetEntity()->GetBasics()->GetEntityName() + "\"";

    return Desc + ".";
}


void MapEntRepresT::Render2D(Renderer2DT& Renderer) const
{
    const BoundingBox3fT BB = GetRepresBB();

    if (BB.IsInited())
    {
        const wxPoint  Point1 = Renderer.GetViewWin2D().WorldToTool(BB.Min);
        const wxPoint  Point2 = Renderer.GetViewWin2D().WorldToTool(BB.Max);
        const wxPoint  Center = Renderer.GetViewWin2D().WorldToTool(BB.GetCenter());
        const wxColour Color  = IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors);

        Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, Color);

        // Render the entities bounding box.
        Renderer.Rectangle(wxRect(Point1, Point2), false);

        // Render the center X handle.
        Renderer.XHandle(Center);


        if (Options.view2d.ShowEntityInfo && (Renderer.GetViewWin2D().GetZoom() >= 1))
        {
            Renderer.SetTextColor(Color, Options.Grid.ColorBackground);
            Renderer.DrawText(m_Parent->GetEntity()->GetBasics()->GetEntityName(), Point1 + wxPoint(2, 1));
        }

        if (Options.view2d.ShowEntityTargets)
        {
            const EntPropertyT* TargetProp = m_Parent->FindProperty("target");

            if (TargetProp!=NULL)
            {
                ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > FoundEntities;
                ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;

                GetParent()->GetEntity()->GetRoot()->GetAll(AllEnts);

                for (unsigned long EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
                {
                    if (AllEnts[EntNr]->GetBasics()->GetEntityName() == TargetProp->Value)
                    {
                        FoundEntities.PushBack(AllEnts[EntNr]);
                    }
                }

                for (unsigned long EntNr = 0; EntNr < FoundEntities.Size(); EntNr++)
                {
                    const wxPoint OtherCenter = Renderer.GetViewWin2D().WorldToTool(FoundEntities[EntNr]->GetTransform()->GetOriginWS());

                    Renderer.DrawLine(Center, OtherCenter);
                }
            }
        }
    }

    // Render the coordinate axes of our local system.
    if (IsSelected())
    {
        Renderer.BasisVectors(m_Parent->GetEntity()->GetTransform()->GetOriginWS(), cf::math::Matrix3x3fT(m_Parent->GetEntity()->GetTransform()->GetQuatWS()));
    }
}


void MapEntRepresT::Render3D(Renderer3DT& Renderer) const
{
    IntrusivePtrT<cf::GameSys::EntityT> Ent = m_Parent->GetEntity();
    const float EntDist = length(Ent->GetTransform()->GetOriginWS() - Renderer.GetViewWin3D().GetCamera().Pos);
    bool ComponentRendered = false;

    if (EntDist < float(Options.view3d.ModelDistance))
    {
        // Render the cf::GameSys::EntityT instance itself.
        MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
        MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, Ent->GetTransform()->GetEntityToWorld());

        ComponentRendered = Ent->RenderComponents(false /*FirstPersonView*/, EntDist);

        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    }

    if (!ComponentRendered || IsSelected())
    {
        // Maybe the distance was too great, maybe the entity's components did not render anything,
        // or maybe we're selected. In all these cases, render an appropriate bounding-box.
        const BoundingBox3fT BB = GetRepresBB();

        if (BB.IsInited())
        {
            Renderer.RenderBox(BB,
                IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors),
                !ComponentRendered && Ent->GetComponents().Size() > 0 /*Solid?*/);
        }
    }

    // Render the coordinate axes of our local system.
    if (IsSelected())
        Renderer.BasisVectors(Ent->GetTransform()->GetOriginWS(), cf::math::Matrix3x3fT(Ent->GetTransform()->GetQuatWS()));
}


bool MapEntRepresT::IsTranslucent() const
{
    return false;
}


BoundingBox3fT MapEntRepresT::GetBB() const
{
    // Returning GetRepresBB() here, or in fact the GetBB() method itself, are somewhat unfortunate,
    // because this assumes that there is only one bounding-box for all MapElementTs for all purposes.
    // Especially, with GetBB() alone we cannot differentiate between a culling BB and an "editor handle"
    // BB -- see ComponentBaseT::GetCullingBB() and ComponentBaseT::GetEditorBB() for details!
    BoundingBox3fT BB = GetRepresBB();

    if (!BB.IsInited())
    {
        // If the components didn't come up with something, fall back to the old, bare-bones, humble bounding-box.
        BoundingBox3fT BB(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8));

        BB.Min += m_Parent->GetEntity()->GetTransform()->GetOriginWS();
        BB.Max += m_Parent->GetEntity()->GetTransform()->GetOriginWS();

        return BB;
    }

    // BB as returned by GetRepresBB() is in world-space already,
    // so we can return it immediately.
    return BB;
}


bool MapEntRepresT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    const BoundingBox3fT BB = GetRepresBB();

    if (!BB.IsInited()) return false;

    return BB.TraceRay(RayOrigin, RayDir, Fraction);
}


bool MapEntRepresT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    // Note that entities in 2D views are always indicated (drawn) and selected
    //   - via their bounding-box,
    //   - via their center handle, and
    //   - if they have an origin, via their origin.
    const BoundingBox3fT BB = GetRepresBB();

    if (!BB.IsInited()) return false;

    const wxRect Disc = wxRect(Pixel, Pixel).Inflate(Radius, Radius);
    const wxRect Rect = wxRect(ViewWin.WorldToWindow(BB.Min), ViewWin.WorldToWindow(BB.Max));

    // Note that the check against the Rect frame (that has a width of 2*Radius) is done in two steps:
    // First by checking if Disc is entirely outside of Rect, then below by checking if Disc is entirely inside Rect.
    if (!Rect.Intersects(Disc)) return false;
    if (Disc.Contains(ViewWin.WorldToWindow(BB.GetCenter()))) return true;
    if (Options.view2d.SelectByHandles) return false;

    return !Rect.Contains(Disc);
}


namespace
{
    // This memento class encapsulates the transform-related state of a MapEntRepresT.
    // It must cover the transform state of the related entity *and* of its immediate children:
    // Per definition, the Map Editor's `MapElementT`s are independent of each other, and thus, `MapEntRepresT`s are
    // independent of each other as well. As a consequence, when the selected map elements are transformed, *only* the
    // *selected* map elements must be affected by the transform. That especially means that if a child of an entity
    // is *not* selected, the child's world-space position must remain unchanged despite the transform of its parent!
    // Therefore, when an entity is transformed, the transform of each child must be updated as well, because the
    // children's transforms are kept in parent-space, but must remain unchanged in *world-space*. Note that it is
    // not necessary to consider any grand-children, as dealing with the immediate children is sufficient.
    class EntRepresTrafoMementoT : public TrafoMementoT
    {
        public:

        EntRepresTrafoMementoT(IntrusivePtrT<cf::GameSys::EntityT> Ent)
        {
            m_Origins.PushBack(Ent->GetTransform()->GetOriginPS());
            m_Quats.PushBack(Ent->GetTransform()->GetQuatPS());

            for (unsigned int ChildNr = 0; ChildNr < Ent->GetChildren().Size(); ChildNr++)
            {
                m_Origins.PushBack(Ent->GetChildren()[ChildNr]->GetTransform()->GetOriginPS());
                m_Quats.PushBack(Ent->GetChildren()[ChildNr]->GetTransform()->GetQuatPS());
            }
        }

        ArrayT<Vector3fT>              m_Origins;
        ArrayT<cf::math::QuaternionfT> m_Quats;
    };
}


TrafoMementoT* MapEntRepresT::GetTrafoState() const
{
    return new EntRepresTrafoMementoT(m_Parent->GetEntity());
}


void MapEntRepresT::RestoreTrafoState(const TrafoMementoT* TM)
{
    const EntRepresTrafoMementoT* EntRepresTM = dynamic_cast<const EntRepresTrafoMementoT*>(TM);

    wxASSERT(EntRepresTM);
    if (!EntRepresTM) return;

    IntrusivePtrT<cf::GameSys::EntityT> Ent = m_Parent->GetEntity();

    Ent->GetTransform()->SetOriginPS(EntRepresTM->m_Origins[0]);
    Ent->GetTransform()->SetQuatPS(EntRepresTM->m_Quats[0]);

    for (unsigned int ChildNr = 0; ChildNr < Ent->GetChildren().Size(); ChildNr++)
    {
        Ent->GetChildren()[ChildNr]->GetTransform()->SetOriginPS(EntRepresTM->m_Origins[1 + ChildNr]);
        Ent->GetChildren()[ChildNr]->GetTransform()->SetQuatPS(EntRepresTM->m_Quats[1 + ChildNr]);
    }
}


void MapEntRepresT::TrafoMove(const Vector3fT& Delta)
{
    const Vector3fT Origin = m_Parent->GetEntity()->GetTransform()->GetOriginWS();

    m_Parent->GetEntity()->GetTransform()->SetOriginWS(Origin + Delta);

    MapElementT::TrafoMove(Delta);
}


void MapEntRepresT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles)
{
    Vector3fT Origin = m_Parent->GetEntity()->GetTransform()->GetOriginWS();

    // Rotate the origin.
    Origin -= RefPoint;

    if (Angles.x != 0.0f) Origin = Origin.GetRotX( Angles.x);
    if (Angles.y != 0.0f) Origin = Origin.GetRotY(-Angles.y);
    if (Angles.z != 0.0f) Origin = Origin.GetRotZ( Angles.z);

    Origin += RefPoint;

    const cf::math::AnglesfT AngRad = Angles * (cf::math::AnglesfT::PI / 180.0);

    const cf::math::QuaternionfT OldQuat = m_Parent->GetEntity()->GetTransform()->GetQuatWS();
    const cf::math::QuaternionfT RotQuat = cf::math::QuaternionfT::Euler(-AngRad[1], AngRad[2], AngRad[0]);

    m_Parent->GetEntity()->GetTransform()->SetOriginWS(Origin);
    m_Parent->GetEntity()->GetTransform()->SetQuatWS(OldQuat * RotQuat);

    MapElementT::TrafoRotate(RefPoint, Angles);
}


void MapEntRepresT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale)
{
    const Vector3fT Origin = m_Parent->GetEntity()->GetTransform()->GetOriginWS();

    m_Parent->GetEntity()->GetTransform()->SetOriginWS(RefPoint + (Origin - RefPoint).GetScaled(Scale));

    MapElementT::TrafoScale(RefPoint, Scale);
}


void MapEntRepresT::TrafoMirror(unsigned int NormalAxis, float Dist)
{
    Vector3fT Origin = m_Parent->GetEntity()->GetTransform()->GetOriginWS();

    Origin[NormalAxis] = Dist - (Origin[NormalAxis] - Dist);

    m_Parent->GetEntity()->GetTransform()->SetOriginWS(Origin);

    MapElementT::TrafoMirror(NormalAxis, Dist);
}


void MapEntRepresT::Transform(const MatrixT& Matrix)
{
    const Vector3fT Origin = m_Parent->GetEntity()->GetTransform()->GetOriginWS();

    m_Parent->GetEntity()->GetTransform()->SetOriginWS(Matrix.Mul1(Origin));

    MapElementT::Transform(Matrix);
}


BoundingBox3fT MapEntRepresT::GetRepresBB() const
{
    IntrusivePtrT<cf::GameSys::EntityT> Ent = m_Parent->GetEntity();

#if 0   // No. Not having a visual representation at all it too confusing,
        // because newly created entities seem to disappear when deselected,
        // entities that are used for grouping seem not to exist, etc.
        // (Simply render them as "wireframe" in the 3D views instead.)
    if (Ent->GetComponents().Size() == 0)
    {
        // An entity without any custom component won't have a visual representation in the map.
        return BoundingBox3fT();
    }
#endif

    BoundingBox3fT BB = Ent->GetBasics()->GetEditorBB();

    if (Ent->GetComponents().Size() > 0)
        BB = Ent->GetComponents()[0]->GetEditorBB();

    wxASSERT(BB.IsInited());

    // Transform BB from the entity's local space into world space.
    const MatrixT Mat = m_Parent->GetEntity()->GetTransform()->GetEntityToWorld();
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
