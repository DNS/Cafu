/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCENEGRAPH_LIGHTMAP_MANAGER_HPP_INCLUDED
#define CAFU_SCENEGRAPH_LIGHTMAP_MANAGER_HPP_INCLUDED

#include "Templates/Array.hpp"

struct BitmapT;

namespace MatSys
{
    class TextureMapI;
}


namespace cf
{
    namespace SceneGraph
    {
        /// This class manages lightmaps, e.g. by "allocating" rectangular areas in larger bitmaps.
        class LightMapManT
        {
            public:

            static const unsigned int SIZE_S;
            static const unsigned int SIZE_T;
            static const unsigned int INIT_COLOR1;
            static const unsigned int INIT_COLOR2;

            /// The constructor.
            LightMapManT();

            /// The destructor.
            ~LightMapManT();

            /// Finds a position for a rectangular lightmap within LightMaps[LightMaps.Size()-1].Data.
            /// If no such position exists, a new, empty LightMapT is created and appended to the LightMaps.
            /// Returns true on success, false on failure (i.e. if SizeS>MAX_SIZE_S || SizeT>MAX_SIZE_T).
            bool Allocate(unsigned int SizeS, unsigned int SizeT, unsigned long& BitmapNr, unsigned int& PosS, unsigned int& PosT);

            /// Initializes the MatSys textures in the Textures and Textures2 arrays.
            /// Gamma correction by value Gamma is applied to the textures in Textures, but not in Textures2.
            void InitTextures(const float Gamma, const int AmbientR, const int AmbientG, const int AmbientB);


            ArrayT<BitmapT*>             Bitmaps;
            ArrayT<BitmapT*>             Bitmaps2;
            ArrayT<MatSys::TextureMapI*> Textures;
            ArrayT<MatSys::TextureMapI*> Textures2;


            private:

            LightMapManT(const LightMapManT&);      ///< Use of the Copy Constructor    is not allowed.
            void operator = (const LightMapManT&);  ///< Use of the Assignment Operator is not allowed.

            bool AllocateHelper(unsigned int SizeS, unsigned int SizeT, unsigned int& PosS, unsigned int& PosT);

            ArrayT<unsigned int> BitmapAllocated;
        };
    }
}

#endif
