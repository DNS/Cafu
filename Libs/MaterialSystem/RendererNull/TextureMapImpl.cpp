/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

/**********************************/
/*** Texture Map Implementation ***/
/**********************************/

#include <stdlib.h>

#include "TextureMapImpl.hpp"


/******************************/
/*** TextureMapManagerImplT ***/
/******************************/

TextureMapManagerImplT& TextureMapManagerImplT::Get()
{
    // It is *okay* to abandon the TextureMapManagerImplT object on program exit on the heap as in C++ FAQ 16.15.
    // Using a local static object instead has the potential for the static DEinitialization order problem (see C++ FAQ 16.16),
    // which has cost me *weeks* to debug once in May/June 2005.
    // Thus, as abandonding is fine and I want to be safe, a local static pointer is used (see C++ FAQ 16.18).
    static TextureMapManagerImplT* TMMI=new TextureMapManagerImplT();

    return *TMMI;
}


void TextureMapManagerImplT::SetMaxTextureSize(unsigned long MaxSize)
{
}


unsigned long TextureMapManagerImplT::GetMaxTextureSize() const
{
    return 0x10000;
}


MatSys::TextureMapI* TextureMapManagerImplT::GetTextureMap2D(const MapCompositionT& MC)
{
    return NULL;
}


MatSys::TextureMapI* TextureMapManagerImplT::GetTextureMap2D(char* Data, unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    return NULL;
}


MatSys::TextureMapI* TextureMapManagerImplT::GetTextureMap2D(BitmapT* Bitmap, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    return NULL;
}


void TextureMapManagerImplT::FreeTextureMap(MatSys::TextureMapI* TM)
{
}
