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

/***************************************************/
/*** Small and Portable Utility Library (Header) ***/
/***************************************************/

#ifndef _CA_UTIL_HPP_
#define _CA_UTIL_HPP_

#ifndef _WIN32
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _getch getchar
typedef long long int __int64;
#endif


/// A platform independent timer class that allows to measure the time passed since
/// its construction or the last measuring point.
class TimerT
{
    private:

    double  Frequency;
    __int64 TimeStamp1st;
    __int64 TimeStampOld;


    public:

    /// The constructor. It initializes the timer.
    TimerT();

    /// This function returns the time elapsed since the function was last called.
    /// The return value is the elapsed time in seconds.
    /// The very first call of this function returns always 0.
    double GetSecondsSinceLastCall();

    /// This function returns the time elapsed since the timer was constructed.
    /// The return value is the elapsed time in seconds.
    double GetSecondsSinceCtor() const;
};


// Given a string of the form "path/filename.extension" (where individual parts may be missing),
// this functions strips off the file extension, including the '.' character.
// This is done by inserting a '\0' character in the right place.
// void StripExt(char* PathFileExt);

// Given a string of the form "path/filename.extension" (where individual parts may be missing),
// this functions strips off the file name, the file extension, and the possibly preceding '/' or '\\' character.
// This is done by inserting a '\0' character in the right place.
// WARNING: This is pretty problematic when no '/' or '\\' was given!
// void StripFileNameAndExt(char* PathFileExt);

#endif
