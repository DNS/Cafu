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

#ifndef _CF_SCENEGRAPH_SHLMAP_MANAGER_HPP_
#define _CF_SCENEGRAPH_SHLMAP_MANAGER_HPP_

#include "Templates/Array.hpp"

#include <fstream>

namespace MatSys
{
    class TextureMapI;
}


namespace cf
{
    namespace SceneGraph
    {
        /// This class manages SHL maps, e.g. by "allocating" rectangular areas in larger coefficient maps.
        class SHLMapManT
        {
            public:

            static const unsigned long SIZE_S;
            static const unsigned long SIZE_T;

            static char         NrOfBands;      ///< Number n of bands for which we have computed and stored coefficients (n^2 many).
            static unsigned int NrOfRepres;     ///< Number of representatives of SH vectors. If >0, the SHL coeffs are compressed, uncompressed otherwise.

            struct SHLMapT
            {
                /// Constructor.
                SHLMapT();

                /// This should actually be   ArrayT< ArrayT<float> > Coeffs;   that is, for each SHLMap element,
                /// store an array of coefficients. However, this adds the overhead of the internal 'ArrayT'
                /// management data to the storage for each element, and thus we prefer the "flat" approach.
                ArrayT<float> Coeffs;

                /// For compressed SHL coeffs, this array contains the index for each map element into the bigger lookup-table.
                /// That is, if NrOfRepres>0, then Indices.Size()==SIZE_S*SIZE_T, and 0 otherwise.
                ArrayT<unsigned short> Indices;
            };


            /// The constructor.
            SHLMapManT();

            /// The destructor.
            ~SHLMapManT();

            /// Reads the lookup-table the indices are referring to (nothing is written when NrOfRepres==0 (uncompressed data)).
            void ReadSHLCoeffsTable(std::istream& InFile);

            /// Write the lookup-table the indices are referring to (nothing is written when NrOfRepres==0 (uncompressed data)).
            /// This writes a total of NrOfRepres*NR_OF_SH_COEFFS coefficients.
            void WriteSHLCoeffsTable(std::ostream& OutFile) const;

            /// Finds a position for a rectangular SHLMap within SHLMaps[SHLMaps.Size()-1].Coeff.
            /// If no such position exists, a new, empty SHLMapT is created and appended to the SHLMaps.
            /// Returns true on success, false on failure (i.e. if SizeS>MAX_SIZE_S || SizeT>MAX_SIZE_T).
            bool Allocate(unsigned long SizeS, unsigned long SizeT, unsigned long& BitmapNr, unsigned long& PosS, unsigned long& PosT);

            /// Initializes the MatSys textures.
            void InitTextures();


            ArrayT<SHLMapT*>             SHLMaps;
            ArrayT<float>                SHLCoeffsTable; ///< The lookup-table of representatives (NrOfRepres * (NrOfBands^2) many) the SHLMaps::Indices refer to.

            /// For uncompressed SHL coeffs, for each SHLMap we store an array of texture objects,
            /// where each texture contains four SHL coeffs (in its RGBA components).
            /// Thus, for 16 SHL coeffs (uncompressed), we keep 16/4 == 4 textures per SHLMap.
            /// For compressed SHL coeffs, each SHLMap only requires a single texture object
            /// (that is, the size of the inner arrays is always 1), because in this case, we only
            /// store SHL vector *indices* (16 bit precision) that only require the R and G components
            /// (8 bits each) of a single texture.
            ArrayT< ArrayT<MatSys::TextureMapI*> > SHLMapTextures;

            // For compressed SHL data, this is the table of representatives.
            // Unused when uncompressed SHL data is used.
            MatSys::TextureMapI*          SHLRepresentativesTexture;
            unsigned long                 SHLRepresentativesTexWidth;


            private:

            SHLMapManT(const SHLMapManT&);          ///< Use of the Copy Constructor    is not allowed.
            void operator = (const SHLMapManT&);    ///< Use of the Assignment Operator is not allowed.

            bool AllocateHelper(unsigned long SizeS, unsigned long SizeT, unsigned long& PosS, unsigned long& PosT);

            ArrayT<unsigned long> BitmapAllocated;
        };
    }
}

#endif
