/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************************/
/*** Texture Map Implementation ***/
/**********************************/

// Required for #include <GL/glu.h> with MS VC++.
#if defined(_WIN32) && defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <GL/glu.h>

#include "TextureMapImpl.hpp"
#include "RendererImpl.hpp"
#include "Bitmap/Bitmap.hpp"
#include "../Common/OpenGLEx.hpp"      // for glext.h


inline bool isPowerOf2(unsigned long i)
{
   return i>0 && (i & (i-1))==0;
}


/*********************/
/*** TextureMap2DT ***/
/*********************/

TextureMap2DT::TextureMap2DT(const MapCompositionT& MapComp_)
    : Source(MC),
      MapComp(MapComp_),
      Bitmap(NULL),
      Data(NULL),
      SizeX(0),
      SizeY(0),
      BytesPerPixel(0),
      OpenGLObject(0),
      InitCounter(0)
{
}


TextureMap2DT::TextureMap2DT(char* Data_, unsigned long SizeX_, unsigned long SizeY_, char BytesPerPixel_, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
    : Source(MakePrivateCopy ? RawPtrOwn : RawPtrExt),
      MapComp(McForFiltersAndWrapping),
      Bitmap(NULL),
      // Assume that all rows are aligned on 4-byte boundaries! See OpenGL Programming Guide (Red Book), p. 311.
      Data(MakePrivateCopy ? new char[((SizeX_*BytesPerPixel_+3) & 0xFFFFFFFC)*SizeY_] : Data_),
      SizeX(SizeX_),
      SizeY(SizeY_),
      BytesPerPixel(BytesPerPixel_),
      OpenGLObject(0),
      InitCounter(0)
{
    // Assume that all rows are aligned on 4-byte boundaries! See OpenGL Programming Guide (Red Book), p. 311.
    if (MakePrivateCopy)
        for (unsigned long i=0; i<((SizeX_*BytesPerPixel_+3) & 0xFFFFFFFC)*SizeY_; i++)
            Data[i]=Data_[i];
}


TextureMap2DT::TextureMap2DT(BitmapT* Bitmap_, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
    : Source(MakePrivateCopy ? BitmapPtrOwn : BitmapPtrExt),
      MapComp(McForFiltersAndWrapping),
      Bitmap(MakePrivateCopy ? new BitmapT(*Bitmap_) : Bitmap_),
      Data(NULL),
      SizeX(0),
      SizeY(0),
      BytesPerPixel(0),
      OpenGLObject(0),
      InitCounter(0)
{
}


bool TextureMap2DT::IsCreatedFromMapComp(const MapCompositionT& MC_)
{
    return Source==MC && MapComp==MC_;
}


TextureMap2DT::~TextureMap2DT()
{
    switch (Source)
    {
        case MC:           delete Bitmap; break;
        case RawPtrExt:                   break;
        case RawPtrOwn:    delete[] Data; break;
        case BitmapPtrExt:                break;
        case BitmapPtrOwn: delete Bitmap; break;
    }

    if (InitCounter==RendererImplT::GetInstance().GetInitCounter() && OpenGLObject!=0)
    {
        glDeleteTextures(1, &OpenGLObject);
    }
}


unsigned int TextureMap2DT::GetSizeX()
{
    switch (Source)
    {
        case MC:
            if (!Bitmap) Bitmap=MapComp.GetBitmap();
            return Bitmap->SizeX;

        case RawPtrExt:
        case RawPtrOwn:
            return SizeX;

        case BitmapPtrExt:
        case BitmapPtrOwn:
            return Bitmap->SizeX;
    }

    return 0;
}


unsigned int TextureMap2DT::GetSizeY()
{
    switch (Source)
    {
        case MC:
            if (!Bitmap) Bitmap=MapComp.GetBitmap();
            return Bitmap->SizeY;

        case RawPtrExt:
        case RawPtrOwn:
            return SizeY;

        case BitmapPtrExt:
        case BitmapPtrOwn:
            return Bitmap->SizeY;
    }

    return 0;
}


GLuint TextureMap2DT::GetOpenGLObject()
{
    if (InitCounter<RendererImplT::GetInstance().GetInitCounter())
    {
        const GLint InternalFormat=(cf::GL_ARB_texture_compression_AVAIL && !MapComp.GetNoCompression()) ? GL_COMPRESSED_RGBA : GL_RGBA8;

        switch (Source)
        {
            case MC:
            case BitmapPtrExt:
            case BitmapPtrOwn:
            {
                if (Source==MC && !Bitmap) Bitmap=MapComp.GetBitmap();

                BitmapT* ScaledBitmap=NULL;
                const unsigned long MaxSize=TextureMapManagerImplT::Get().GetMaxTextureSize();

                // Scale to next smaller power of 2 and to or below the max. texture size.
                if (isPowerOf2(Bitmap->SizeX) && isPowerOf2(Bitmap->SizeY) && Bitmap->SizeX<=MaxSize && Bitmap->SizeY<=MaxSize)
                {
                    // The side lengths of Bitmap are already powers of 2 and at or below MaxSize - no need to scale.
                    ScaledBitmap=Bitmap;
                }
                else
                {
                    ScaledBitmap=new BitmapT(*Bitmap);

                    unsigned long NewX=1; while (2*NewX<=Bitmap->SizeX) NewX*=2;
                    unsigned long NewY=1; while (2*NewY<=Bitmap->SizeY) NewY*=2;

                    if (!MapComp.GetNoScaleDown())
                    {
                        // Note that NewX and NewY are now NOT treated independently of each other.
                        while (NewX>1 && NewY>1 && NewX>MaxSize && NewY>MaxSize)
                        {
                            NewX/=2;
                            NewY/=2;
                        }
                    }

                    ScaledBitmap->Scale(NewX, NewY);
                }

                glGenTextures(1, &OpenGLObject);
                glBindTexture(GL_TEXTURE_2D, OpenGLObject);

                if (MapComp.GetMinFilter()==MapCompositionT::Nearest || MapComp.GetMinFilter()==MapCompositionT::Linear)
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, ScaledBitmap->SizeX, ScaledBitmap->SizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, &ScaledBitmap->Data[0]);
                }
                else
                {
                    // Note that the gluBuild2DMipmaps() function can fail, but we ignore any errors.
                    gluBuild2DMipmaps(GL_TEXTURE_2D, InternalFormat, ScaledBitmap->SizeX, ScaledBitmap->SizeY, GL_RGBA, GL_UNSIGNED_BYTE, &ScaledBitmap->Data[0]);
                }

                switch (MapComp.GetMinFilter())
                {
                    case MapCompositionT::Nearest:                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST               ); break;
                    case MapCompositionT::Linear:                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR                ); break;
                    case MapCompositionT::Nearest_MipMap_Nearest: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); break;
                    case MapCompositionT::Nearest_MipMap_Linear:  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR ); break;
                    case MapCompositionT::Linear_MipMap_Nearest:  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST ); break;
                    case MapCompositionT::Linear_MipMap_Linear:   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR  ); break;
                }

                switch (MapComp.GetMagFilter())
                {
                    case MapCompositionT::Nearest: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); break;
                    default:                       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); break;
                }

                switch (MapComp.GetWrapModeS())
                {
                    case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT       ); break;
                    case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP        ); break;
                    case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); break;
                }

                switch (MapComp.GetWrapModeT())
                {
                    case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT       ); break;
                    case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP        ); break;
                    case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); break;
                }

                if (ScaledBitmap!=Bitmap)
                {
                    delete ScaledBitmap;
                    ScaledBitmap=NULL;
                }

                // if (Source==MC && WantToUnloadBitmapAfterOpenGLObjGen)
                // {
                //     delete Bitmap;
                //     Bitmap=NULL;
                // }
            }

            case RawPtrExt:
            case RawPtrOwn:
            {
                if (isPowerOf2(SizeX) && isPowerOf2(SizeY))
                {
                    glGenTextures(1, &OpenGLObject);
                    glBindTexture(GL_TEXTURE_2D, OpenGLObject);

                    if (MapComp.GetMinFilter()==MapCompositionT::Nearest || MapComp.GetMinFilter()==MapCompositionT::Linear)
                    {
                        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, SizeX, SizeY, 0, BytesPerPixel==3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, Data);
                    }
                    else
                    {
                        // Note that the gluBuild2DMipmaps() function can fail, but we ignore any errors.
                        gluBuild2DMipmaps(GL_TEXTURE_2D, InternalFormat, SizeX, SizeY, BytesPerPixel==3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, Data);
                    }

                    switch (MapComp.GetMinFilter())
                    {
                        case MapCompositionT::Nearest:                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST               ); break;
                        case MapCompositionT::Linear:                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR                ); break;
                        case MapCompositionT::Nearest_MipMap_Nearest: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); break;
                        case MapCompositionT::Nearest_MipMap_Linear:  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR ); break;
                        case MapCompositionT::Linear_MipMap_Nearest:  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST ); break;
                        case MapCompositionT::Linear_MipMap_Linear:   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR  ); break;
                    }

                    switch (MapComp.GetMagFilter())
                    {
                        case MapCompositionT::Nearest: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); break;
                        default:                       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); break;
                    }

                    switch (MapComp.GetWrapModeS())
                    {
                        case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT       ); break;
                        case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP        ); break;
                        case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); break;
                    }

                    switch (MapComp.GetWrapModeT())
                    {
                        case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT       ); break;
                        case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP        ); break;
                        case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); break;
                    }
                }
            }
        }

        InitCounter=RendererImplT::GetInstance().GetInitCounter();
    }

    return OpenGLObject;
}


