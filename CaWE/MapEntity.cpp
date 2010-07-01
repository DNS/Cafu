/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "MapEntity.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "MapHelperBB.hpp"
#include "MapHelperModel.hpp"
#include "MapPrimitive.hpp"
#include "Options.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "Math3D/Matrix.hpp"
#include "TypeSys.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapEntityT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapEntityT::TypeInfo(GetMapElemTIM(), "MapEntityT", "MapEntityBaseT", MapEntityT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapEntityT::MapEntityT()
    : MapEntityBaseT(Options.colors.Entity),
      m_Origin(),
      m_Helpers()
{
}


MapEntityT::MapEntityT(const MapEntityT& Entity)
    : MapEntityBaseT(Entity),
      m_Origin(Entity.m_Origin),
      m_Helpers()
{
    for (unsigned long HelperNr=0; HelperNr<Entity.m_Helpers.Size(); HelperNr++)
    {
        m_Helpers.PushBack(Entity.m_Helpers[HelperNr]->Clone());
        m_Helpers[HelperNr]->SetParentEntity(this);
    }
}


MapEntityT::~MapEntityT()
{
    for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
        delete m_Helpers[HelperNr];
}


MapEntityT* MapEntityT::Clone() const
{
    return new MapEntityT(*this);
}


void MapEntityT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapEntityBaseT::Assign(Elem);

    const MapEntityT* Ent=dynamic_cast<const MapEntityT*>(Elem);
    wxASSERT(Ent!=NULL);
    if (Ent==NULL) return;

    m_Origin=Ent->m_Origin;

    for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
        delete m_Helpers[HelperNr];

    m_Helpers.Overwrite();

    for (unsigned long HelperNr=0; HelperNr<Ent->m_Helpers.Size(); HelperNr++)
    {
        m_Helpers.PushBack(Ent->m_Helpers[HelperNr]->Clone());
        m_Helpers[HelperNr]->SetParentEntity(this);
    }
}


wxColour MapEntityT::GetColor(bool ConsiderGroup) const
{
    if (m_Group && ConsiderGroup)
        return m_Group->Color;

    return m_Class->GetColor();
}


wxString MapEntityT::GetDescription() const
{
    const EntPropertyT* NameProp=FindProperty("name");

    return m_Class->GetName()+" entity"+(NameProp ? " ("+NameProp->Value+")" : "");
}


void MapEntityT::Render2D(Renderer2DT& Renderer) const
{
    const BoundingBox3fT BB    =GetBB();
    const wxPoint        Point1=Renderer.GetViewWin2D().WorldToTool(BB.Min);
    const wxPoint        Point2=Renderer.GetViewWin2D().WorldToTool(BB.Max);
    const wxPoint        Center=Renderer.GetViewWin2D().WorldToTool(BB.GetCenter());
    const wxColour       Color =IsSelected() ? Options.colors.Selection : GetColor(Options.view2d.UseGroupColors);

    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, Color);

    // Render the entities bounding box.
    Renderer.Rectangle(wxRect(Point1, Point2), false);

    // Render the center X handle.
    Renderer.XHandle(Center);


    if (Options.view2d.ShowEntityInfo && (Renderer.GetViewWin2D().GetZoom() >= 1))
    {
        Renderer.SetTextColor(Color, Options.Grid.ColorBackground);
        Renderer.DrawText(m_Class->GetName(), Point1+wxPoint(2,  1));

        const EntPropertyT* NameProp=FindProperty("name");
        if (NameProp!=NULL) Renderer.DrawText(NameProp->Value, Point1+wxPoint(2, 12));
    }

    if (Options.view2d.ShowEntityTargets)
    {
        const EntPropertyT* TargetProp=FindProperty("target");

        if (TargetProp!=NULL)
        {
            const ArrayT<MapEntityBaseT*>& Entities=Renderer.GetViewWin2D().GetMapDoc().GetEntities();
            ArrayT<MapEntityT*>            FoundEntities;

            for (unsigned long EntNr=1/*skip world*/; EntNr<Entities.Size(); EntNr++)
            {
                const EntPropertyT* FoundProp=Entities[EntNr]->FindProperty("name");

                wxASSERT(dynamic_cast<MapEntityT*>(Entities[EntNr])!=NULL);
                if (FoundProp!=NULL && FoundProp->Value==TargetProp->Value && Entities[EntNr]->GetType()==&MapEntityT::TypeInfo)
                {
                    FoundEntities.PushBack(static_cast<MapEntityT*>(Entities[EntNr]));
                }
            }

            for (unsigned long EntNr=0; EntNr<FoundEntities.Size(); EntNr++)
            {
                const wxPoint OtherCenter=Renderer.GetViewWin2D().WorldToTool(FoundEntities[EntNr]->GetOrigin());

                Renderer.DrawLine(Center, OtherCenter);
            }
        }
    }

    // Render the coordinate axes of our local system.
    if (IsSelected() && FindProperty("angles")!=NULL)
        Renderer.BasisVectors(GetOrigin(), cf::math::Matrix3x3fT::GetFromAngles_COMPAT(GetAngles()));

    // Render all helpers.
    for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
        m_Helpers[HelperNr]->Render2D(Renderer);
}


