/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CF_SCENEGRAPH_AUX_HPP_
#define _CF_SCENEGRAPH_AUX_HPP_

#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"

#ifdef _WIN32
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif

#include <fstream>
#include <map>
#include <string>


namespace cf
{
    namespace SceneGraph
    {
        namespace aux
        {
            /// Reads an "int32_t" from a std::istream.
            int32_t ReadInt32(std::istream& InFile);

            /// Reads an "uint16_t" from a std::istream.
            uint16_t ReadUInt16(std::istream& InFile);

            /// Reads an "uint32_t" from a std::istream.
            uint32_t ReadUInt32(std::istream& InFile);

            /// Reads a "float" from a std::istream.
            float ReadFloat(std::istream& InFile);

            /// Reads a "double" from a std::istream.
            double ReadDouble(std::istream& InFile);

            /// Reads a std::string from a std::istream.
            std::string ReadString(std::istream& InFile);

            /// Reads a Vector3dT from a std::istream.
            Vector3dT ReadVector3d(std::istream& InFile);

            /// Reads a Vector3fT from a std::istream.
            Vector3fT ReadVector3f(std::istream& InFile);


            /// Writes an "int32_t" into a std::ostream.
            void Write(std::ostream& OutFile, int32_t i);

            /// Writes an "uint16_t" into a std::ostream.
            void Write(std::ostream& OutFile, uint16_t ui);

            /// Writes an "uint32_t" into a std::ostream.
            void Write(std::ostream& OutFile, uint32_t ui);

            /// Writes a "float" into a std::ostream.
            void Write(std::ostream& OutFile, float f);

            /// Writes a "double" into a std::ostream.
            void Write(std::ostream& OutFile, double d);

            /// Writes a std::string into a std::ostream.
            void Write(std::ostream& OutFile, const std::string& Str);

            /// Writes a Vector3dT into a std::ostream.
            void Write(std::ostream& OutFile, const Vector3dT& v);

            /// Writes a Vector3fT into a std::ostream.
            void Write(std::ostream& OutFile, const Vector3fT& v);


            /// Converts the given ptrdiff_t to int32_t and checks for overflow caused by the conversion ("cnc" is short for "cast and check").
            /// On ILP32 (and LLP64) systems, both ptrdiff_t and int32_t are int or long int. Thus we have sizeof(ptrdiff_t)==sizeof(int32_t)==4,
            ///     so the conversion is trivial and overflow cannot occur.
            /// On LP64 (quasi all 64-bit Unix and Linux) systems, sizeof(ptrdiff_t)==8 and sizeof(int32_t)==4, so the cast may lose data.
            ///     On overflow, the function triggers an assertion in debug builds and then throws an std::overflow_error in all builds.
            int32_t cnc32(ptrdiff_t d);

            /// Just like cnc32(long int i), but for unsigned long ints.
            uint32_t cnc32(unsigned long int ui);


            class PoolT
            {
                public:

                struct LessVector3d
                {
                    // See "Die C++ Programmiersprache" by Bjarne Stroustrup pages 498 and 510 and
                    // Scott Meyers "Effective STL" Item 21 for more information about this struct.
                    bool operator () (const Vector3dT& v1, const Vector3dT& v2) const
                    {
                        if (v1.x < v2.x) return true;
                        if (v1.x > v2.x) return false;

                        if (v1.y < v2.y) return true;
                        if (v1.y > v2.y) return false;

                        return v1.z < v2.z;
                    }
                };

                struct LessVector3f
                {
                    // See "Die C++ Programmiersprache" by Bjarne Stroustrup pages 498 and 510 and
                    // Scott Meyers "Effective STL" Item 21 for more information about this struct.
                    bool operator () (const Vector3fT& v1, const Vector3fT& v2) const
                    {
                        if (v1.x < v2.x) return true;
                        if (v1.x > v2.x) return false;

                        if (v1.y < v2.y) return true;
                        if (v1.y > v2.y) return false;

                        return v1.z < v2.z;
                    }
                };

                std::string ReadString(std::istream& InFile);
                Vector3T<double> ReadVector3d(std::istream& InFile);
                Vector3T<float> ReadVector3f(std::istream& InFile);

                void Write(std::ostream& OutFile, const std::string& s);
                void Write(std::ostream& OutFile, const Vector3T<double>& v);
                void Write(std::ostream& OutFile, const Vector3T<float>& v);


                private:

                // Helper containers when the PoolT is used for reading.
                ArrayT< std::string >      ReadStrings;
                ArrayT< Vector3T<double> > ReadVectors3d;
                ArrayT< Vector3T<float>  > ReadVectors3f;

                // Helper containers when the PoolT is used for writing.
                std::map<std::string, uint32_t              > WriteStrings;
                std::map<Vector3dT,   uint32_t, LessVector3d> WriteVectors3d;
                std::map<Vector3fT,   uint32_t, LessVector3f> WriteVectors3f;
            };
        }
    }
}

#endif
