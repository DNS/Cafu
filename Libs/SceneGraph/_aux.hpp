/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCENEGRAPH_AUX_HPP_INCLUDED
#define CAFU_SCENEGRAPH_AUX_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"

#if defined(_WIN32) && _MSC_VER<1600
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
            void Write(std::ostream& OutFile, const Vector3T<double>& v);

            /// Writes a Vector3fT into a std::ostream.
            void Write(std::ostream& OutFile, const Vector3T<float>& v);


            /// This function casts the given integer i to an int32_t, and checks that the returned int32_t has the same value as i.
            /// If the check fails (the value of i overflows the int32_t), the function triggers an assertion in debug builds and
            /// throws an std::overflow_error in all builds.
            /// The function is intended for use in cross-platform (ILP32, LLP64, LP64) serialization code where sizeof(i) can be 4 or 8,
            /// and i should be written as a 4-byte value in order to be readable on all platforms.
            template<class T> int32_t cnc_i32(T i)
            {
                assert(sizeof(i)>=4);   // If this ever fails, it is more a curiosity than a real issue.

                const int32_t i32=static_cast<int32_t>(i);

                assert(i32==i);
                if (i32!=i) throw std::overflow_error("The cast in aux::cnc_i32() caused a loss of data.");

                return i32;
            }

            /// Just like cnc_i32(), but for unsigned integers that are cast to uint32_t.
            template<class T> uint32_t cnc_ui32(T ui)
            {
                assert(sizeof(ui)>=4);  // If this ever fails, it is more a curiosity than a real issue.

                const uint32_t ui32=static_cast<uint32_t>(ui);

                assert(ui32==ui);
                if (ui32!=ui) throw std::overflow_error("The cast in aux::cnc_ui32() caused a loss of data.");

                return ui32;
            }


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
