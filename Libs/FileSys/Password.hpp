/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_PASSWORD_HPP_INCLUDED
#define CAFU_PASSWORD_HPP_INCLUDED

#include <string>
#include "Templates/Array.hpp"


namespace cf
{
    namespace Password
    {
        /// Generates a string that consists only of ASCII characters.
        /// The reason for the ASCII characters is that they are assumed to be safe to use and enter on any system.
        /// @param Length       The length of the password string to be generated.
        /// @param RandomSeed   The seed for the random number generator.
        /// @returns The generated password string.
        ArrayT<unsigned char> GenerateAsciiPassword(unsigned long Length, unsigned long RandomSeed=0);

        /// Generates a string that the password generator code uses to obfuscate the cleartext password
        /// and that the engine uses to de-obfuscate the obfuscated password.
        ArrayT<unsigned char> GenerateObfuscationString(unsigned long Length);

        /// Returns the component-wise XOR of String1 and String2.
        ArrayT<unsigned char> XOR(const ArrayT<unsigned char>& String1, const ArrayT<unsigned char>& String2);

        /// Returns the values of String as C-code for initializing an array.
        std::string GenerateArrayCode(const ArrayT<unsigned char>& String);

        /// Converts an array of unsigned char to a std::string.
        std::string ToString(const ArrayT<unsigned char>& String);
    }
}

#endif
