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

/****************************/
/*** MatSys Font (Header) ***/
/****************************/

#ifndef _MATSYS_FONT_HPP_
#define _MATSYS_FONT_HPP_

#include <string>


namespace MatSys { class RenderMaterialT; }


/// A class for MatSys-based font rendering.
/// The only requirement is that the MatSys in fully initialized (the global MatSys::Renderer pointer is set)
/// before any object of this class is instantiated.
class FontT
{
    public:

    /// The constructor.
    FontT(const std::string& MaterialName);

    /// The destructor.
    ~FontT();

    /// The copy constructor.
    FontT(const FontT& Other);

    /// The assignment operator.
    FontT& operator = (const FontT& Other);

    /// Prints PrintString at (PosX, PosY) in color Color.
    void Print(int PosX, int PosY, float FrameWidth, float FrameHeight, unsigned long Color, const char* PrintString, ...);

    /// Accumulative printing functions. Faster if you have to call Print() a lot.
    void AccPrintBegin(float FrameWidth, float FrameHeight);
    void AccPrint(int PosX, int PosY, unsigned long Color, const char* PrintString, ...);
    void AccPrintEnd();


    private:

    MatSys::RenderMaterialT* RenderMaterial;
};

#endif
