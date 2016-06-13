/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*******************/
/*** Texture Map ***/
/*******************/

#ifndef CAFU_MATSYS_TEXTUREMAP_HPP_INCLUDED
#define CAFU_MATSYS_TEXTUREMAP_HPP_INCLUDED


struct BitmapT;
class  MapCompositionT;


namespace MatSys
{
    /// This is an interface to a texture-map.
    /// Texture maps are only ever created by the texture map manager.
    class TextureMapI
    {
        public:

        // Needed by some user code for computing the texture s/t-coords.
        // REALLY? Shouln't the user code simply get the X/Y dims somehow directly from the *material*?!?
        virtual unsigned int GetSizeX()=0;
        virtual unsigned int GetSizeY()=0;

        virtual ~TextureMapI() { };
    };


    /// This is an interface to the texture-map manager.
    /// The interface is specified as ABC in order to share the texture-map manager across exe/dll boundaries.
    class TextureMapManagerI
    {
        public:

        /// Sets the maximum side length to which textures should be scaled down before they are employed for rendering.
        /// The unscaled data is kept in memory, though, even if the TextureMapManager is the owner of the texture data.
        /// The value that is set here is only effective when a texture must be re-initialized for rendering
        /// (e.g. after an OpenGL rendering-context change) and normally has no effect at other times.
        virtual void SetMaxTextureSize(unsigned long MaxSize)=0;

        /// Returns the currently set maximum texture size.
        virtual unsigned long GetMaxTextureSize() const=0;

        /// Creates a 2D texture-map by a texture-map composition. The function never fails.
        /// Calling this multiple times with the same MapComp will return identical pointers.
        /// The maximum texture size value is respected, unless the "NoScaleDown" property is set in the MapComp.
        virtual TextureMapI* GetTextureMap2D(const MapCompositionT& MapComp)=0;

        /// Creates a 2D texture-map from a pointer. The function never fails.
        /// Calling this multiple times with identical paramaters will each time return a different pointer!
        /// If MakePrivateCopy=true, the function makes a private copy of the data pointed to by Data. The caller can then free the original data.
        /// If MakePrivateCopy=false, the function relies on the Data being valid and available during the entire lifetime of the returned texture map.
        /// SizeX and SizeY MUST be powers of 2, and BytesPerPixel MUST be 3 or 4!
        /// Moreover, all rows must be aligned on 4-byte boundaries! (See e.g. OpenGL Programming Guide (Red Book), p. 311.)
        /// The maximum texture size value is NOT respected - no scaling is performed even if SizeX or SizeY exceed this value!
        virtual TextureMapI* GetTextureMap2D(char* Data, unsigned long SizeX, unsigned long SizeY, char BytesPerPixel, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)=0;

        /// Creates a 2D texture-map from a BitmapT. The function never fails.
        /// Calling this multiple times with identical paramaters will each time return a different pointer!
        /// If MakePrivateCopy=true, the function makes a private copy of the data pointed to by Data. The caller can then free the original data.
        /// If MakePrivateCopy=false, the function relies on the Bitmap being valid and available during the entire lifetime of the returned texture map.
        /// The maximum texture size value is respected, unless the "NoScaleDown" property is set in the McForFiltersAndWrapping.
        virtual TextureMapI* GetTextureMap2D(BitmapT* Bitmap, bool MakePrivateCopy, const MapCompositionT& McForFiltersAndWrapping)=0;

        /// Releases the texture map from the texture manager, and releases all of its resources.
        virtual void FreeTextureMap(TextureMapI* TM)=0;

        /// Virtual destructor, so that nothing can go wrong and even g++ is happy.
        virtual ~TextureMapManagerI() { }
    };


    /// A global pointer to the current texture-map manager, for common access by all modules that use the MatSys.
    /// Just set this after you loaded the desired renderer DLL to the pointer returned by the DLLs GetTextureMapManager() function.
    /// (And NULL it on unloading the DLL.)
    /// An analogous object exists for the Renderer interface, see Renderer.hpp.
    extern TextureMapManagerI* TextureMapManager;
}

#endif