void MapEntityT::Render3D(Renderer3DT& Renderer) const
{
    // Render the coordinate axes of our local system.
    if (IsSelected() && FindProperty("angles")!=NULL)
        Renderer.BasisVectors(GetOrigin(), cf::math::Matrix3x3fT::GetFromAngles_COMPAT(GetAngles()));

    // Render all helpers.
    for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
        m_Helpers[HelperNr]->Render3D(Renderer);
}


/// Entities dilemma: What should the bounding-box of an entity comprise?
///    a) Only its origin (plus some margin)?
///    b) Its origin plus all of its primitives children?
///
/// This looks very much like a dilemma, because a) seems useful in a few cases (e.g. when an entity - i.e. its origin,
/// but none of its primitives - is moved and the observers want to determine the smallest required screen update region),
/// but b) seems to be required in at least as many cases.
///
/// The solution to the dilemma is to fix the assumptions: It's wrong to think about an entity as just its origin point.
/// Instead, we think about an entity always as the whole. That is, b) is the proper choice:
/// The size of an entity always equates to a bounding-box that comprises its origin and *all of its primitives*.
/// For example, the visual represenation of an entity in the 2D views is a rectangle that outlines that bounding-box.
/// Note however that this property is entirely *unrelated* to the mechanics of selection, where an entity and its
/// primitives are treated independently as if they had no relationship at all!
BoundingBox3fT MapEntityT::GetBB() const
{
    BoundingBox3fT BB;

    if (!m_Class->IsSolidClass()) BB+=m_Origin;

    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        BB+=m_Primitives[PrimNr]->GetBB();

    for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
        BB+=m_Helpers[HelperNr]->GetBB();

    if (!BB.IsInited()) BB+=m_Origin;
    return BB;
}


bool MapEntityT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    // Entities are either hit indirectly via their primitives (brushes, Bezier patches, terrains, etc.),
    // or directly via their helpers.
    unsigned long HitCount=0;

    for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
    {
        float         Fraction_;
        unsigned long FaceNr_;

        if (m_Helpers[HelperNr]->TraceRay(RayOrigin, RayDir, Fraction_, FaceNr_))
        {
            if (HitCount==0 || Fraction_<Fraction)
            {
                Fraction=Fraction_;
                FaceNr  =FaceNr_;
            }

            HitCount++;
        }
    }

    return HitCount>0;
}


bool MapEntityT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    // Note that entities in 2D views are always indicated (drawn) and selected
    //   - via their bounding-box,
    //   - via their center handle, and
    //   - if they have an origin, via their origin.
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


void MapEntityT::TrafoMove(const Vector3fT& delta)
{
    m_Origin+=delta;

    MapElementT::TrafoMove(delta);
}


