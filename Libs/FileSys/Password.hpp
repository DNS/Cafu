/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