/***********************/
/*** TextureMapCubeT ***/
/***********************/

const GLenum TextureMapCubeT::CubeTargets[6]=
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
};


const std::string TextureMapCubeT::CubeSuffixes[6]=
{
    "_px",
    "_py",
    "_pz",
    "_nx",
    "_ny",
    "_nz"
};


// Replaces all occurrences of '#' in BaseString with TextureMapCubeT::CubeSuffixes[SuffixNr], the cube map suffix with number SuffixNr.
std::string TextureMapCubeT::GetFullCubeMapString(std::string BaseString, unsigned long SuffixNr)
{
    for (std::string::size_type i=BaseString.find("#"); i!=std::string::npos; i=BaseString.find("#"))
        BaseString.replace(i, 1, TextureMapCubeT::CubeSuffixes[SuffixNr]);

    return BaseString;
}


TextureMapCubeT::TextureMapCubeT(const MapCompositionT& MapComp_)
    : Source(Files),
      MapComp(MapComp_),
      SizeX(0),
      SizeY(0),
      BytesPerPixel(0),
      OpenGLObject(0),
      InitCounter(0)
{
    for (int i=0; i<6; i++) Bitmap[i]=NULL;
    for (int i=0; i<6; i++) Data  [i]=NULL;
}


TextureMapCubeT::TextureMapCubeT(char* Data_[6], unsigned long SizeX_, unsigned long SizeY_, char BytesPerPixel_, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
    : Source(MakePrivateCopy ? RawPtrOwn : RawPtrExt),
      MapComp(McForFiltersAndWrapping),
      SizeX(SizeX_),
      SizeY(SizeY_),
      BytesPerPixel(BytesPerPixel_),
      OpenGLObject(0),
      InitCounter(0)
{
    for (int i=0; i<6; i++) Bitmap[i]=NULL;

    // Assume that all rows are aligned on 4-byte boundaries! See OpenGL Programming Guide (Red Book), p. 311.
    for (int i=0; i<6; i++) Data[i]=MakePrivateCopy ? new char[((SizeX_*BytesPerPixel_+3) & 0xFFFFFFFC)*SizeY_] : Data_[i];

    // Assume that all rows are aligned on 4-byte boundaries! See OpenGL Programming Guide (Red Book), p. 311.
    if (MakePrivateCopy)
        for (int i=0; i<6; i++)
            for (unsigned long j=0; j<((SizeX_*BytesPerPixel_+3) & 0xFFFFFFFC)*SizeY_; j++)
                Data[i][j]=Data_[i][j];
}


TextureMapCubeT::TextureMapCubeT(BitmapT* Bitmap_[6], bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
    : Source(MakePrivateCopy ? BitmapPtrOwn : BitmapPtrExt),
      MapComp(McForFiltersAndWrapping),
      SizeX(0),
      SizeY(0),
      BytesPerPixel(0),
      OpenGLObject(0),
      InitCounter(0)
{
    for (int i=0; i<6; i++) Bitmap[i]=MakePrivateCopy ? new BitmapT(*Bitmap_[i]) : Bitmap_[i];
    for (int i=0; i<6; i++) Data  [i]=NULL;
}


bool TextureMapCubeT::IsCreatedFromMapComp(const MapCompositionT& MC_)
{
    return Source==Files && MapComp==MC_;
}


TextureMapCubeT::~TextureMapCubeT()
{
    switch (Source)
    {
        case Files:        for (int i=0; i<6; i++) delete Bitmap[i]; break;
        case RawPtrExt:                                              break;
        case RawPtrOwn:    for (int i=0; i<6; i++) delete[] Data[i]; break;
        case BitmapPtrExt:                                           break;
        case BitmapPtrOwn: for (int i=0; i<6; i++) delete Bitmap[i]; break;
    }

    if (InitCounter==RendererImplT::GetInstance().GetInitCounter() && OpenGLObject!=0)
    {
        glDeleteTextures(1, &OpenGLObject);
    }
}


unsigned int TextureMapCubeT::GetSizeX()
{
    switch (Source)
    {
        case Files:
            if (!Bitmap[0])
            {
                TextParserT TP(GetFullCubeMapString(MapComp.GetString(), 0).c_str(), "({[]}),", false);
                Bitmap[0]=MapCompositionT(TP, MapComp.GetBaseDir()).GetBitmap();
            }
            return Bitmap[0]->SizeX;

        case RawPtrExt:
        case RawPtrOwn:
            return SizeX;

        case BitmapPtrExt:
        case BitmapPtrOwn:
            return Bitmap[0]->SizeX;
    }

    return 0;
}


unsigned int TextureMapCubeT::GetSizeY()
{
    switch (Source)
    {
        case Files:
            if (!Bitmap[0])
            {
                TextParserT TP(GetFullCubeMapString(MapComp.GetString(), 0).c_str(), "({[]}),", false);
                Bitmap[0]=MapCompositionT(TP, MapComp.GetBaseDir()).GetBitmap();
            }
            return Bitmap[0]->SizeY;

        case RawPtrExt:
        case RawPtrOwn:
            return SizeY;

        case BitmapPtrExt:
        case BitmapPtrOwn:
            return Bitmap[0]->SizeY;
    }

    return 0;
}


GLuint TextureMapCubeT::GetOpenGLObject()
{
    // The cf::GL_ARB_texture_cube_map_AVAIL query in the next line is only because the OpenGL 1.2 renderer
    // must also work without the cube-map extension, and I want to keep all TextureMapImpl.cpp files
    // identical across all OpenGL-based renderers.
    // Thus, the "&& cf::GL_ARB_texture_cube_map_AVAIL" is only required for the OpenGL 1.2. renderer and
    // could be removed for all others (where it is checked once in RendererImplT::IsSupported().
    if (InitCounter<RendererImplT::GetInstance().GetInitCounter() && cf::GL_ARB_texture_cube_map_AVAIL)
    {
        const GLint InternalFormat=(cf::GL_ARB_texture_compression_AVAIL && !MapComp.GetNoCompression()) ? GL_COMPRESSED_RGBA : GL_RGBA8;

        switch (Source)
        {
            case Files:
            case BitmapPtrExt:
            case BitmapPtrOwn:
            {
                glGenTextures(1, &OpenGLObject);
                glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, OpenGLObject);

                for (unsigned long SideNr=0; SideNr<6; SideNr++)
                {
                    if (Source==Files && !Bitmap[SideNr])
                    {
                        TextParserT TP(GetFullCubeMapString(MapComp.GetString(), SideNr).c_str(), "({[]}),", false);
                        Bitmap[SideNr]=MapCompositionT(TP, MapComp.GetBaseDir()).GetBitmap();
                    }

                    BitmapT* ScaledBitmap=NULL;
                    const unsigned long MaxSize=TextureMapManagerImplT::Get().GetMaxTextureSize();

                    // Scale to next smaller power of 2 and to or below the max. texture size.
                    if (isPowerOf2(Bitmap[SideNr]->SizeX) && isPowerOf2(Bitmap[SideNr]->SizeY) && Bitmap[SideNr]->SizeX<=MaxSize && Bitmap[SideNr]->SizeY<=MaxSize)
                    {
                        // The side lengths of Bitmap are already powers of 2 and at or below MaxSize - no need to scale.
                        ScaledBitmap=Bitmap[SideNr];
                    }
                    else
                    {
                        ScaledBitmap=new BitmapT(*Bitmap[SideNr]);

                        unsigned long NewX=1; while (2*NewX<=Bitmap[SideNr]->SizeX) NewX*=2;
                        unsigned long NewY=1; while (2*NewY<=Bitmap[SideNr]->SizeY) NewY*=2;

                        if (!MapComp.GetNoScaleDown())
                        {
                            // Note that NewX and NewY are now NOT treated independently of each other.
                            while (NewX>1 && NewY>1 && NewX>MaxSize && NewY>MaxSize)
                            {
                                NewX/=2;
                                NewY/=2;
                            }
                        }

                        ScaledBitmap->Scale(NewX, NewY);
                    }

                    if (MapComp.GetMinFilter()==MapCompositionT::Nearest || MapComp.GetMinFilter()==MapCompositionT::Linear)
                    {
                        glTexImage2D(CubeTargets[SideNr], 0, InternalFormat, ScaledBitmap->SizeX, ScaledBitmap->SizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, &ScaledBitmap->Data[0]);
                    }
                    else
                    {
                        // Note that the gluBuild2DMipmaps() function can fail, but we ignore any errors.
                        gluBuild2DMipmaps(CubeTargets[SideNr], InternalFormat, ScaledBitmap->SizeX, ScaledBitmap->SizeY, GL_RGBA, GL_UNSIGNED_BYTE, &ScaledBitmap->Data[0]);
                    }

                    if (ScaledBitmap!=Bitmap[SideNr])
                    {
                        delete ScaledBitmap;
                        ScaledBitmap=NULL;
                    }

                    // if (Source==Files && WantToUnloadBitmapAfterOpenGLObjGen)
                    // {
                    //     delete Bitmap[SideNr];
                    //     Bitmap[SideNr]=NULL;
                    // }
                }

                switch (MapComp.GetMinFilter())
                {
                    case MapCompositionT::Nearest:                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST               ); break;
                    case MapCompositionT::Linear:                 glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR                ); break;
                    case MapCompositionT::Nearest_MipMap_Nearest: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); break;
                    case MapCompositionT::Nearest_MipMap_Linear:  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR ); break;
                    case MapCompositionT::Linear_MipMap_Nearest:  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST ); break;
                    case MapCompositionT::Linear_MipMap_Linear:   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR  ); break;
                }

                switch (MapComp.GetMagFilter())
                {
                    case MapCompositionT::Nearest: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST); break;
                    default:                       glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); break;
                }

                switch (MapComp.GetWrapModeS())
                {
                    case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_REPEAT       ); break;
                    case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP        ); break;
                    case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); break;
                }

                switch (MapComp.GetWrapModeT())
                {
                    case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_REPEAT       ); break;
                    case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP        ); break;
                    case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); break;
                }
            }

            case RawPtrExt:
            case RawPtrOwn:
            {
                if (isPowerOf2(SizeX) && isPowerOf2(SizeY))
                {
                    glGenTextures(1, &OpenGLObject);
                    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, OpenGLObject);

                    for (unsigned long SideNr=0; SideNr<6; SideNr++)
                    {
                        if (MapComp.GetMinFilter()==MapCompositionT::Nearest || MapComp.GetMinFilter()==MapCompositionT::Linear)
                        {
                            glTexImage2D(CubeTargets[SideNr], 0, InternalFormat, SizeX, SizeY, 0, BytesPerPixel==3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, Data[SideNr]);
                        }
                        else
                        {
                            // Note that the gluBuild2DMipmaps() function can fail, but we ignore any errors.
                            gluBuild2DMipmaps(CubeTargets[SideNr], InternalFormat, SizeX, SizeY, BytesPerPixel==3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, Data[SideNr]);
                        }
                    }

                    switch (MapComp.GetMinFilter())
                    {
                        case MapCompositionT::Nearest:                glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST               ); break;
                        case MapCompositionT::Linear:                 glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR                ); break;
                        case MapCompositionT::Nearest_MipMap_Nearest: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST); break;
                        case MapCompositionT::Nearest_MipMap_Linear:  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR ); break;
                        case MapCompositionT::Linear_MipMap_Nearest:  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST ); break;
                        case MapCompositionT::Linear_MipMap_Linear:   glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR  ); break;
                    }

                    switch (MapComp.GetMagFilter())
                    {
                        case MapCompositionT::Nearest: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST); break;
                        default:              glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR ); break;
                    }

                    switch (MapComp.GetWrapModeS())
                    {
                        case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_REPEAT       ); break;
                        case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP        ); break;
                        case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); break;
                    }

                    switch (MapComp.GetWrapModeT())
                    {
                        case MapCompositionT::Repeat:      glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_REPEAT       ); break;
                        case MapCompositionT::Clamp:       glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP        ); break;
                        case MapCompositionT::ClampToEdge: glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); break;
                    }
                }
            }
        }

        InitCounter=RendererImplT::GetInstance().GetInitCounter();
    }

    return OpenGLObject;
}


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
    MaxTextureMapSize=MaxSize;
}


