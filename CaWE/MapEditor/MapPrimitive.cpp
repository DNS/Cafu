/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MapPrimitive.hpp"
#include "CompMapEntity.hpp"
#include "Group.hpp"
#include "MapEntRepres.hpp"
#include "TypeSys.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapPrimitiveT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapPrimitiveT::TypeInfo(GetMapElemTIM(), "MapPrimitiveT", "MapElementT", MapPrimitiveT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapPrimitiveT::MapPrimitiveT(const wxColour& Color)
    : MapElementT(),
      m_Color(Color)
{
}


MapPrimitiveT::MapPrimitiveT(const MapPrimitiveT& Prim)
    : MapElementT(Prim),
      m_Color(Prim.m_Color)
{
}


wxColour MapPrimitiveT::GetColor(bool ConsiderGroup) const
{
    if (m_Group && ConsiderGroup)
        return m_Group->Color;

    if (m_Parent != NULL && !m_Parent->IsWorld())
        return m_Parent->GetRepres()->GetColor(false);

    // The primitive has no parent, or the parent is the world.
    return m_Color;
}
