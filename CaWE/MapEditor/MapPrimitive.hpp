/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAP_PRIMITIVE_HPP_INCLUDED
#define CAFU_MAP_PRIMITIVE_HPP_INCLUDED

#include "MapElement.hpp"


/// This class adds no functionality of its own, but only exists for proper type separation.
/// Especially MapBaseEntityT keeps a MapEntRepresT and an array of MapPrimitiveT%s, and the two
/// "sets" must not overlap (we don't want MapEntRepresT instances among the "regular" primitives,
/// and no regular primitive should ever be in place of the m_Repres member).
/// In many other regards, all derived classes are considered equivalent and treated the same;
/// then we use arrays of MapElementT%s.
/// The clear distinction between MapElementT%s and MapPrimitiveT%s (the former can also contain
/// MapEntRepresT%s, the latter cannot) is also a great help in documentation and communication.
class MapPrimitiveT : public MapElementT
{
    public:

    /// The default constructor.
    MapPrimitiveT(const wxColour& Color);

    /// The copy constructor for copying a primitive.
    /// @param Prim   The primitive to copy-construct this primitive from.
    MapPrimitiveT(const MapPrimitiveT& Prim);

    /// The virtual copy constructor.
    /// Creates a copy of this primitive that is of the *same* class as the original, even when
    /// called via a base class pointer (the caller doesn't need to know the exact derived class).
    virtual MapPrimitiveT* Clone() const = 0;

    // Implementations and overrides for base class methods.
    wxColour GetColor(bool ConsiderGroup=true) const;


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    wxColour m_Color;   ///< The inherent color of this map primitive.
};

#endif
