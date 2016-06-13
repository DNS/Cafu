/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************************/
/*** Texture Map Implementation ***/
/**********************************/

#ifndef CAFU_MATSYS_TEXTUREMAP_IMPLEMENTATION_HPP_INCLUDED
#define CAFU_MATSYS_TEXTUREMAP_IMPLEMENTATION_HPP_INCLUDED

#include "../TextureMap.hpp"


class TextureMapManagerImplT : public MatSys::TextureMapManagerI
{
    public:

    // TextureMapManagerI implementation.
    void SetMaxTextureSize(unsigned long MaxSize);
    unsigned long GetMaxTextureSize() const;
    MatSys::TextureMapI* GetTextureMap2D(const MapCompositionT& MapComp);
    MatSys::TextureMapI* GetTextureMap2D(char* Data, unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);
    MatSys::TextureMapI* GetTextureMap2D(BitmapT* Bitmap, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);
    void FreeTextureMap(MatSys::TextureMapI* TM);


    // Internal interface.

    /// Get a pointer/reference to the texture-map manager singleton.
    static TextureMapManagerImplT& Get();


    private:

    TextureMapManagerImplT() { }
};

#endif
