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

// Required for #include <GL/gl.h> with MS VC++.
#if defined(_WIN32) && defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <GL/gl.h>

#include "../MapComposition.hpp"
#include "../TextureMap.hpp"


/// This class represents a texture-map.
class TextureMapImplT : public MatSys::TextureMapI
{
    public:

    // The TextureMapI interface is not repeated here.

    /// Returns true if this texture was created from a map composition that was "equivalent" to MC_.
    virtual bool IsCreatedFromMapComp(const MapCompositionT& MC_)=0;

    /// This function returns an OpenGL object for this texture.
    virtual GLuint GetOpenGLObject()=0;

    /// Virtual destructor. See the class design diagram and the C++ FAQ 21.05 for more information.
    /// (We will delete derived classes via pointers to TextureMapImplT.)
    virtual ~TextureMapImplT() {}
};


/// This class represents a 2D texture-map.
class TextureMap2DT : public TextureMapImplT
{
    private:

    TextureMap2DT(const TextureMap2DT&);        // Use of the Copy    Constructor is not allowed.
    void operator = (const TextureMap2DT&);     // Use of the Assignment Operator is not allowed.

    enum SourceT { MC, RawPtrExt, RawPtrOwn, BitmapPtrExt, BitmapPtrOwn };

    SourceT         Source;

    MapCompositionT MapComp;        ///< Used for Source==MC (and for *ALL* source types for the min/mag filters and wrapping mode).
    BitmapT*        Bitmap;         ///< Used for Source==MC or Source==BitmapPtr*

    char*           Data;           ///< Used for Source==RawPtr*
    unsigned long   SizeX;          ///< Used for Source==RawPtr*
    unsigned long   SizeY;          ///< Used for Source==RawPtr*
    char            BytesPerPixel;  ///< Used for Source==RawPtr*

    GLuint          OpenGLObject;
    unsigned long   InitCounter;    ///< Do we have to re-upload the Bitmap?


    public:

    // TextureMapI implementation.
    // Needed by some user code for computing the texture s/t-coords.
    unsigned int GetSizeX();
    unsigned int GetSizeY();


    // TextureMapImplT implementation.
    bool IsCreatedFromMapComp(const MapCompositionT& MC_);
    GLuint GetOpenGLObject();
    ~TextureMap2DT();


    // Internal interface.
    // Constructors.
    TextureMap2DT(const MapCompositionT& MapComp_);
    TextureMap2DT(char* Data_, unsigned long SizeX_, unsigned long SizeY_, char BytesPerPixel_, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);
    TextureMap2DT(BitmapT* Bitmap_, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);
};


/// This class represents a cube texture-map.
class TextureMapCubeT : public TextureMapImplT
{
    private:

    TextureMapCubeT(const TextureMapCubeT&);    // Use of the Copy    Constructor is not allowed.
    void operator = (const TextureMapCubeT&);   // Use of the Assignment Operator is not allowed.

    static const GLenum      CubeTargets[6];
    static const std::string CubeSuffixes[6];

    static std::string GetFullCubeMapString(std::string BaseString, unsigned long SuffixNr);

    enum SourceT { Files, RawPtrExt, RawPtrOwn, BitmapPtrExt, BitmapPtrOwn };

    SourceT         Source;

    MapCompositionT MapComp;        ///< Used for all source types for the min/mag filters and wrapping modes. If Source==Files, MapComp.GetString() is used as the cube map base name (with '#' placeholders).
    BitmapT*        Bitmap[6];      ///< Used for Source==Files or Source==BitmapPtr*

    char*           Data[6];        ///< Used for Source==RawPtr*
    unsigned long   SizeX;          ///< Used for Source==RawPtr*
    unsigned long   SizeY;          ///< Used for Source==RawPtr*
    char            BytesPerPixel;  ///< Used for Source==RawPtr*

    GLuint          OpenGLObject;
    unsigned long   InitCounter;    ///< Do we have to re-upload the Bitmap?


    public:

    // TextureMapI implementation.
    // Needed by some user code for computing the texture s/t-coords.
    unsigned int GetSizeX();
    unsigned int GetSizeY();