void MapEntityT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& RotAngles)
{
    // Rotate the origin.
    m_Origin-=RefPoint;

    if (RotAngles.x!=0.0f) m_Origin=m_Origin.GetRotX( RotAngles.x);
    if (RotAngles.y!=0.0f) m_Origin=m_Origin.GetRotY(-RotAngles.y);
    if (RotAngles.z!=0.0f) m_Origin=m_Origin.GetRotZ( RotAngles.z);

    m_Origin+=RefPoint;


    // Convert the existing orientation (expressed in m_Angles) and the additionally to be applied delta rotation
    // (expressed in Angles) to 3x3 rotation matrixes (for backwards-compatibility, both conversions require extra code).
    // Then multiply the matrices in order to obtain the new orientation, and convert that (again bw.-comp.) back to m_Angles.
    const cf::math::Matrix3x3fT OldMatrix=cf::math::Matrix3x3fT::GetFromAngles_COMPAT(GetAngles());
    const cf::math::Matrix3x3fT RotMatrix=cf::math::Matrix3x3fT::GetFromAngles_COMPAT(cf::math::AnglesfT(-RotAngles[1], RotAngles[2], RotAngles[0]));

    cf::math::AnglesfT NewAngles=(RotMatrix*OldMatrix).ToAngles_COMPAT();

    // Carefully round and normalize the angles.
    if (fabs(NewAngles[PITCH])<0.001f) NewAngles[PITCH]=0;
    if (fabs(NewAngles[YAW  ])<0.001f) NewAngles[YAW  ]=0; if (NewAngles[YAW]<0) NewAngles[YAW]+=360.0f;
    if (fabs(NewAngles[ROLL ])<0.001f) NewAngles[ROLL ]=0;

    SetAngles(NewAngles);

    MapElementT::TrafoRotate(RefPoint, RotAngles);
}


void MapEntityT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale)
{
    m_Origin=RefPoint + (m_Origin-RefPoint).GetScaled(Scale);

    MapElementT::TrafoScale(RefPoint, Scale);
}


void MapEntityT::TrafoMirror(unsigned int NormalAxis, float Dist)
{
    m_Origin[NormalAxis]=Dist-(m_Origin[NormalAxis]-Dist);

    MapElementT::TrafoMirror(NormalAxis, Dist);
}


void MapEntityT::Transform(const MatrixT& Matrix)
{
    m_Origin=Matrix.Mul1(m_Origin);

    MapElementT::Transform(Matrix);
}


void MapEntityT::SetClass(const EntityClassT* NewClass)
{
    if (m_Class==NewClass) return;

    // Assign the new class (m_Class=NewClass) and instantiate the variables (properties) of the new class.
    MapEntityBaseT::SetClass(NewClass);

    // Our entity class changed, so update our helpers.
    UpdateHelpers();
}


