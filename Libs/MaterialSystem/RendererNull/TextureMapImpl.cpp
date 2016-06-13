/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