    // TextureMapImplT implementation.
    bool IsCreatedFromMapComp(const MapCompositionT& MC_);
    GLuint GetOpenGLObject();
    ~TextureMapCubeT();


    // Internal interface.
    // Constructors.
    TextureMapCubeT(const MapCompositionT& MapComp_);
    TextureMapCubeT(char* Data_[6], unsigned long SizeX_, unsigned long SizeY_, char BytesPerPixel_, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);
    TextureMapCubeT(BitmapT* Bitmap_[6], bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);
};


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

    /// Creates a 2D texture-map by a texture-map composition. The function never fails.
    /// Calling this multiple times with the same MapComp will return identical pointers.
    TextureMap2DT* GetTextureMap2DInternal(const MapCompositionT& MapComp);

    /// Creates a 2D texture-map from a pointer. The function never fails.
    /// Calling this multiple times with identical paramaters will each time return a different pointer!
    /// If MakePrivateCopy=true, the function makes a private copy of the data pointed to by Data. The caller can then free the original data.
    /// If MakePrivateCopy=false, the function relies on the Data being valid and available during the entire lifetime of the returned texture map.
    /// SizeX and SizeY MUST be powers of 2, and BytesPerPixel MUST be 3 or 4!
    TextureMap2DT* GetTextureMap2DInternal(char* Data, unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);

    /// Creates a 2D texture-map from a BitmapT. The function never fails.
    /// Calling this multiple times with identical paramaters will each time return a different pointer!
    /// If MakePrivateCopy=true, the function makes a private copy of the data pointed to by Data. The caller can then free the original data.
    /// If MakePrivateCopy=false, the function relies on the Bitmap being valid and available during the entire lifetime of the returned texture map.
    TextureMap2DT* GetTextureMap2DInternal(BitmapT* Bitmap, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);

    /// Creates a cube texture-map by a texture-map composition. The function never fails.
    /// Calling this multiple times with the same MapComp will return identical pointers.
    TextureMapCubeT* GetTextureMapCubeInternal(const MapCompositionT& MapComp);

    /// Creates a cube texture-map from a pointer. The function never fails.
    /// Calling this multiple times with identical paramaters will each time return a different pointer!
    /// If MakePrivateCopy=true, the function makes a private copy of the data pointed to by Data. The caller can then free the original data.
    /// If MakePrivateCopy=false, the function relies on the Data being valid and available during the entire lifetime of the returned texture map.
    /// SizeX and SizeY MUST be powers of 2, and BytesPerPixel MUST be 3 or 4!
    TextureMapCubeT* GetTextureMapCubeInternal(char* Data[6], unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);

    /// Creates a cube texture-map from a BitmapT. The function never fails.
    /// Calling this multiple times with identical paramaters will each time return a different pointer!
    /// If MakePrivateCopy=true, the function makes a private copy of the data pointed to by Data. The caller can then free the original data.
    /// If MakePrivateCopy=false, the function relies on the Bitmap being valid and available during the entire lifetime of the returned texture map.
    TextureMapCubeT* GetTextureMapCubeInternal(BitmapT* Bitmap[6], bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping);

    /// Releases the texture map from the texture manager, and all its resources.
    void FreeTextureMap(TextureMapImplT* TM);

    /// Returns a reference to the texture-map repository.
    const ArrayT<TextureMapImplT*>& GetTexMapRepository() const { return TexMapRepository; }


    /// Get a pointer/reference to the texture-map manager singleton.
    static TextureMapManagerImplT& Get();


    private:

    TextureMapManagerImplT() : MaxTextureMapSize(4096) { }

    TextureMapManagerImplT(const TextureMapManagerImplT&);      // Use of the Copy    Constructor is not allowed.
    void operator = (const TextureMapManagerImplT&);            // Use of the Assignment Operator is not allowed.

    ArrayT<TextureMapImplT*> TexMapRepository;
    ArrayT<unsigned long>    TexMapRepositoryCount;
    unsigned long            MaxTextureMapSize;
};

#endif
