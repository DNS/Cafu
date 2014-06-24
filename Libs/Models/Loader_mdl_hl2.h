/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_HL2_MDL_FILE_FORMAT_HPP_INCLUDED
#define CAFU_HL2_MDL_FILE_FORMAT_HPP_INCLUDED

#include "Math3D/Quaternion.hpp"
#include "Math3D/Vector3.hpp"

#if defined(_WIN32)
    // We are on the Win32 platform.
    #if defined(__WATCOMC__)
        // Using the OpenWatcom C/C++ compiler.
        #pragma off(unreferenced);
        #define WATCOM_PACKED _Packed
        #define GCC_PACKED
    #elif defined(_MSC_VER)
        // Using the Microsoft Visual C++ compiler.
        #pragma pack(push, 1)
        #define WATCOM_PACKED
        #define GCC_PACKED
    #else
        #error Using unknown compiler on the Win32 platform.
    #endif
#elif __linux__
    // We are on the Linux platform.
    #if __GNUG__    // This is equivalent to testing (__GNUC__ && __cplusplus).
        // Using the g++ compiler.
        // See http://www.delorie.com/gnu/docs/gcc/cpp_toc.html for documentation about the C preprocessor.
        #define WATCOM_PACKED
        #define GCC_PACKED __attribute__ ((packed))
    #else
        #error Using unknown compiler on the Linux platform.
    #endif
#else
    #error Compiling on unknown platform.
#endif




#if defined(_WIN32) && defined (_MSC_VER)
    #pragma pack(pop)
#endif

#endif
