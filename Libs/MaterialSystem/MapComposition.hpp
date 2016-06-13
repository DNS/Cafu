/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATSYS_MAP_COMPOSITION_HPP_INCLUDED
#define CAFU_MATSYS_MAP_COMPOSITION_HPP_INCLUDED

#include "TextParser/TextParser.hpp"


struct BitmapT;


/// A MapCompositionT is a description of how a SINGLE texture map image is composited from several source images on disk.
/// (Each material stage has an own MatCompositionT. Each frame of an animated sequence, too. Don't know yet about videos.)
class MapCompositionT
{
    public:

    /// This enum describes the filter mode of this map composition, kept in the root MapCompositionT (the children just keep the default values).
    enum MinMagFiltersT { Nearest, Linear, Nearest_MipMap_Nearest, Nearest_MipMap_Linear, Linear_MipMap_Nearest, Linear_MipMap_Linear };

    /// This enum describes the wrap mode of this map composition, kept in the root MapCompositionT (the children just keep the default values).
    enum WrapModesT { Repeat, Clamp, ClampToEdge };

    /// This enum describes the type of the map composition.
    /// Note that there is no scaling operation - scaling is automatic.
    /// TODO: Operations for the alpha-channel: e.g. taking it from the red channel of another image...
    enum TypeT { Empty, Map, Add, Mul, CombineNormals, HeightMapToNormalMap, FlipNormalMapYAxis, ReNormalize, BlueToAlpha };    // VideoStream


    /// Constructor for creating an "empty" map composition.
    MapCompositionT(MinMagFiltersT MinFilter_=Linear_MipMap_Linear, MinMagFiltersT MagFilter_=Linear, WrapModesT WrapS_=Repeat, WrapModesT WrapT_=Repeat, bool NoScaleDown_=false, bool NoCompression_=false);

    /// Constructor for creating a MapCompositionT from a string description.
    MapCompositionT(const std::string& s, const std::string& BaseDir_) /*throw (TextParserT::ParseError)*/;

    /// Constructor for creating a MapCompositionT from TextParserT input.
    MapCompositionT(TextParserT& TP, const std::string& BaseDir_, bool NoCompression_=false, const unsigned long RecursionCount=0) /*throw (TextParserT::ParseError)*/;

    /// Copy Constructor (Law of the Big Three).
    MapCompositionT(const MapCompositionT& Source);

    /// Destructor (Law of the Big Three).
    ~MapCompositionT();

    /// Assignment Operator (Law of the Big Three).
    MapCompositionT& operator = (const MapCompositionT& Source);

    /// Equal Operator. Treats Add, Mul and CombineNormals as commutative (a+b==b+a),
    /// but never takes the associative or distributive law into account.
    bool operator == (const MapCompositionT& rhs) const;

    /// Returns true iff this MapComposition is empty.
    bool IsEmpty() const { return Type==Empty; };

    /// Returns one of the children of this MapCompositionT.
    /// Use this method together with GetType().
    const MapCompositionT* GetChild(int Num) const { return Num==0 ? Child1 : Child2; }

    MinMagFiltersT GetMinFilter() const { return MinFilter; }       ///< Returns the filter for minification.
    MinMagFiltersT GetMagFilter() const { return MagFilter; }       ///< Returns the filter for magnification.
    WrapModesT     GetWrapModeS() const { return WrapS; }           ///< Returns the wrapping mode in s-direction.
    WrapModesT     GetWrapModeT() const { return WrapT; }           ///< Returns the wrapping mode in t-direction.
    bool           GetNoScaleDown() const { return NoScaleDown; }   ///< Returns whether the texture should not be scaled down (e.g. for optimizing performance).
    bool           GetNoCompression() const { return NoCompression; }   ///< Returns whether the texture should not be compressed (e.g. for optimizing performance/memory).
    TypeT          GetType() const { return Type; }                 ///< Returns the type of this map composition.

    /// This function loads all image source files from disk (at *BaseDir+FileName), and combines them (according to this MapCompositionT)
    /// into a single resulting BitmapT. On any error with the participating bitmaps (ie. file not found, file unreadable, ...),
    /// a default texture is substituted for the missing participant, and a warning is printed out. Thus, the function never fails.
    /// The caller becomes the owner of the returned pointer (i.e. its the callers responsibility to delete it.)
    BitmapT* GetBitmap() const;

    /// Returns a string description of this MapCompositionT (quasi the counter-piece to the constructor).
    std::string GetString() const;

    /// Like GetString(), which is kept for backwards-compatibility, but includes the list of options as well.
    std::string GetStringWithOptions(bool NoCompressionDefault=false) const;

    /// Returns the base dir of this MapCompositionT. Can be the empty string for empty map compositions.
    std::string GetBaseDir() const;


    private:

    /// As each MapCompositionT needs a base directory, this is the cache for the base directories.
    /// MapCompositionTs store pointers to cache objects rather than array indices, in order to be able to
    /// later copy MapCompositionTs even across exe/dll-boundaries, if need be. May later reconsider this, though...
    /// Currently, the cache is not intended to be cleared during program run-time, the OS does the clean-up after program termination.
    static ArrayT<std::string*> BaseDirCache;

    /// An auxiliary method for the constructors.
    void Init(TextParserT& TP, const unsigned long RecursionCount) /*throw (TextParserT::ParseError)*/;


    TypeT            Type;          ///< The type of this MapCompositionT.
    MinMagFiltersT   MinFilter;     ///< The minification  filter   (valid only for the root MapCompositionT).
    MinMagFiltersT   MagFilter;     ///< The magnification filter   (valid only for the root MapCompositionT).
    WrapModesT       WrapS;         ///< The s-coordinate wrap mode (valid only for the root MapCompositionT).
    WrapModesT       WrapT;         ///< The t-coordinate wrap mode (valid only for the root MapCompositionT).
    bool             NoScaleDown;   ///< The "user wishes no scale down" flag (valid only for the root MapCompositionT). Prevents the bitmap image from being scaled down by the Renderer when the user optimizes the game graphics. (That is, this is for overriding e.g. performance optimizations.) Useful for fonts, HUD, lightmaps, and everything else that must not get mixed up.
    bool             NoCompression; ///< The "no texture compression" flag (valid only for the root MapCompositionT). Prevents the Renderer from applying texture compression to this bitmap image when the user optimizes the game graphics. (That is, this is for overriding e.g. performance optimizations.) Useful e.g. for normal-maps, where the memory gains are outweighed by the compression artifacts.
    std::string      FileName;      ///< The map filename                            (valid only for Type==Map).
    std::string*     BaseDir;       ///< The base dir corresponding to the file name (valid only for Type==Map).
    float            HeightScale;   ///< The scale for the heightmap (valid only for Type==HeightMapToNormalMap).
    MapCompositionT* Child1;        ///< The first  child for most types except Empty and Map.
    MapCompositionT* Child2;        ///< The second child for most types except Empty and Map.
};

#endif
