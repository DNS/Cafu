/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************/
/*** Common Helpers ***/
/**********************/

// Required for #include <GL/gl.h> with MS VC++.
#if defined(_WIN32) && defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

#include <GL/gl.h>


#ifndef CAFU_MATSYS_COMMON_HELPERS_HPP_INCLUDED
#define CAFU_MATSYS_COMMON_HELPERS_HPP_INCLUDED

GLenum UploadProgram(GLenum ProgramTarget, const char* ProgramCode);

#endif
