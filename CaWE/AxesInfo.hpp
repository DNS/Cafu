/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_AXES_INFO_HPP_INCLUDED
#define CAFU_AXES_INFO_HPP_INCLUDED


/// This class describes how the three world-space axes are mapped to the two screen- or window-space axes.
/// The origin of screen/window-space is always in the upper left corner. The screen/window-space horizontal axis
/// ("x-axis") points from left to right, and the related vertical axis ("y-axis") points from top to bottom.
/// In the context of this class, the three world space axes x, y and z are numbered 0, 1 and 2, respectively.
class AxesInfoT
{
    public:

    AxesInfoT(int HorzAxis_, bool MirrorHorz_, int VertAxis_, bool MirrorVert_);


    public:

    int  HorzAxis;      ///< The number of the world axis that maps to the horizontal (x) screen/window axis.
    int  VertAxis;      ///< The number of the world axis that maps to the vertical   (y) screen/window axis.
    int  ThirdAxis;     ///< The number of the world axis that points into or out of the screen/window.

    bool MirrorHorz;    ///< When true, the mapping from the horizontal world space axis to the horizontal screen/window axis is mirrored.
    bool MirrorVert;    ///< When true, the mapping from the vertical   world space axis to the vertical   screen/window axis is mirrored.
};

#endif