unsigned long TextureMapManagerImplT::GetMaxTextureSize() const
{
    return MaxTextureMapSize;
}


MatSys::TextureMapI* TextureMapManagerImplT::GetTextureMap2D(const MapCompositionT& MC)
{
    return GetTextureMap2DInternal(MC);
}


MatSys::TextureMapI* TextureMapManagerImplT::GetTextureMap2D(char* Data, unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    return GetTextureMap2DInternal(Data, SizeX, SizeY, BytesPerPixel, MakePrivateCopy, McForFiltersAndWrapping);
}


MatSys::TextureMapI* TextureMapManagerImplT::GetTextureMap2D(BitmapT* Bitmap, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    return GetTextureMap2DInternal(Bitmap, MakePrivateCopy, McForFiltersAndWrapping);
}


void TextureMapManagerImplT::FreeTextureMap(MatSys::TextureMapI* TM)
{
    FreeTextureMap((TextureMapImplT*)TM);
}


TextureMap2DT* TextureMapManagerImplT::GetTextureMap2DInternal(const MapCompositionT& MapComp_)
{
    for (unsigned long TMINr=0; TMINr<TexMapRepository.Size(); TMINr++)
        if (TexMapRepository[TMINr]->IsCreatedFromMapComp(MapComp_))
        {
            TexMapRepositoryCount[TMINr]++;
            return (TextureMap2DT*)TexMapRepository[TMINr];
        }

    TextureMap2DT* NewTMI=new TextureMap2DT(MapComp_);
    TexMapRepository.PushBack(NewTMI);
    TexMapRepositoryCount.PushBack(1);
    return NewTMI;
}


