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
#include "MapEntity.hpp"
#include "MapHelperBB.hpp"
#include "MapHelperModel.hpp"
#include "EntityClass.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapEntRepresT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapEntRepresT::TypeInfo(GetMapElemTIM(), "MapEntRepresT", "MapPrimitiveT", MapEntRepresT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapEntRepresT::MapEntRepresT()
    : MapPrimitiveT(wxColour(100 + (rand() % 156), 80, 100 + (rand() % 156))),
      m_Helper(NULL)
{
}


MapEntRepresT::MapEntRepresT(const MapEntRepresT& EntRepres)
    : MapPrimitiveT(EntRepres),
      m_Helper(NULL)
{
}


void MapEntRepresT::Update()
{
    delete m_Helper;
    m_Helper = NULL;

    if (!m_Parent) return;

    const MapEntityT* Ent = dynamic_cast<const MapEntityT*>(m_Parent);

    if (!Ent) return;

    if (m_Parent->FindProperty("model"))
    {
        static const HelperInfoT HelperInfo;

        m_Helper = new MapHelperModelT(Ent, &HelperInfo);
    }
    else
    {
        m_Helper = new MapHelperBoundingBoxT(Ent,
            m_Parent->GetClass() ? m_Parent->GetClass()->GetBoundingBox() : BoundingBox3fT(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8)));
    }
}


MapEntRepresT* MapEntRepresT::Clone() const
{
    return new MapEntRepresT(*this);
}


void MapEntRepresT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapPrimitiveT::Assign(Elem);

    const MapEntRepresT* MapRepres=dynamic_cast<const MapEntRepresT*>(Elem);
    wxASSERT(MapRepres!=NULL);
    if (MapRepres==NULL) return;

    Update();
}


void MapEntRepresT::Render2D(Renderer2DT& Renderer) const
{
    // The helpers don't implement this:
    //
    //      "Render nothing in 2D, as the parent entity already renders its bounding box,
    //       center cross, orientation (angles), etc. by itself."
    //
    // ... so we have to move the code from the parent entity here!
}


void MapEntRepresT::Render3D(Renderer3DT& Renderer) const
{
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

    const MapEntityT* Ent = dynamic_cast<const MapEntityT*>(m_Parent);

    if (!Ent)
        return BoundingBox3fT(Vector3fT(0.0f, 0.0f, 0.0f));

    return BoundingBox3fT(Ent->GetOrigin());
}


bool MapEntRepresT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    return false;
}


bool MapEntRepresT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    return false;
}


void MapEntRepresT::Save_cmap(std::ostream& OutFile, unsigned long EntRepNr, const MapDocumentT& MapDoc) const
{
    // MapEntRepresT are always created dynamically,
    // and thus are never saved to disk.
}


wxString MapEntRepresT::GetDescription() const
{
    return "The representation of an entity in the map.";
}


void MapEntRepresT::TrafoMove(const Vector3fT& Delta)
{
    MapPrimitiveT::TrafoMove(Delta);
}


void MapEntRepresT::TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles)
{
    MapPrimitiveT::TrafoRotate(RefPoint, Angles);
}


void MapEntRepresT::TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale)
{
    MapPrimitiveT::TrafoScale(RefPoint, Scale);
}


void MapEntRepresT::TrafoMirror(unsigned int NormalAxis, float Dist)
{
    MapPrimitiveT::TrafoMirror(NormalAxis, Dist);
}


void MapEntRepresT::Transform(const MatrixT& Matrix)
{
    MapPrimitiveT::Transform(Matrix);
}
