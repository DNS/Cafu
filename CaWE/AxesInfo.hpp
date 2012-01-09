/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _AXES_INFO_HPP_
#define _AXES_INFO_HPP_


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
