/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "AxesInfo.hpp"


AxesInfoT::AxesInfoT(int HorzAxis_, bool MirrorHorz_, int VertAxis_, bool MirrorVert_)
    : HorzAxis(HorzAxis_),
      VertAxis(VertAxis_),
      ThirdAxis(0),
      MirrorHorz(MirrorHorz_),
      MirrorVert(MirrorVert_)
{
    if (HorzAxis!=0 && VertAxis!=0) ThirdAxis=0;
    if (HorzAxis!=1 && VertAxis!=1) ThirdAxis=1;
    if (HorzAxis!=2 && VertAxis!=2) ThirdAxis=2;
}