TextureMap2DT* TextureMapManagerImplT::GetTextureMap2DInternal(char* Data, unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    TextureMap2DT* NewTMI=new TextureMap2DT(Data, SizeX, SizeY, BytesPerPixel, MakePrivateCopy, McForFiltersAndWrapping);
    TexMapRepository.PushBack(NewTMI);
    TexMapRepositoryCount.PushBack(1);
    return NewTMI;
}


TextureMap2DT* TextureMapManagerImplT::GetTextureMap2DInternal(BitmapT* Bitmap, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    TextureMap2DT* NewTMI=new TextureMap2DT(Bitmap, MakePrivateCopy, McForFiltersAndWrapping);
    TexMapRepository.PushBack(NewTMI);
    TexMapRepositoryCount.PushBack(1);
    return NewTMI;
}


TextureMapCubeT* TextureMapManagerImplT::GetTextureMapCubeInternal(const MapCompositionT& MapComp_)
{
    for (unsigned long TMINr=0; TMINr<TexMapRepository.Size(); TMINr++)
        if (TexMapRepository[TMINr]->IsCreatedFromMapComp(MapComp_))
        {
            TexMapRepositoryCount[TMINr]++;
            return (TextureMapCubeT*)TexMapRepository[TMINr];
        }

    TextureMapCubeT* NewTMI=new TextureMapCubeT(MapComp_);
    TexMapRepository.PushBack(NewTMI);
    TexMapRepositoryCount.PushBack(1);
    return NewTMI;
}


