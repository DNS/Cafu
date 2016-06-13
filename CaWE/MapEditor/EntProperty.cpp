/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "EntProperty.hpp"

#include "wx/wx.h"
#include "wx/sstream.h"
#include "wx/txtstrm.h"


Vector3fT EntPropertyT::GetVector3f() const
{
    wxStringInputStream sis(Value);
    wxTextInputStream   tis(sis);
    Vector3fT           Vec;

    tis >> Vec.x >> Vec.y >> Vec.z;

    return Vec;
}
