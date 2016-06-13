/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Password.hpp"
#include "../../ExtLibs/MersenneTwister.h"
#include <cassert>


ArrayT<unsigned char> cf::Password::GenerateAsciiPassword(unsigned long Length, unsigned long RandomSeed)
{
    ArrayT<unsigned char> Result;

    MTRand mtr(RandomSeed);

    while (Result.Size()<Length)
    {
        unsigned char AsciiChar=(unsigned char)mtr.randInt();

        if (AsciiChar< 35) continue;
        if (AsciiChar>125) continue;

        Result.PushBack(AsciiChar);
    }

    return Result;
}


// Note that this method MUST generate the same random result on ALL platforms and build modes etc.
// Therefore, the implementation must not use the srand() and rand() functions of the C standard libaries,
// whose implemenetation differs among platforms and who therefore generate different random sequences on
// different platforms. In this bad case, proper password de-obfuscation is not possible,
// which for example means that encrypted zip archives cannot be opened on that platform.
ArrayT<unsigned char> cf::Password::GenerateObfuscationString(unsigned long Length)
{
    ArrayT<unsigned char> Result;
    std::string           SomeString("Imagination is more important than knowledge. For knowledge is limited. -- Albert Einstein");

    // Well, they'll probably figure this out if they really want to... but it's better than nothing.
    MTRand mtr(0x19780112);

    while (Result.Size()<Length)
    {
        unsigned char AsciiChar=(mtr.randInt(7)!=0) ? (unsigned char)mtr.randInt() : SomeString[mtr.randInt((MTRand::uint32)SomeString.length()-1)];

        Result.PushBack(AsciiChar);
    }

    return Result;
}


ArrayT<unsigned char> cf::Password::XOR(const ArrayT<unsigned char>& String1, const ArrayT<unsigned char>& String2)
{
    ArrayT<unsigned char> Result;

    assert(String1.Size()==String2.Size());

    for (unsigned long i=0; i<String1.Size(); i++)
    {
        Result.PushBack(String1[i] ^ String2[i]);
    }

    return Result;
}


std::string cf::Password::GenerateArrayCode(const ArrayT<unsigned char>& String)
{
    char        TempStr[64];
    std::string Code;

    sprintf(TempStr, "const unsigned char ObfusPassword[%lu]={ ", String.Size());
    Code+=TempStr;

    for (unsigned long i=0; i<String.Size(); i++)
    {
        sprintf(TempStr, "%u", String[i]);
        Code+=TempStr;
        if (i+1<String.Size()) Code+=", ";
    }

    Code+=" };";
    return Code;
}


std::string cf::Password::ToString(const ArrayT<unsigned char>& String)
{
    std::string Result;

    for (unsigned long i=0; i<String.Size(); i++)
    {
        if (String[i]==0) break;
        Result+=String[i];
    }

    return Result;
}
