/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/******************************************/
/*** Print Help for Windows 32 (Header) ***/
/******************************************/

#ifndef CAFU_WIN32_PRINTHELP_HPP_INCLUDED
#define CAFU_WIN32_PRINTHELP_HPP_INCLUDED


const char* GetString(const char* String, ...);         ///< Writes a 'variable argument string' into a temporary buffer.

void        EnqueueString(const char* String, ...);     ///< Writes a string into the queue.
const char* DequeueString();                            ///< Reads a string from the queue. Returns NULL if the queue is empty.

// void DequeueAllStrings (Zeiger auf Funktion, die const char* nimmt);
// void DequeueToEndOfFile(FileName);   // Erster Aufruf schreibt File neu, danach Append

#endif