ArrayT<EntPropertyT> MapEntityT::CheckUniqueValues(MapDocumentT& MapDoc, bool Repair)
{
    // All nonunique properties that are found (and repaired if Repair is true).
    ArrayT<EntPropertyT> FoundVars;

    // Only handle entities that have a class.
    if (m_Class==NULL) return FoundVars;

    const ArrayT<const EntClassVarT*>& ClassVars=m_Class->GetVariables();

    // For all class vars (only class vars can be unique).
    for (unsigned long i=0; i<ClassVars.Size(); i++)
    {
        // Check if class var is unique.
        // FIXME: At the moment only the "name" variable is supported, as this is the only unique variable at this time.
        if (ClassVars[i]->IsUnique() && ClassVars[i]->GetName()=="name")
        {
            // Remember current value.
            EntPropertyT*         FoundProp=FindProperty("name");
            const MapEntityBaseT* FoundEnt =NULL;

            // If value is set.
            if (FoundProp!=NULL)
            {
                for (unsigned long EntNr=1/*skip world*/; EntNr<MapDoc.GetEntities().Size(); EntNr++)
                {
                    const MapEntityBaseT* Ent    =MapDoc.GetEntities()[EntNr];
                    const EntPropertyT*   EntProp=Ent->FindProperty("name");

                    if (Ent!=this && EntProp && EntProp->Value==FoundProp->Value)
                    {
                        FoundEnt=Ent;
                        break;
                    }
                }
            }

            // Found a faulty value.
            if (FoundEnt!=NULL || FoundProp==NULL)
            {
                // Set property value to default.
                wxString ValueTmp=ClassVars[i]->GetDefault();

                // Overwrite default value by concrete value if property is set.
                if (FoundProp!=NULL) ValueTmp=FoundProp->Value;

                FoundVars.PushBack(EntPropertyT("name", ValueTmp));
            }

            // User doesn't want the faulty value to be repaired or there is nothing to repair.
            if (!Repair || (FoundEnt==NULL && FoundProp!=NULL)) continue;

            // Repair faulty value.
            for (unsigned long Count=1; true; Count++)
            {
                const wxString UniqueValue=MakeLuaVarName(m_Class->GetName())+wxString::Format("_%03lu", Count);
                unsigned long  EntNr;

                for (EntNr=1/*skip world*/; EntNr<MapDoc.GetEntities().Size(); EntNr++)
                {
                    const MapEntityBaseT* Ent    =MapDoc.GetEntities()[EntNr];
                    const EntPropertyT*   EntProp=Ent->FindProperty("name");

                    if (Ent!=this && EntProp && EntProp->Value==UniqueValue)
                    {
                        FoundEnt=Ent;
                        break;
                    }
                }

                if (EntNr>=MapDoc.GetEntities().Size())
                {
                    FindProperty("name", NULL, true)->Value=UniqueValue;

                    ArrayT<MapElementT*> MapElements;
                    MapElements.PushBack(this);

                    MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, "name");
                    break;
                }
            }
        }
    }

    return FoundVars;
}


Vector3fT MapEntityT::GetOrigin() const
{
    if (!m_Class->IsSolidClass()) return m_Origin;

    // This is very similar to GetBB().GetCenter(), but without accounting for the helpers.
    // The helpers GetBB() methods call m_ParentEntity->GetOrigin(), possibly creating an infinite recursion.
    BoundingBox3fT BB;

    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        BB+=m_Primitives[PrimNr]->GetBB();

    return BB.IsInited() ? BB.GetCenter() : m_Origin;
}


void MapEntityT::SetOrigin(const Vector3fT& Origin)
{
    m_Origin=Origin;
}


void MapEntityT::UpdateHelpers()
{
    for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
        delete m_Helpers[HelperNr];

    m_Helpers.Overwrite();

    if (m_Class!=NULL)
    {
        // Add all the helpers that this class declares in the EntityClassDefs.lua file.
        for (unsigned long HelperNr=0; HelperNr<m_Class->GetHelpers().Size(); HelperNr++)
        {
            const HelperInfoT* HelperInfo=m_Class->GetHelpers()[HelperNr];
            MapHelperT*        Helper    =NULL;

            // It would be ideal if we could somehow better employ the type system here:
            //     Helper=GetMapElemTIM().FindTypeInfoByName(HelperInfo->Name)->CreateInstance(Params);
            if (HelperInfo->Name=="model")
            {
                Helper=new MapHelperModelT(this, HelperInfo);
            }
            else if (HelperInfo->Name=="iconsprite")
            {
                // TODO: Implement!
            }
            else wxMessageBox(wxString("Helper \"")+HelperInfo->Name+"\" not found!", "CaWE WARNING");

            if (Helper!=NULL)
                m_Helpers.PushBack(Helper);
        }
    }

    // If the class definition does not specify any helpers, or none of the helpers could be added,
    // a box helper is added so that the entity has some visual representation.
    if (m_Primitives.Size()==0 && m_Helpers.Size()==0)
    {
        m_Helpers.PushBack(new MapHelperBoundingBoxT(this,
            m_Class!=NULL ? m_Class->GetBoundingBox() : BoundingBox3fT(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8))));
    }
}
