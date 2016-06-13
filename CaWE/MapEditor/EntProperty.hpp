/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_ENT_PROPERTY_HPP_INCLUDED
#define CAFU_ENT_PROPERTY_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "wx/string.h"
#include <ostream>


class TextParserT;


class EntPropertyT
{
    public:

    EntPropertyT(const wxString& Key_="", const wxString& Value_="") : Key(Key_), Value(Value_) { }

    void Load_cmap(TextParserT& TP);
    void Save_cmap(std::ostream& OutFile) const;

    Vector3fT GetVector3f() const;


    wxString Key;
    wxString Value;
 // const EntClassVarT* Var;    //CF: Should we have such a member here? Saves us searching all variables of the entity class...
};

#endif
