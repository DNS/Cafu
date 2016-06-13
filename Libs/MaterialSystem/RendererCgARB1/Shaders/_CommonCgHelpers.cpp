/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*************************/
/*** Common Cg Helpers ***/
/*************************/

#include "_CommonCgHelpers.hpp"
#include "ConsoleCommands/Console.hpp"

#include <stdio.h>


CGprogram UploadCgProgram(CGcontext CgContext, CGprofile Profile, const char* SourceCode, bool IsObjectCode)
{
    const char* CgCompilerOptions[3]={ "-strict", "-fastprecision", NULL };

    CGprogram NewCgProgram=cgCreateProgram(CgContext, IsObjectCode ? CG_OBJECT : CG_SOURCE, SourceCode, Profile, NULL, CgCompilerOptions);

    if (NewCgProgram==NULL)
    {
        FILE*   ErrorFile=fopen("CgError.txt", "a");
        CGerror err      =cgGetError();

        Console->Print(std::string("ERROR: Unable to create Cg program: ")+cgGetErrorString(err)+"\n");
        if (ErrorFile) fprintf(ErrorFile, "ERROR: Unable to create Cg program: %s\n", cgGetErrorString(err));

        const char* LastListing=cgGetLastListing(CgContext);
        if (LastListing) Console->Print(std::string("LAST LISTING:\n")+LastListing+"\n");
        if (LastListing && ErrorFile) fprintf(ErrorFile, "LAST LISTING:\n%s\n", LastListing);

        if (ErrorFile) fclose(ErrorFile);
        return NULL;
    }

    cgGLLoadProgram(NewCgProgram);

    CGerror err=cgGetError();
    if (err!=CG_NO_ERROR)
    {
        // This should never happen, as we have already made sure elsewhere that the desired profile is supported!
        Console->Print(std::string("ERROR: Unable to load Cg program: ")+cgGetErrorString(err)+"\n");
        return NULL;
    }

    return NewCgProgram;
}