TextureMapCubeT* TextureMapManagerImplT::GetTextureMapCubeInternal(char* Data[6], unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    TextureMapCubeT* NewTMI=new TextureMapCubeT(Data, SizeX, SizeY, BytesPerPixel, MakePrivateCopy, McForFiltersAndWrapping);
    TexMapRepository.PushBack(NewTMI);
    TexMapRepositoryCount.PushBack(1);
    return NewTMI;
}


TextureMapCubeT* TextureMapManagerImplT::GetTextureMapCubeInternal(BitmapT* Bitmap[6], bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)
{
    TextureMapCubeT* NewTMI=new TextureMapCubeT(Bitmap, MakePrivateCopy, McForFiltersAndWrapping);
    TexMapRepository.PushBack(NewTMI);
    TexMapRepositoryCount.PushBack(1);
    return NewTMI;
}


void TextureMapManagerImplT::FreeTextureMap(TextureMapImplT* TM)
{
    if (TM==NULL) return;

    for (unsigned long TMINr=0; TMINr<TexMapRepository.Size(); TMINr++)
        if (TexMapRepository[TMINr]==TM)
        {
            TexMapRepositoryCount[TMINr]--;

            if (TexMapRepositoryCount[TMINr]==0)
            {
                delete TM;

                TexMapRepository.RemoveAt(TMINr);
                TexMapRepositoryCount.RemoveAt(TMINr);
            }

            break;
        }
}
