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

/******************************************/
/*** Print Help for Windows 32 (Header) ***/
/******************************************/

#ifndef _CA_WIN32_PRINTHELP_HPP_
#define _CA_WIN32_PRINTHELP_HPP_


const char* GetString(const char* String, ...);         ///< Writes a 'variable argument string' into a temporary buffer.

void        EnqueueString(const char* String, ...);     ///< Writes a string into the queue.
const char* DequeueString();                            ///< Reads a string from the queue. Returns NULL if the queue is empty.

// void DequeueAllStrings (Zeiger auf Funktion, die const char* nimmt);
// void DequeueToEndOfFile(FileName);   // Erster Aufruf schreibt File neu, danach Append

#endif
